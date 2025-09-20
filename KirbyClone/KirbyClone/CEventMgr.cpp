#include "gamePCH.h"
#include "CEventMgr.h"

#include "CObject.h"
#include "CSceneMgr.h"
#include "CScene.h"
#include "CPlayer.h"
#include "CPlayerHealthSystem.h"
#include "CMonster.h"
#include "CBasicMonster.h"

#include "CCollider.h"
#include "CPlayerStateMachine.h"
#include "CProjectile.h"
#include "CPlayerDataMgr.h"
#include "CSoundMgr.h"
#include "CFadeEffect.h"
#include "CBoss.h"
#include "CCamera.h"
#include "CUIMgr.h"
#include "CTile.h"
#include "CDoor.h"

// 문 전환용 임시 저장 변수
static SCENE_TYPE s_eDoorTargetScene = SCENE_TYPE::START;
static Vec2 s_vDoorTargetPosition = Vec2(0.f, 0.f);

CEventMgr::CEventMgr()
{
}

CEventMgr::~CEventMgr()
{
}

void CEventMgr::init()
{
    // 초기화할 내용이 있다면 여기에
}

void CEventMgr::update()
{
    // 이전 프레임에서 삭제 예정된 오브젝트들 정리
    ClearGarbageObject();

    // 현재 프레임의 모든 이벤트 처리
    for (size_t i = 0; i < m_vecEvent.size(); ++i)
    {
        Execute(m_vecEvent[i]);
    }

    // 처리된 이벤트들 클리어
    m_vecEvent.clear();
}

void CEventMgr::Execute(tEvent& _event)
{
    switch (_event.eType)
    {
    case EVENT_TYPE::CREATE_OBJECT:
    {
        // wParam: GROUP_TYPE, lParam: CObject*
        CObject* pObj = (CObject*)_event.lParam;
        GROUP_TYPE eType = (GROUP_TYPE)_event.wParam;

        CSceneMgr::GetInst()->GetCurScene()->AddObject(pObj, eType);
    }
    break;

    case EVENT_TYPE::DELETE_OBJECT:
    {
        // lParam: CObject*
        CObject* pObj = (CObject*)_event.lParam;
        pObj->SetDead();

        // 가비지 컬렉션에 추가 (다음 프레임에 실제 삭제)
        m_vecGarbage.push_back(pObj);
    }
    break;

    case EVENT_TYPE::SCENE_CHANGE:
    {
        // 씬 변경 시 HoldFade 해제 - 새 씬에서 페이드 효과가 정상 작동하도록
        CFadeEffect::GetInst()->ReleaseFadeHold();
        
        // lParam: SCENE_TYPE
        SCENE_TYPE eNextScene = (SCENE_TYPE)_event.lParam;
        CSceneMgr::GetInst()->ChangeScene(eNextScene);
    }
    break;

    case EVENT_TYPE::COLLISION_ENTER:
    {
        // wParam: CCollider*, lParam: CCollider*
        CCollider* pCol1 = (CCollider*)_event.wParam;
        CCollider* pCol2 = (CCollider*)_event.lParam;

        // 실제 충돌 콜백 호출
        pCol1->GetOwner()->OnCollisionEnter(pCol2);
        pCol2->GetOwner()->OnCollisionEnter(pCol1);
    }
    break;

    case EVENT_TYPE::COLLISION_EXIT:
    {
        // wParam: CCollider*, lParam: CCollider*
        CCollider* pCol1 = (CCollider*)_event.wParam;
        CCollider* pCol2 = (CCollider*)_event.lParam;

        // 실제 충돌 종료 콜백 호출
        pCol1->GetOwner()->OnCollisionExit(pCol2);
        pCol2->GetOwner()->OnCollisionExit(pCol1);
    }
    break;

    case EVENT_TYPE::PLAYER_DAMAGE:
        ExecutePlayerDamage(_event);
        break;

    case EVENT_TYPE::PLAYER_SLIDE_KICK_RECOIL:
        ExecutePlayerSlideKickRecoil(_event);
        break;

    case EVENT_TYPE::MONSTER_DAMAGE:
        ExecuteMonsterDamage(_event);
        break;
        
    case EVENT_TYPE::PLAYER_DEATH:
        ExecutePlayerDeath(_event);
        break;
        
    case EVENT_TYPE::GAME_OVER:
        ExecuteGameOver(_event);
        break;
        
    case EVENT_TYPE::FADE_COMPLETE:
        ExecuteFadeComplete(_event);
        break;
        
    case EVENT_TYPE::BOSS_BATTLE_START:
        ExecuteBossBattleStart(_event);
        break;
        
    case EVENT_TYPE::DOOR_ENTER:
        ExecuteDoorEnter(_event);
        break;
        
    case EVENT_TYPE::STAGE_CLEAR:
        ExecuteStageClear(_event);
        break;
    }
}

void CEventMgr::ClearGarbageObject()
{
    for (size_t i = 0; i < m_vecGarbage.size(); ++i)
    {
        delete m_vecGarbage[i];
    }
    m_vecGarbage.clear();
}

void CEventMgr::ExecutePlayerDamage(tEvent& _event)
{
    // wParam이 플레이어인지 확인 (기존 시스템과의 호환성)
    CObject* pObject = (CObject*)_event.wParam;
    CPlayer* pPlayer = nullptr;
    
    // 타입 확인하여 플레이어 찾기
    if (pObject && pObject->GetType() == OBJECT_TYPE::PLAYER)
    {
        pPlayer = (CPlayer*)pObject;
    }
    else
    {
        // wParam이 사과 등 다른 객체인 경우, 씬에서 플레이어 찾기
        CScene* pCurScene = CSceneMgr::GetInst()->GetCurScene();
        if (pCurScene)
        {
            const vector<CObject*>& vecPlayers = pCurScene->GetGroupObject(GROUP_TYPE::PLAYER);
            if (!vecPlayers.empty())
            {
                pPlayer = (CPlayer*)vecPlayers[0];
            }
        }
    }
    
    Vec2* pKnockbackDir = (Vec2*)_event.lParam;

    if (!pPlayer || pPlayer->IsDead())
    {
        // 메모리 정리
        if (pKnockbackDir)
            delete pKnockbackDir;
        return;
    }

    // 무적 상태 체크 (이중 체크)
    if (pPlayer->GetHealthSystem() &&
        pPlayer->GetHealthSystem()->IsInvincible())
    {
        if (pKnockbackDir)
            delete pKnockbackDir;
        return;
    }

    // === 체력 시스템에서만 데미지 처리 (무적시간, 넉백 등) ===
    // 상태 변경은 PLAYER_STATE_CHANGE 이벤트에서 별도 처리
    Vec2 knockbackDir = pKnockbackDir ? *pKnockbackDir : Vec2(0.f, 0.f);
    if (pPlayer->GetHealthSystem())
    {
        pPlayer->GetHealthSystem()->TakeDamage(1, knockbackDir);
    }

    // 메모리 정리
    if (pKnockbackDir)
        delete pKnockbackDir;
}

void CEventMgr::ExecutePlayerSlideKickRecoil(tEvent& _event)
{
    CProjectile* pProjectile = (CProjectile*)_event.wParam;
    CObject* pMonster = (CObject*)_event.lParam;
    
    if (!pProjectile || !pMonster)
        return;

    // 플레이어 찾기
    CPlayer* pPlayer = nullptr;
    CScene* pCurScene = CSceneMgr::GetInst()->GetCurScene();
    if (pCurScene)
    {
        const vector<CObject*>& playerObjs = pCurScene->GetGroupObject(GROUP_TYPE::PLAYER);
        if (!playerObjs.empty())
        {
            pPlayer = dynamic_cast<CPlayer*>(playerObjs[0]);
        }
    }

    if (!pPlayer)
        return;

    // 플레이어에게 슬라이드킥 반동 요청
    pPlayer->RequestSlideKickRecoil();
}

void CEventMgr::ExecuteMonsterDamage(tEvent& _event)
{
    CMonster* pMonster = (CMonster*)_event.wParam;
    CProjectile* pProjectile = (CProjectile*)_event.lParam;

    if (!pMonster || pMonster->IsDead())
        return;

    // === 보스 데미지 처리 ===
    CBoss* pBoss = dynamic_cast<CBoss*>(pMonster);
    if (pBoss && pProjectile)
    {
        // 투사체 타입별 데미지 계산
        int iDamage = 1; // 기본 데미지
        PROJECTILE_TYPE eProjectileType = pProjectile->GetProjectileType();
        
        // 슬라이드킥과 공기포는 보스에게 데미지 없음
        if (eProjectileType == PROJECTILE_TYPE::KIRBY_SLIDE_KICK || 
            eProjectileType == PROJECTILE_TYPE::KIRBY_AIR_PUFF)
        {
            return; // 데미지 없음
        }
        
        // 별 투사체는 12 데미지
        if (eProjectileType == PROJECTILE_TYPE::KIRBY_STAR)
        {
            iDamage = 12;
        }
        
        // 보스에게 데미지 적용
        pBoss->TakeBossDamage(iDamage);
        return;
    }

    // === 일반 몬스터 데미지 처리 ===
    CBasicMonster* pBasicMonster = dynamic_cast<CBasicMonster*>(pMonster);
    if (pBasicMonster && pProjectile)
    {
        // 데미지 소스 위치 설정 (넉백 방향 계산용)
        Vec2 vSourcePos = pProjectile->GetPos();
        pBasicMonster->SetDamageSourcePos(vSourcePos);
    }

    // 일반 몬스터 데미지 처리
    pMonster->TakeDamage();
}

void CEventMgr::ExecutePlayerDeath(tEvent& _event)
{
    CPlayer* pPlayer = (CPlayer*)_event.wParam;
    if (!pPlayer)
        return;
    
    // HoldFade 해제 - 플레이어 죽음 후 게임오버 처리가 정상 작동하도록
    CFadeEffect::GetInst()->ReleaseFadeHold();
    
    // 씬 일시정지 (다른 오브젝트들 멈춤)
    CScene* pCurrentScene = CSceneMgr::GetInst()->GetCurScene();
    if (pCurrentScene)
    {
        pCurrentScene->SetPaused(true);
    }
    
    // 게임 오버 사운드 재생
    CSoundMgr::GetInst()->PlaySFX(L"gameover");
    
    // 플레이어에게 게임 오버 모션 시작 요청
    pPlayer->StartGameOverSequence();
}

void CEventMgr::ExecuteGameOver(tEvent& _event)
{
    CPlayer* pPlayer = (CPlayer*)_event.wParam;
    if (!pPlayer)
        return;
    
    // HoldFade 강제 해제 - 게임오버 시 페이드가 정상 작동하도록
    CFadeEffect::GetInst()->ReleaseFadeHold();
    
    CPlayerDataMgr* pDataMgr = CPlayerDataMgr::GetInst();
    // 생명 감소 및 플레이어 상태 초기화
    pDataMgr->OnGameOver();
    
    // 생명이 남아있으면 현재 스테이지에서 리스폰
    if (!pDataMgr->IsGameOver())
    {
        // 리스폰용 페이드 아웃 (콜백 데이터 2 = 스테이지 리스폰)
        CFadeEffect::GetInst()->StartFadeOut(FADE_COLOR::WHITE, 0.5f, (DWORD_PTR)2);
    }
    else
    {
        // 생명이 0이 되면 흰색 페이드 아웃 후 StartScene으로 이동
        // 콜백 데이터 1 = 게임 오버 후 씬 변경
        CFadeEffect::GetInst()->StartFadeOut(FADE_COLOR::WHITE, 1.0f, (DWORD_PTR)1);
    }
}

void CEventMgr::ExecuteFadeComplete(tEvent& _event)
{
    // 페이드 완료 후 콜백 데이터 확인
    DWORD_PTR dwCallbackData = _event.wParam;

    // 콜백 데이터별 처리
    if (dwCallbackData == 1)
    {
        // 현재 씬 일시정지 해제 (씬 변경 전에 먼저)
        CScene* pCurrentScene = CSceneMgr::GetInst()->GetCurScene();
        if (pCurrentScene)
        {
            pCurrentScene->SetPaused(false);
        }
        
        // 게임 데이터 초기화 (새 게임)
        CPlayerDataMgr::GetInst()->ResetGame();
        
        // StartScene으로 변경 (마지막에 실행)
        tEvent sceneChangeEvent = {};
        sceneChangeEvent.eType = EVENT_TYPE::SCENE_CHANGE;
        sceneChangeEvent.lParam = (DWORD_PTR)SCENE_TYPE::START;
        AddEvent(sceneChangeEvent);
        
        // 씬 변경 후 페이드 인 효과 (콜백 데이터 0 = 특별한 처리 없음)
        CFadeEffect::GetInst()->StartFadeIn(FADE_COLOR::WHITE, 0.5f, 0);
    }
    else if (dwCallbackData == 2)
    {
        // 스테이지 재시작: 현재 씬을 다시 로드
        SCENE_TYPE eCurSceneType = CSceneMgr::GetInst()->GetCurSceneType();
        
        // 현재 씬 일시정지 해제 (씬 변경 전에 먼저)
        CScene* pCurrentScene = CSceneMgr::GetInst()->GetCurScene();
        if (pCurrentScene)
        {
            pCurrentScene->SetPaused(false);
        }
        
        // 현재 스테이지 재시작을 위한 씬 변경 이벤트
        tEvent sceneChangeEvent = {};
        sceneChangeEvent.eType = EVENT_TYPE::SCENE_CHANGE;
        sceneChangeEvent.lParam = (DWORD_PTR)eCurSceneType;
        AddEvent(sceneChangeEvent);
        
        // 스테이지 재시작 후 페이드 인 효과
        CFadeEffect::GetInst()->StartFadeIn(FADE_COLOR::WHITE, 0.5f, 0);
    }
    else if (dwCallbackData == 3)
    {
        // 저장된 목표 씬과 위치 정보 사용
        SCENE_TYPE eTargetScene = s_eDoorTargetScene;
        Vec2 vSpawnPos = s_vDoorTargetPosition;
        
        // 씬 변경 이벤트
        tEvent sceneChangeEvent = {};
        sceneChangeEvent.eType = EVENT_TYPE::SCENE_CHANGE;
        sceneChangeEvent.lParam = (DWORD_PTR)eTargetScene;
        AddEvent(sceneChangeEvent);
        
        // 씬 변경 후 페이드 인 효과
        CFadeEffect::GetInst()->StartFadeIn(FADE_COLOR::WHITE, 0.5f, 0);
    }
    else if (dwCallbackData == 4)
    {
        // 승리 춤 완료 후 스타트 씬으로 이동
        // 게임 데이터 초기화 (새 게임)
        CPlayerDataMgr::GetInst()->ResetGame();
        
        // StartScene으로 변경
        tEvent sceneChangeEvent = {};
        sceneChangeEvent.eType = EVENT_TYPE::SCENE_CHANGE;
        sceneChangeEvent.lParam = (DWORD_PTR)SCENE_TYPE::START;
        AddEvent(sceneChangeEvent);
        
        // 씬 변경 후 페이드 인 효과
        CFadeEffect::GetInst()->StartFadeIn(FADE_COLOR::WHITE, 0.5f, 0);
    }
}

void CEventMgr::ExecuteBossBattleStart(tEvent& _event)
{
    
    CScene* pCurrentScene = CSceneMgr::GetInst()->GetCurScene();
    if (!pCurrentScene)
        return;
    
    // 트리거 타일에서 카메라 고정 위치 정보 가져오기
    CTile* pTriggerTile = (CTile*)_event.lParam;
    Vec2 vCameraLockPos;
    
    if (pTriggerTile)
    {
        vCameraLockPos = pTriggerTile->GetBossLockPosition();
    }
    else
    {
        vCameraLockPos = Vec2(400.f, 300.f); // 기본값
    }
    
    // WhispyWoods 찾기
    CBoss* pWhispyWoods = nullptr;
    const vector<CObject*>& vecMonster = pCurrentScene->GetGroupObject(GROUP_TYPE::MONSTER);
    for (CObject* pObj : vecMonster)
    {
        if (pObj->GetType() == OBJECT_TYPE::MONSTER_WHISPY_WOODS)
        {
            pWhispyWoods = dynamic_cast<CBoss*>(pObj);
            break;
        }
    }
    
    if (!pWhispyWoods)
    {
        return;
    }
    
    // 카메라를 지정된 위치로 고정
    CCamera::GetInst()->StartBossMode(vCameraLockPos);
    
    // UI 매니저에 보스 HP 바 표시 시작
    CUIMgr::GetInst()->StartBossHPFillAnimation();
    
    // WhispyWoods 보스전 상태 활성화
    pWhispyWoods->StartBossEvent( );
    
    // 씬의 모든 트리거 박스 제거
    RemoveAllTriggerBoxes(pCurrentScene);
}

void CEventMgr::RemoveAllTriggerBoxes(CScene* _pScene)
{
    if (!_pScene)
        return;
    
    // 타일 그룹에서 모든 트리거 박스 찾아서 비활성화
    const vector<CObject*>& vecTiles = _pScene->GetGroupObject(GROUP_TYPE::TILE);
    int deactivatedCount = 0;
    
    for (CObject* pObj : vecTiles)
    {
        if (pObj && pObj->GetType() == OBJECT_TYPE::TILE_TRIGGER)
        {
            CTile* pTile = dynamic_cast<CTile*>(pObj);
            if (pTile && pTile->GetVisualType() == TILE_VISUAL_TYPE::BOSS_TRIGGER)
            {
                // 삭제하는 대신 비활성화
                pTile->SetTriggerActive(false);
                deactivatedCount++;
            }
        }
    }
}

void CEventMgr::ExecuteDoorEnter(tEvent& _event)
{
    CDoor* pDoor = (CDoor*)_event.wParam;
    SCENE_TYPE eTargetScene = (SCENE_TYPE)_event.lParam;
    
    if (!pDoor)
        return;
    
    // 문 오브젝트에서 목표 씬과 위치 정보 가져오기
    s_eDoorTargetScene = pDoor->GetTargetScene();
    s_vDoorTargetPosition = pDoor->GetTargetPosition();
    
    // 플레이어 데이터 매니저에 목표 위치 저장
    CPlayerDataMgr::GetInst()->SetSpawnPosition(s_vDoorTargetPosition);
    
    // 흰색 페이드 아웃 시작 (콜백 데이터 3 = 문 입장)
    CFadeEffect::GetInst()->StartFadeOut(FADE_COLOR::WHITE, 0.5f, (DWORD_PTR)3);
}

void CEventMgr::ExecuteStageClear(tEvent& _event)
{
    CBoss* pBoss = (CBoss*)_event.lParam;
    if (!pBoss)
        return;
    
    // 1단계: 기본 보스 격파 처리만 수행
    // 커비의 업데이트 정지 (입력 차단)
    CScene* pCurrentScene = CSceneMgr::GetInst()->GetCurScene();
    if (pCurrentScene)
    {
        const vector<CObject*>& vecPlayers = pCurrentScene->GetGroupObject(GROUP_TYPE::PLAYER);
        if (!vecPlayers.empty() && vecPlayers[0])
        {
            CPlayer* pPlayer = (CPlayer*)vecPlayers[0];
            // 플레이어를 보스 격파 대기 상태로 설정 (입력 차단)
            pPlayer->SetBossDefeatWaiting(true);
        }
    }
}