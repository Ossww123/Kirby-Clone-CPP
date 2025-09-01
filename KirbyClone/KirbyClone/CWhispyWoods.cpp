#include "pch.h"
#include "CWhispyWoods.h"

#include "CTimeMgr.h"
#include "CSceneMgr.h"
#include "CScene.h"
#include "CCollider.h"
#include "CApple.h"
#include "CProjectileFactory.h"
#include "CProjectile.h"

CWhispyWoods::CWhispyWoods()
    : m_fAttackTimer(0.f)
    , m_fAttackInterval(3.f)
    , m_iLastAttackPattern(0)
    , m_iConsecutiveCount(0)
    , m_fAppleDropTimer(0.f)
    , m_iAppleDropCount(0)
    , m_bAppleDropInProgress(false)
    , m_fAirPuffTimer(0.f)
    , m_iAirPuffCount(0)
    , m_iTargetAirPuffCount(0)
    , m_bAirPuffInProgress(false)
{
    // 오브젝트 타입 설정
    SetType(OBJECT_TYPE::MONSTER_WHISPY_WOODS);

    // 위스피 우드 전용 설정
    m_fSpeed = 0.f;                         // 이동하지 않음
    SetBossHP(64);                          // 보스 체력 64

    // 매우 큰 크기
    SetScale(Vec2(128.f, 160.f));

    // 충돌체 크기 조정 (2x9 타일 = 128x576)
    GetCollider()->SetScale(Vec2(128.f, 576.f));

    // 애니메이션 생성
    LoadAnimationsFromFile(L"whispy_woods_animations.json");
    // 사과 스폰 위치 초기화 (위스피 우드 오른쪽으로 2,4,6,8,10타일, 땅으로부터 7.5타일 위)
    Vec2 bossPos = GetPos();
    float groundY = 544.f;  // 스타트씬 바닥 위치
    for (int i = 0; i < 5; ++i)
    {
        m_vAppleSpawnPositions[i] = Vec2(
            bossPos.x + (2 + i * 2) * 64.f,  // 오른쪽으로 2,4,6,8,10타일 (128, 256, 384, 512, 640픽셀)
            groundY - 7.5f * 64.f             // 땅으로부터 7.5타일 위 (480픽셀)
        );
    }

    // 공기포 발사 위치 설정 (충돌체 우하단 기준 오른쪽 1타일, 아래에서 3타일)
    m_vAirPuffStartPos = Vec2(
        bossPos.x + 64.f,   // 오른쪽으로 1타일
        bossPos.y + 288.f - 192.f  // 충돌체 아래에서 3타일 위 (충돌체 중심에서 +288-192)
    );

    // 위스피우드는 간단한 보스 (페이즈 변화 없음)
    ChangeState(MONSTER_STATE::IDLE);
}

CWhispyWoods::~CWhispyWoods()
{
    // 상위 클래스에서 정리
}

void CWhispyWoods::Move()
{
    // 패배 상태에서는 모든 공격 패턴 중단
    if (GetBossPhase() == BOSS_PHASE::DEFEATED)
    {
        m_bAppleDropInProgress = false;
        m_bAirPuffInProgress = false;
        m_fAttackTimer = 0.f;
        return;
    }

    // 위스피 우드는 이동하지 않음 (고정형 보스)
    // 공격 패턴 실행
    float fDT = CTimeMgr::GetInst()->GetDT();
    
    // IDLE 상태에서만 공격 타이머 업데이트 (보스 이벤트가 시작된 후에만)
    if (GetCurrentState() == MONSTER_STATE::IDLE && 
        !m_bAppleDropInProgress && 
        !m_bAirPuffInProgress && 
        IsBossEventStarted())
    {
        m_fAttackTimer += fDT;
        
        // 공격 간격마다 랜덤 공격 실행
        if (m_fAttackTimer >= m_fAttackInterval)
        {
            ExecuteRandomAttack();
            m_fAttackTimer = 0.f;
            
            // 다음 공격 간격을 3~4초 사이로 랜덤 설정
            m_fAttackInterval = 3.f + ((float)rand() / RAND_MAX);  // 3.0 ~ 4.0초
        }
    }
    
    // 사과 떨어뜨리기 진행 중일 때 순차 처리
    if (m_bAppleDropInProgress)
    {
        m_fAppleDropTimer += fDT;
        
        // 1초마다 사과 하나씩 떨어뜨리기
        if (m_fAppleDropTimer >= 1.0f && m_iAppleDropCount < 3)
        {
            int posIndex = m_iAppleDropPositions[m_iAppleDropCount];
            
            // 현재 바라보는 방향에 따라 사과 위치 계산
            Vec2 bossPos = GetPos();
            int direction = GetDirection(); // 1: 오른쪽, -1: 왼쪽
            
            // Air puff 위치보다 5타일(320픽셀) 높게 설정
            float airPuffY = bossPos.y + 288.f - 192.f;  // Air puff Y 위치
            float appleY = airPuffY - 5 * 64.f;  // Air puff보다 5타일 높게
            
            Vec2 spawnPos = Vec2(
                bossPos.x + direction * (2 + posIndex * 2) * 64.f,  // 바라보는 방향으로 2,4,6,8,10타일
                appleY
            );
            
            CreateApple(spawnPos);
            
            m_iAppleDropCount++;
            m_fAppleDropTimer = 0.f;
            
            // 3개 다 떨어뜨렸으면 패턴 종료
            if (m_iAppleDropCount >= 3)
            {
                m_bAppleDropInProgress = false;
                m_iAppleDropCount = 0;
            }
        }
    }
    
    // 공기포 발사 진행 중일 때 순차 처리
    if (m_bAirPuffInProgress)
    {
        m_fAirPuffTimer += fDT;
        
        // 0.5초마다 공기포 하나씩 발사 (간격 증가)
        if (m_fAirPuffTimer >= 0.5f && m_iAirPuffCount < m_iTargetAirPuffCount)
        {
            int direction = GetDirection(); // 1: 오른쪽, -1: 왼쪽
            // 약간 아래쪽으로 향하도록 방향 설정 (x축: 방향, y축: 0.3 정도 아래로)
            CreateAirPuff(Vec2((float)direction, 0.3f));  // 바라보는 방향 + 약간 아래로
            
            m_iAirPuffCount++;
            m_fAirPuffTimer = 0.f;
            
            // 목표 개수만큼 발사했으면 패턴 종료
            if (m_iAirPuffCount >= m_iTargetAirPuffCount)
            {
                m_bAirPuffInProgress = false;
                m_iAirPuffCount = 0;
            }
        }
    }
    
    // 공격 상태에서 일정 시간 후 IDLE로 복귀
    if (GetCurrentState() == MONSTER_STATE::ATTACK)
    {
        m_fStateTimer += fDT;
        if (m_fStateTimer >= 1.0f)  // 1초 후 IDLE로 복귀
        {
            ChangeState(MONSTER_STATE::IDLE);
            m_fStateTimer = 0.f;
        }
    }
}

void CWhispyWoods::StartBossEvent()
{
    // 기본 보스 이벤트 시작 (보스 상태 설정 등)
    CBoss::StartBossEvent();
    
    // 위스피 우드 전용 시작 처리 (간소화)
    CreateBossIntroEffect();
    PlayBossMusic();
}

void CWhispyWoods::EndBossEvent()
{
    // 보스전 종료 처리 (간소화)
    StopBossMusic();
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
    default:
        // 기본적으로 사과 떨어뜨리기
        AttackPattern1_AppleDrop();
        break;
    }
}

void CWhispyWoods::SetupAnimationMapping()
{
    // 위스피 우드 애니메이션 매핑
    m_mapStateToAnimation[MONSTER_STATE::IDLE] = L"IDLE";
    m_mapStateToAnimation[MONSTER_STATE::ATTACK] = L"ATTACK";     // 사과떨구기와 공기포공격 공통
    m_mapStateToAnimation[MONSTER_STATE::DAMAGE] = L"DAMAGE";    // 데미지받는상태
    m_mapStateToAnimation[MONSTER_STATE::DEFEAT1] = L"DEFEAT1";  // 패배 애니메이션 1단계
    m_mapStateToAnimation[MONSTER_STATE::DEFEAT3] = L"DEFEAT3";  // 패배 애니메이션 3단계
    m_mapStateToAnimation[MONSTER_STATE::EDITOR_IDLE] = L"IDLE"; // 에디터에서도 IDLE 애니메이션 사용
}

void CWhispyWoods::ExecuteRandomAttack()
{
    // 다음 공격 패턴 선택
    int nextPattern = SelectWhispyAttackPattern();
    
    // 선택된 패턴 실행
    switch (nextPattern)
    {
    case 1:
        AttackPattern1_AppleDrop();
        break;
    case 2:
        AttackPattern2_AirPuff();
        break;
    default:
        AttackPattern1_AppleDrop();
        break;
    }
    
    // 마지막 공격 패턴 기록
    if (m_iLastAttackPattern == nextPattern)
    {
        m_iConsecutiveCount++;
    }
    else
    {
        m_iConsecutiveCount = 1;
        m_iLastAttackPattern = nextPattern;
    }
}

int CWhispyWoods::SelectWhispyAttackPattern()
{
    // 2가지 공격 패턴: 1(사과), 2(공기포)
    const int PATTERN_COUNT = 2;
    
    // 연속으로 같은 패턴을 MAX_CONSECUTIVE번 이상 하면 강제로 다른 패턴 선택
    if (m_iConsecutiveCount >= MAX_CONSECUTIVE)
    {
        // 다른 패턴 선택
        int newPattern = (m_iLastAttackPattern % PATTERN_COUNT) + 1;
        
        return newPattern;
    }
    
    // 랜덤 선택
    int randomPattern = (rand() % PATTERN_COUNT) + 1;  // 1 또는 2

    return randomPattern;
}

void CWhispyWoods::AttackPattern1_AppleDrop()
{
    // ATTACK 상태로 변경하여 애니메이션 재생
    ChangeState(MONSTER_STATE::ATTACK);
    
    // 사과 떨어뜨리기 패턴 시작
    m_bAppleDropInProgress = true;
    m_fAppleDropTimer = 0.f;
    m_iAppleDropCount = 0;
    
    // 5개 위치 중 3개 랜덤 선택
    bool used[5] = { false };
    for (int i = 0; i < 3; ++i)
    {
        int pos;
        do {
            pos = rand() % 5;  // 0~4
        } while (used[pos]);
        
        used[pos] = true;
        m_iAppleDropPositions[i] = pos;
    }
}

void CWhispyWoods::AttackPattern2_AirPuff()
{
    // ATTACK 상태로 변경하여 애니메이션 재생
    ChangeState(MONSTER_STATE::ATTACK);
    
    // 공기포 발사 패턴 시작
    m_bAirPuffInProgress = true;
    m_fAirPuffTimer = 0.f;
    m_iAirPuffCount = 0;
    
    // 2~4개 랜덤 결정
    m_iTargetAirPuffCount = 2 + (rand() % 3);  // 2, 3, 또는 4
}

void CWhispyWoods::CreateApple(Vec2 _vPos)
{
    // 사과 오브젝트 생성
    CApple* pApple = new CApple();
    
    if (pApple)
    {
        pApple->SetPos(_vPos);
        pApple->SetScale(Vec2(32.f, 32.f));  // 적절한 크기
        
        // 씬에 추가
        CScene* pScene = CSceneMgr::GetInst()->GetCurScene();
        if (pScene)
        {
            pScene->AddObject(pApple, GROUP_TYPE::MONSTER);
        }
        else
        {
            delete pApple;
        }
    }
    else
    {
        OutputDebugStringW(L"WhispyWoods: ERROR - Failed to create CApple object!\n");
    }
}

void CWhispyWoods::CreateAirPuff(Vec2 _vDirection)
{
    // 현재 바라보는 방향에 따라 공기포 발사 위치 계산
    Vec2 bossPos = GetPos();
    int direction = GetDirection(); // 1: 오른쪽, -1: 왼쪽
    
    Vec2 airPuffStartPos = Vec2(
        bossPos.x + direction * 64.f,   // 바라보는 방향으로 1타일
        bossPos.y + 288.f - 192.f       // 충돌체 아래에서 3타일 위
    );
    
    // 공기포 투사체 생성
    CProjectile* pAirPuff = CProjectileFactory::CreateBossAirPuff(
        airPuffStartPos, 
        _vDirection, 
        GROUP_TYPE::PROJ_MONSTER
    );
    
    if (pAirPuff)
    {
        // 3배 빠른 속도로 설정
        pAirPuff->SetSpeed(450.f);
        
        
        
        // 씬에 추가
        CProjectileFactory::AddToScene(pAirPuff, GROUP_TYPE::MONSTER);
    }
    else
    {
        OutputDebugStringW(L"WhispyWoods: Failed to create air puff projectile\n");
    }
}