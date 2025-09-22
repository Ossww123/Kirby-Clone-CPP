#include "gamePCH.h"
#include "CHotHead.h"
#include "CSceneMgr.h"
#include "CScene.h"
#include "CPlayer.h"
#include "CProjectileFactory.h"
#include "CProjectile.h"
#include "CTimeMgr.h"
#include "CRigidBody.h"
#include "CSoundMgr.h"

CHotHead::CHotHead()
    : m_bFireSpat(false)
    , m_fFireTimer(0.f)
{
    // 오브젝트 타입 설정
    SetType(OBJECT_TYPE::MONSTER_HOT_HEAD);

    // 핫 헤드 전용 설정
    m_fSpeed = 90.f;                        // 빠른 이동
    SetAttackCooldown(5.f);                 // 5초 쿨타임 (요청사항)
    m_fAttackRange = 180.f;                 // 180픽셀 범위
    m_fAttackReadyTime = 0.6f;              // 0.6초 준비
    m_fAttackDuration = 2.0f;               // 2.0초 공격 (DPS형으로 변경)

    // 애니메이션 로드
    LoadAnimationsFromFile(L"hot_head_animations.json");
    SetupAnimationMapping();

    // 초기 상태 설정
    ChangeState(MONSTER_STATE::IDLE);
}

CHotHead::~CHotHead()
{
    // 화염 투사체들 정리
    ClearFireProjectiles();
    // 상위 클래스에서 정리
}

void CHotHead::Move ( )
{
    // TURN 상태이거나 공격 관련 상태에서는 Move 로직 실행하지 않음
    MONSTER_STATE eCurrentState = GetCurrentState ( );
    if ( eCurrentState == MONSTER_STATE::TURN ||
        eCurrentState == MONSTER_STATE::ATTACK_READY ||
        eCurrentState == MONSTER_STATE::ATTACK ||
        eCurrentState == MONSTER_STATE::DAMAGE ||
        eCurrentState == MONSTER_STATE::BEING_INHALED )
    {
        return;
    }

    // 플레이어 탐지 및 공격 체크 (공격 쿨타임이 끝났을 때만)
    if ( CanAttack ( ) && IsPlayerInRange ( ) )
    {
        ChangeState ( MONSTER_STATE::ATTACK_READY );
        return;
    }

    // 벽과 충돌했으면 방향 전환 (충돌 콜백에서 이미 방향이 바뀌었지만 상태도 변경)
    if (m_bWallCollision && !m_bPrevWallCollision)
    {
        ChangeState(MONSTER_STATE::TURN);
        return;
    }

    // 바닥과 충돌하지 않으면 방향 전환 (낭떠러지 감지)
    if (!m_bGroundCollision && m_bPrevGroundCollision)
    {
        ChangeState(MONSTER_STATE::TURN);
        return;
    }

    // 계속 걷기
    MoveHorizontal ( m_fSpeed );
}

void CHotHead::Attack()
{
    // 화염 공격 시작 (상태 시작 시 한 번만)
    if (!m_bFireSpat)
    {
        SpitFire();
        m_bFireSpat = true;
        m_fFireTimer = 0.f;
    }
}

void CHotHead::UpdateAttackReady()
{
    // 부모 클래스 호출
    CCopyMonster::UpdateAttackReady();
    
    // ATTACK_READY 상태에서는 화염 발사 준비
    m_bFireSpat = false;      // 다음 공격을 위해 리셋
    m_fFireTimer = 0.f;       // 화염 발사 타이머 리셋
    ClearFireProjectiles();   // 이전 화염들 정리
}

void CHotHead::UpdateAttack()
{
    
    // 화염 발사 타이머 업데이트
    m_fFireTimer += CTimeMgr::GetInst()->GetfDT();
    
    // 0.15초마다 화염 발사 (커비보다 약간 느림)
    const float FIRE_INTERVAL = 0.15f;
    if (m_fFireTimer >= FIRE_INTERVAL)
    {
        // 현재 활성화된 화염 투사체 개수 확인 (최대 4개로 제한)
        int activeFireCount = 0;
        for (CProjectile* pFire : m_vecFireProjectiles)
        {
            if (pFire && !pFire->IsDead())
            {
                activeFireCount++;
            }
        }
        
        // 4개 미만일 때만 새로운 화염 생성
        if (activeFireCount < 4)
        {
            CreateFireProjectile();
        }
        
        m_fFireTimer = 0.f; // 타이머 리셋
    }
    
    // 공격 시간이 끝나면 상태 변경
    if (m_fStateTimer >= m_fAttackDuration)
    {
        EndAttack();
    }
    else
    {
        // 공격 중에는 이동 정지
        if (nullptr != GetRigidBody())
        {
            GetRigidBody()->SetVelocityX(0.f);
        }
    }
}

void CHotHead::SetupAnimationMapping()
{
    // 핫 헤드 상태별 애니메이션 매핑
    m_mapStateToAnimation[MONSTER_STATE::IDLE] = L"IDLE";
    m_mapStateToAnimation[MONSTER_STATE::WALK] = L"WALK";
    m_mapStateToAnimation[MONSTER_STATE::TURN] = L"WALK";
    m_mapStateToAnimation[MONSTER_STATE::ATTACK_READY] = L"ATTACK_READY";  // 불 뿜기 준비
    m_mapStateToAnimation[MONSTER_STATE::ATTACK] = L"ATTACK";              // 불 뿜기
    m_mapStateToAnimation[MONSTER_STATE::DAMAGE] = L"DAMAGE";
    m_mapStateToAnimation[MONSTER_STATE::BEING_INHALED] = L"DAMAGE";
}

void CHotHead::SpitFire()
{
    // 화염 뿜기 로직 - DPS형 연속 발사 시작
    m_fFireTimer = 0.f;

    // TODO: 화염 발사 사운드
    // TODO: 화염 발사 이펙트
}

void CHotHead::CreateFireProjectile()
{
    // 기존 사운드를 중단하고 새로운 사운드 재생
    CSoundMgr::GetInst()->StopSFX(L"hothead");
    CSoundMgr::GetInst()->PlaySFX(L"hothead");
    
    // 플레이어 방향으로 화염 발사 (수평 직선 - 커비 스타일)
    Vec2 vDirection = GetPlayerDirection();
    Vec2 vSpawnPos = GetPos();
    
    CScene* pScene = CSceneMgr::GetInst()->GetCurScene();
    if (!pScene)
    {
        return;
    }

    // 입 위치에서 발사
    vSpawnPos.x += (vDirection.x > 0) ? 25.f : -25.f;
    vSpawnPos.y -= 5.f; // 약간 위쪽에서
    
    // 수평 직선으로 발사 (커비 FIRE 스타일)
    vDirection.y = 0.f; // 수평으로만 발사
    vDirection = vDirection.GetNormalized();
    
    // 몬스터용 화염탄 생성
    CProjectile* pFire = CProjectileFactory::CreateMonsterFireball(vSpawnPos, vDirection, GROUP_TYPE::MONSTER);
    if (pFire)
    {
        pFire->SetSpeed(300.f);  // 일정한 속도로 발사

        // 씬에 투사체 추가
        pScene->AddObject(pFire, GROUP_TYPE::PROJ_MONSTER);
        
        // 화염 목록에 추가
        m_vecFireProjectiles.push_back(pFire);
    }
}

void CHotHead::ClearFireProjectiles()
{
    // 화염 투사체들을 삭제하지 않고 벡터에서만 제거
    // (실제 삭제는 씬에서 처리)
    for (CProjectile* pFire : m_vecFireProjectiles)
    {
        if (pFire)
        {
            pFire->SetDead();  // 삭제 표시
        }
    }
    m_vecFireProjectiles.clear();
}
