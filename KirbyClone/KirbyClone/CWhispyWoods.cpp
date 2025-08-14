#include "pch.h"
#include "CWhispyWoods.h"

#include "CTimeMgr.h"
#include "CSceneMgr.h"
#include "CScene.h"

CWhispyWoods::CWhispyWoods()
    : m_iCurrentRootIndex(0)
    , m_fAppleDropTimer(0.f)
    , m_fAirPuffTimer(0.f)
    , m_bFinalPhaseStarted(false)
{
    // 오브젝트 타입 설정
    SetType(OBJECT_TYPE::MONSTER_WHISPY_WOODS);

    // 위스피 우드 전용 설정
    m_fSpeed = 0.f;                         // 이동하지 않음
    SetBossHP(1500);                        // 높은 체력

    // 매우 큰 크기
    SetScale(Vec2(128.f, 160.f));

    // 충돌체 크기 조정
    GetCollider()->SetScale(Vec2(100.f, 140.f));

    // 뿌리 공격 위치 설정 (보스 주변)
    Vec2 bossPos = GetPos();
    m_vRootPositions[0] = Vec2(bossPos.x - 200.f, bossPos.y + 100.f);
    m_vRootPositions[1] = Vec2(bossPos.x - 100.f, bossPos.y + 100.f);
    m_vRootPositions[2] = Vec2(bossPos.x, bossPos.y + 100.f);
    m_vRootPositions[3] = Vec2(bossPos.x + 100.f, bossPos.y + 100.f);
    m_vRootPositions[4] = Vec2(bossPos.x + 200.f, bossPos.y + 100.f);

    // 애니메이션 생성
    CreateAnimations();

    // 보스 인트로 상태로 시작
    SetBossPhase(BOSS_PHASE::INTRO);
    ChangeState(MONSTER_STATE::IDLE);
}

CWhispyWoods::~CWhispyWoods()
{
    // 상위 클래스에서 정리
}

void CWhispyWoods::Move()
{
    // 위스피 우드는 이동하지 않음 (고정형 보스)
    // 대신 보스 페이즈 업데이트
    UpdateBossPhase();
}

void CWhispyWoods::StartBossEvent()
{
    // 보스전 시작 처리
    CreateBossArena();
    CreateBossIntroEffect();
    PlayBossMusic();

    // TODO: 카메라 고정
    // TODO: 플레이어 이동 제한
    // TODO: UI 표시 (보스 체력바 등)
}

void CWhispyWoods::EndBossEvent()
{
    // 보스전 종료 처리
    DestroyBossArena();
    StopBossMusic();

    // TODO: 카메라 해제
    // TODO: 플레이어 이동 해제
    // TODO: UI 숨기기
    // TODO: 보상 아이템 생성
}

void CWhispyWoods::ExecuteAttackPattern(BOSS_ATTACK_PATTERN _ePattern)
{
    switch (_ePattern)
    {
    case BOSS_ATTACK_PATTERN::PATTERN_1:
        AttackPattern1_AppleDrop();
        break;
    case BOSS_ATTACK_PATTERN::PATTERN_2:
        AttackPattern2_AirPuff();
        break;
    case BOSS_ATTACK_PATTERN::PATTERN_3:
        AttackPattern3_RootAttack();
        break;
    case BOSS_ATTACK_PATTERN::PATTERN_4:
        AttackPattern4_LeafStorm();
        break;
    case BOSS_ATTACK_PATTERN::PATTERN_5:
        AttackPattern5_FinalAttack();
        break;
    }
}

void CWhispyWoods::CreateAnimations()
{
    // 위스피 우드는 큰 스프라이트 사용 (가정: 별도 텍스처 파일)
    // 실제로는 boss_whispy_woods.bmp 등의 별도 파일 사용

    // 기본 애니메이션 (눈 깜빡임)
    Vec2 startPos = Vec2(0.f, 0.f);
    Vec2 bigFrameSize = Vec2(128.f, 160.f);

    CreateBasicAnimation(L"IDLE", startPos, 2, bigFrameSize, Vec2(128.f, 0.f), 1.f, true);

    // 공격 준비 애니메이션 (화난 표정)
    CreateBasicAnimation(L"ATTACK_READY", Vec2(256.f, 0.f), 2, bigFrameSize, Vec2(128.f, 0.f), 0.3f, true);

    // 공격 애니메이션 (입 벌리기)
    CreateBasicAnimation(L"ATTACK", Vec2(512.f, 0.f), 3, bigFrameSize, Vec2(128.f, 0.f), 0.2f, false);

    // 데미지 애니메이션 (아픈 표정)
    CreateBasicAnimation(L"DAMAGE", Vec2(768.f, 0.f), 2, bigFrameSize, Vec2(128.f, 0.f), 0.3f, false);
}

void CWhispyWoods::AttackPattern1_AppleDrop()
{
    // 사과 떨어뜨리기 공격
    Vec2 bossPos = GetPos();

    // 플레이어 위치 추정 (TODO: 실제 플레이어 위치 가져오기)
    Vec2 playerPos = Vec2(bossPos.x, bossPos.y + 200.f);

    // 플레이어 주변에 사과들 생성
    for (int i = 0; i < 5; ++i)
    {
        float offsetX = (i - 2) * 50.f;  // -100, -50, 0, 50, 100
        Vec2 applePos = Vec2(playerPos.x + offsetX, bossPos.y - 50.f);
        CreateApple(applePos);
    }

    // 공격 애니메이션 재생
    ChangeState(MONSTER_STATE::ATTACK);
}

void CWhispyWoods::AttackPattern2_AirPuff()
{
    // 바람 불기 공격
    Vec2 bossPos = GetPos();

    // 좌우로 바람 생성
    CreateAirPuff(Vec2(-1.f, 0.f));  // 왼쪽으로
    CreateAirPuff(Vec2(1.f, 0.f));   // 오른쪽으로

    // 페이즈에 따라 추가 바람
    if (GetBossPhase() >= BOSS_PHASE::PHASE_2)
    {
        CreateAirPuff(Vec2(-0.7f, -0.7f));  // 대각선
        CreateAirPuff(Vec2(0.7f, -0.7f));   // 대각선
    }

    // 공격 애니메이션 재생
    ChangeState(MONSTER_STATE::ATTACK);
}

void CWhispyWoods::AttackPattern3_RootAttack()
{
    // 뿌리 공격 (순차적으로 뿌리가 올라옴)
    for (int i = 0; i < 5; ++i)
    {
        // 0.3초 간격으로 뿌리 생성
        // TODO: 타이머를 이용한 순차 생성 구현
        CreateRoot(m_vRootPositions[i]);
    }

    m_iCurrentRootIndex = 0;

    // 공격 애니메이션 재생
    ChangeState(MONSTER_STATE::ATTACK);
}

void CWhispyWoods::AttackPattern4_LeafStorm()
{
    // 잎사귀 폭풍 (2페이즈부터 사용)
    Vec2 bossPos = GetPos();

    // 원형으로 잎사귀 발사
    for (int i = 0; i < 12; ++i)
    {
        float angle = (i * 30.f) * 3.14159f / 180.f;  // 30도씩
        Vec2 direction = Vec2(cosf(angle), sinf(angle));
        CreateLeaf(bossPos, direction);
    }

    // 공격 애니메이션 재생
    ChangeState(MONSTER_STATE::ATTACK);
}

void CWhispyWoods::AttackPattern5_FinalAttack()
{
    // 최종 공격 (3페이즈 전용)
    if (!m_bFinalPhaseStarted)
    {
        m_bFinalPhaseStarted = true;
        ShakeScreen();
    }

    // 모든 공격을 동시에 실행
    AttackPattern1_AppleDrop();
    AttackPattern2_AirPuff();
    AttackPattern3_RootAttack();
    AttackPattern4_LeafStorm();

    // 공격 애니메이션 재생
    ChangeState(MONSTER_STATE::ATTACK);
}

void CWhispyWoods::CreateApple(Vec2 _vPos)
{
    // 사과 투사체 생성
    // TODO: CApple 클래스 구현 후 생성
    // CApple* pApple = new CApple;
    // pApple->SetPos(_vPos);
    // pApple->SetGravity(true);  // 중력 적용
    // pApple->SetDamage(1);
    // 
    // CScene* pCurScene = CSceneMgr::GetInst()->GetCurScene();
    // pCurScene->AddObject(pApple, GROUP_TYPE::BOSS_PROJECTILE);
}

void CWhispyWoods::CreateAirPuff(Vec2 _vDirection)
{
    // 바람 투사체 생성
    // TODO: CAirPuff 클래스 구현 후 생성
    // CAirPuff* pAirPuff = new CAirPuff;
    // pAirPuff->SetPos(GetPos());
    // pAirPuff->SetDirection(_vDirection);
    // pAirPuff->SetSpeed(200.f);
    // pAirPuff->SetPushForce(400.f);  // 플레이어를 밀어내는 힘
    // 
    // CScene* pCurScene = CSceneMgr::GetInst()->GetCurScene();
    // pCurScene->AddObject(pAirPuff, GROUP_TYPE::BOSS_PROJECTILE);
}

void CWhispyWoods::CreateRoot(Vec2 _vPos)
{
    // 뿌리 공격 생성
    // TODO: CRoot 클래스 구현 후 생성
    // CRoot* pRoot = new CRoot;
    // pRoot->SetPos(_vPos);
    // pRoot->SetWarningTime(1.f);   // 1초 경고 후 공격
    // pRoot->SetDamage(2);
    // 
    // CScene* pCurScene = CSceneMgr::GetInst()->GetCurScene();
    // pCurScene->AddObject(pRoot, GROUP_TYPE::BOSS_PROJECTILE);
}

void CWhispyWoods::CreateLeaf(Vec2 _vPos, Vec2 _vDirection)
{
    // 잎사귀 투사체 생성
    // TODO: CLeaf 클래스 구현 후 생성
    // CLeaf* pLeaf = new CLeaf;
    // pLeaf->SetPos(_vPos);
    // pLeaf->SetDirection(_vDirection);
    // pLeaf->SetSpeed(150.f);
    // pLeaf->SetLifetime(3.f);
    // pLeaf->SetDamage(1);
    // 
    // CScene* pCurScene = CSceneMgr::GetInst()->GetCurScene();
    // pCurScene->AddObject(pLeaf, GROUP_TYPE::BOSS_PROJECTILE);
}

void CWhispyWoods::ShakeScreen()
{
    // 화면 진동 효과
    // TODO: 카메라 매니저를 통한 화면 진동
    // CCameraMgr::GetInst()->StartShake(2.f, 10.f);  // 2초간 진동
}

void CWhispyWoods::CreateBossArena()
{
    // 보스 전투 공간 생성 (벽 생성 등)
    // TODO: 보스 아레나 벽 생성
}

void CWhispyWoods::DestroyBossArena()
{
    // 보스 전투 공간 제거
    // TODO: 보스 아레나 벽 제거
}