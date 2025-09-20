#include "gamePCH.h"
#include "CSparky.h"
#include "CRigidBody.h"
#include "CTimeMgr.h"
#include "CProjectileFactory.h"
#include "CProjectile.h"
#include "CSceneMgr.h"
#include "CScene.h"
#include "CPlayer.h"
#include "CSoundMgr.h"

CSparky::CSparky()
    : m_eSparkyState(CSparky::SPARKY_STATE::IDLE)
    , m_fIdleTimer(0.f)
    , m_fIdleDuration(1.0f)
    , m_eJumpType(CSparky::SPARKY_JUMP_TYPE::SMALL_IN_PLACE)
    , m_bJumping(false)
    , m_vJumpStartPos(Vec2(0.f, 0.f))
    , m_vJumpTargetPos(Vec2(0.f, 0.f))
    , m_fJumpProgress(0.f)
    , m_fJumpDuration(0.8f)
    , m_vKirbyDirection(Vec2(1.f, 0.f))
    , m_bElectricFieldCreated(false)
    , m_fSoundTimer(0.f)
    , m_fSoundInterval(0.3f)  // 0.3초마다 사운드 반복
{
    // 오브젝트 타입 설정
    SetType(OBJECT_TYPE::MONSTER_SPARKY);

    // 스파키 전용 설정
    m_fSpeed = 70.f;                        // 느린 이동 (점프로 보완)
    SetAttackCooldown(5.f);                 // 5초 쿨타임 (강력한 공격)
    m_fAttackRange = 120.f;                 // 120픽셀 범위
    m_fAttackReadyTime = 1.2f;              // 1.2초 준비 (긴 준비시간)
    m_fAttackDuration = 1.5f;               // 1.5초 공격 (긴 지속시간)

    // 애니메이션 로드
    LoadAnimationsFromFile(L"sparky_animations.json");
    SetupAnimationMapping();

    // 초기 상태 설정
    ChangeState(MONSTER_STATE::IDLE);
}

CSparky::~CSparky()
{
    // 상위 클래스에서 정리
}

void CSparky::Move()
{
    // 공격 쿨타임 업데이트 (모든 상태에서 실행)
    UpdateAttackCooldown();
    
    // 플레이어 위치 업데이트 (매우 중요!)
    UpdatePlayerPosition();
    
    // 커비 방향 업데이트
    UpdateKirbyDirection();
    
    // 공격 관련 상태에서는 Move 로직 실행하지 않음
    MONSTER_STATE eCurrentState = GetCurrentState();
    if (eCurrentState == MONSTER_STATE::ATTACK_READY ||
        eCurrentState == MONSTER_STATE::ATTACK ||
        eCurrentState == MONSTER_STATE::DAMAGE ||
        eCurrentState == MONSTER_STATE::BEING_INHALED)
    {
        return;
    }
    
    // 스파키 고유 상태 업데이트
    UpdateSparkyState();
}

void CSparky::Attack()
{
    // 빈 구현 (실제 공격 로직은 UpdateAttack에서 처리)
}

void CSparky::UpdateAttackReady()
{
    // 부모 클래스 호출
    CCopyMonster::UpdateAttackReady();
    
    // ATTACK_READY 상태에서는 전기 공격 준비
    m_bElectricFieldCreated = false;  // 다음 공격을 위해 리셋
    
    OutputDebugStringW(L"[Sparky] UpdateAttackReady called\n");
}

void CSparky::UpdateAttack()
{
    static bool firstCall = true;
    if (firstCall)
    {
        wchar_t debugMsg[256];
        swprintf_s(debugMsg, L"[Sparky] UpdateAttack started - Duration: %.1f seconds\n", m_fAttackDuration);
        OutputDebugStringW(debugMsg);
        firstCall = false;
    }
    
    // 전기 공격 실행 (상태 시작 시 한 번만)
    if (!m_bElectricFieldCreated)
    {
        OutputDebugStringW(L"[Sparky] Creating electric field\n");
        CreateElectricField();
        m_bElectricFieldCreated = true;
        m_fSoundTimer = 0.f;  // 사운드 타이머 초기화
    }

    // 사운드 반복 재생
    m_fSoundTimer += CTimeMgr::GetInst()->GetfDT();
    if (m_fSoundTimer >= m_fSoundInterval)
    {
        CSoundMgr::GetInst()->PlaySFX(L"sparky");
        m_fSoundTimer = 0.f;  // 타이머 리셋
    }

    // 정적 전기장이므로 추가 스파크 생성 불필요
    // (전기장은 공격 시작 시 한 번만 생성됨)
    
    // 공격 진행 상황 디버깅
    static float lastReportTime = 0.f;
    if (m_fStateTimer - lastReportTime >= 0.5f)
    {
        wchar_t progressMsg[256];
        swprintf_s(progressMsg, L"[Sparky] Attack progress: %.1f/%.1f seconds\n", m_fStateTimer, m_fAttackDuration);
        OutputDebugStringW(progressMsg);
        lastReportTime = m_fStateTimer;
    }
    
    // 공격 시간이 끝나면 상태 변경
    if (m_fStateTimer >= m_fAttackDuration)
    {
        OutputDebugStringW(L"[Sparky] Attack completed, calling EndAttack()\n");
        EndAttack();
        firstCall = true; // 다음 공격을 위해 리셋
        lastReportTime = 0.f;
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

void CSparky::EndAttack()
{
    OutputDebugStringW(L"[Sparky] EndAttack called - Setting cooldown and returning to IDLE\n");
    
    // Sparky 공격 사운드 정지
    CSoundMgr::GetInst()->StopSFX(L"sparky");
    
    // 부모 클래스의 쿨타임 설정
    m_bAttacking = false;
    m_fAttackCooldown = m_fMaxAttackCooldown;  // 쿨타임 시작
    
    // Sparky는 WALK가 아닌 IDLE로 돌아감
    ChangeState(MONSTER_STATE::IDLE);
    
    // Sparky 내부 상태도 IDLE로 리셋
    m_eSparkyState = CSparky::SPARKY_STATE::IDLE;
    m_fIdleTimer = 0.f;
    
    wchar_t debugMsg[256];
    swprintf_s(debugMsg, L"[Sparky] Attack ended - Cooldown: %.1f seconds, State: IDLE\n", m_fAttackCooldown);
    OutputDebugStringW(debugMsg);
}

void CSparky::SetupAnimationMapping()
{
    // 스파키 상태별 애니메이션 매핑
    m_mapStateToAnimation[MONSTER_STATE::IDLE] = L"IDLE";
    m_mapStateToAnimation[MONSTER_STATE::WALK] = L"JUMP";          // 통통 뛰는 애니메이션
    m_mapStateToAnimation[MONSTER_STATE::TURN] = L"JUMP";
    m_mapStateToAnimation[MONSTER_STATE::ATTACK_READY] = L"ATTACK_READY";       // 전기 모으기
    m_mapStateToAnimation[MONSTER_STATE::ATTACK] = L"ATTACK";    // 전기장 공격
    m_mapStateToAnimation[MONSTER_STATE::DAMAGE] = L"DAMAGE";
    m_mapStateToAnimation[MONSTER_STATE::BEING_INHALED] = L"DAMAGE";
}

void CSparky::UpdateWalk()
{
    // 공격 쿨타임 업데이트만 직접 호출 (중복 공격 감지 방지)
    UpdateAttackCooldown();
    
    // 스파키는 기본 Walk 대신 자체 상태 시스템 사용
    UpdateSparkyState();
}

void CSparky::UpdateSparkyState()
{
    float fDT = CTimeMgr::GetInst()->GetfDT();
    
    switch (m_eSparkyState)
    {
    case CSparky::SPARKY_STATE::IDLE:
        UpdateSparkyIdle();
        break;
    case CSparky::SPARKY_STATE::JUMP:
        UpdateSparkyJump();
        break;
    }
}

void CSparky::UpdateIdle()
{
    // 스파키는 기본 Idle에서도 자체 상태 시스템 사용
    UpdateSparkyState();
}

void CSparky::UpdateSparkyIdle()
{
    float fDT = CTimeMgr::GetInst()->GetfDT();
    m_fIdleTimer += fDT;
    
    // 디버깅: 현재 상태 정보 출력
    MONSTER_STATE currentState = GetCurrentState();
    bool canAttack = CanAttack();
    bool playerInRange = IsPlayerInRange();
    
    // 거리 계산 디버깅
    Vec2 myPos = GetPos();
    Vec2 playerPos = GetPlayerPos(); // m_vPlayerPos 값
    Vec2 vDist = playerPos - myPos;
    float fDistance = vDist.Length();
    float fAttackRange = m_fAttackRange;
    
    // 쿨타임 정보 추가
    float fCooldownRemaining = GetAttackCooldown();
    bool bAttacking = IsAttacking();
    
    static float debugTimer = 0.f;
    debugTimer += fDT;
    if (debugTimer >= 1.0f) // 1초마다 출력
    {
        wchar_t debugMsg[1024];
        swprintf_s(debugMsg, 
            L"[Sparky Debug] State: %d, SparkyState: %d, CanAttack: %d, PlayerInRange: %d, Attacking: %d\n"
            L"  MyPos: (%.1f, %.1f), PlayerPos: (%.1f, %.1f), Distance: %.1f, Range: %.1f\n"
            L"  IdleTimer: %.2f, CooldownRemaining: %.2f\n",
            (int)currentState, (int)m_eSparkyState, canAttack, playerInRange, bAttacking,
            myPos.x, myPos.y, playerPos.x, playerPos.y, fDistance, fAttackRange,
            m_fIdleTimer, fCooldownRemaining);
        OutputDebugStringW(debugMsg);
        debugTimer = 0.f;
    }
    
    // IDLE 상태일 때만 플레이어 탐지 및 공격 체크 (공격 쿨타임이 끝났을 때만)
    if (currentState == MONSTER_STATE::IDLE && 
        canAttack && playerInRange)
    {
        OutputDebugStringW(L"[Sparky] Starting attack! Changing to ATTACK_READY state\n");
        ChangeState(MONSTER_STATE::ATTACK_READY);
        return;
    }
    
    // IDLE 지속시간이 끝나면 점프 시작
    if (m_fIdleTimer >= m_fIdleDuration)
    {
        StartJump();
        m_eSparkyState = CSparky::SPARKY_STATE::JUMP;
        m_fIdleTimer = 0.f;
        
        // 다음 IDLE 지속시간 랜덤 설정 (0.8~1.5초)
        m_fIdleDuration = 0.8f + (float)(rand() % 8) * 0.1f;
        
        OutputDebugStringW(L"[Sparky] Starting jump\n");
    }
}

void CSparky::UpdateSparkyJump()
{
    float fDT = CTimeMgr::GetInst()->GetfDT();
    m_fJumpProgress += fDT / m_fJumpDuration;
    
    if (m_fJumpProgress >= 1.0f && GetRigidBody()->IsGround())
    {
        // 점프 완료
        m_fJumpProgress = 1.0f;
        SetPos(m_vJumpTargetPos);
        m_eSparkyState = CSparky::SPARKY_STATE::IDLE;
        m_bJumping = false;
    }
    else
    {
        // 포물선 이동 (충돌 검사 포함)
        Vec2 vCurrentPos = CalculateParabolicPosition(m_fJumpProgress);
        
        // 점프 중 충돌 검사
        if (CheckJumpCollision(vCurrentPos))
        {
            // 충돌 시 점프 중단하고 현재 위치에서 착지
            m_fJumpProgress = 1.0f;
            m_eSparkyState = CSparky::SPARKY_STATE::IDLE;
            m_bJumping = false;
            
            // RigidBody에 의존하여 자연스러운 착지 처리
            if (GetRigidBody())
            {
                GetRigidBody()->SetVelocityX(0.f);
                GetRigidBody()->SetVelocityY(0.f);
            }
            
            OutputDebugStringW(L"[Sparky] Jump collision detected - landing early\n");
        }
        else
        {
            SetPos(vCurrentPos);
        }
    }
}

void CSparky::StartJump()
{
    m_eJumpType = SelectRandomJumpType();
    m_vJumpStartPos = GetPos();
    CalculateJumpTarget();
    m_fJumpProgress = 0.f;
    m_bJumping = true;
    
    // 점프 타입에 따른 지속시간 설정
    switch (m_eJumpType)
    {
    case CSparky::SPARKY_JUMP_TYPE::SMALL_IN_PLACE:
        m_fJumpDuration = 0.6f;
        break;
    case CSparky::SPARKY_JUMP_TYPE::SMALL_FORWARD:
        m_fJumpDuration = 0.8f;
        break;
    case CSparky::SPARKY_JUMP_TYPE::BIG_FORWARD:
        m_fJumpDuration = 1.0f;
        break;
    }
}

CSparky::SPARKY_JUMP_TYPE CSparky::SelectRandomJumpType()
{
    int randValue = rand() % 100;
    
    // 확률 분배: 제자리(30%), 작은점프(40%), 큰점프(30%)
    if (randValue < 30)
        return CSparky::SPARKY_JUMP_TYPE::SMALL_IN_PLACE;
    else if (randValue < 70)
        return CSparky::SPARKY_JUMP_TYPE::SMALL_FORWARD;
    else
        return CSparky::SPARKY_JUMP_TYPE::BIG_FORWARD;
}

void CSparky::CalculateJumpTarget()
{
    float fTileSize = GetTileSize();
    Vec2 vCurrentPos = GetPos();
    
    switch (m_eJumpType)
    {
    case CSparky::SPARKY_JUMP_TYPE::SMALL_IN_PLACE:
        // 제자리 점프 (x축 이동 없음) - 현재 바닥 높이 유지
        m_vJumpTargetPos = vCurrentPos;
        m_vJumpTargetPos.y = FindGroundHeight(vCurrentPos.x);
        break;
        
    case CSparky::SPARKY_JUMP_TYPE::SMALL_FORWARD:
        // 작은 전진 점프 (커비 방향으로 1타일)
        m_vJumpTargetPos = vCurrentPos + m_vKirbyDirection * fTileSize;
        // 목표 지점의 바닥 높이 찾기
        m_vJumpTargetPos.y = FindGroundHeight(m_vJumpTargetPos.x);
        break;
        
    case CSparky::SPARKY_JUMP_TYPE::BIG_FORWARD:
        // 큰 전진 점프 (커비 방향으로 2타일)
        m_vJumpTargetPos = vCurrentPos + m_vKirbyDirection * (fTileSize * 2.0f);
        // 목표 지점의 바닥 높이 찾기
        m_vJumpTargetPos.y = FindGroundHeight(m_vJumpTargetPos.x);
        break;
    }
}

Vec2 CSparky::CalculateParabolicPosition(float progress)
{
    // 시작점과 목표점 사이의 선형 보간
    Vec2 vLerpPos = m_vJumpStartPos + (m_vJumpTargetPos - m_vJumpStartPos) * progress;
    
    // 포물선 높이 계산 (progress가 0.5일 때 최대 높이)
    float fMaxHeight = 0.f;
    switch (m_eJumpType)
    {
    case CSparky::SPARKY_JUMP_TYPE::SMALL_IN_PLACE:
    case CSparky::SPARKY_JUMP_TYPE::SMALL_FORWARD:
        fMaxHeight = GetTileSize() * 0.5f;  // 0.5타일 높이
        break;
    case CSparky::SPARKY_JUMP_TYPE::BIG_FORWARD:
        fMaxHeight = GetTileSize() * 1.2f;  // 1.2타일 높이
        break;
    }
    
    // 포물선 공식: height = 4 * maxHeight * progress * (1 - progress)
    float fParabolicHeight = 4.0f * fMaxHeight * progress * (1.0f - progress);
    
    return Vec2(vLerpPos.x, vLerpPos.y - fParabolicHeight);
}

void CSparky::UpdateKirbyDirection()
{
    // 현재 씬에서 플레이어 찾기
    CScene* pCurScene = CSceneMgr::GetInst()->GetCurScene();
    if (pCurScene)
    {
        const vector<CObject*>& vecPlayers = pCurScene->GetGroupObject(GROUP_TYPE::PLAYER);
        if (!vecPlayers.empty())
        {
            CObject* pPlayer = vecPlayers[0];  // 플레이어는 한 명뿐
            Vec2 vPlayerPos = pPlayer->GetPos();
            Vec2 vMyPos = GetPos();
            Vec2 vDirection = vPlayerPos - vMyPos;
            
            // 방향 정규화 (x축만 고려, y축은 점프로 처리)
            if (abs(vDirection.x) > 10.f)  // 최소 거리 이상일 때만
            {
                m_vKirbyDirection.x = vDirection.x > 0 ? 1.f : -1.f;
                m_vKirbyDirection.y = 0.f;
                
                // 스프라이트 방향도 업데이트
                m_iDir = (int)m_vKirbyDirection.x;
            }
        }
    }
}

void CSparky::UpdatePlayerPosition()
{
    // 플레이어 위치 업데이트 (CCopyMonster::CheckAttackCondition과 같은 로직)
    CScene* pCurScene = CSceneMgr::GetInst()->GetCurScene();
    if (pCurScene)
    {
        const vector<CObject*>& vecPlayer = pCurScene->GetGroupObject(GROUP_TYPE::PLAYER);
        if (!vecPlayer.empty() && vecPlayer[0])
        {
            CPlayer* pPlayer = dynamic_cast<CPlayer*>(vecPlayer[0]);
            if (pPlayer)
            {
                m_vPlayerPos = pPlayer->GetPos();
            }
        }
    }
}

void CSparky::CreateElectricField()
{
    // 스파키 몸 주변에 큰 정적 전기장 생성 (커비 SPARK 능력과 같은 방식)
    Vec2 vCenterPos = GetPos();
    
    CScene* pScene = CSceneMgr::GetInst()->GetCurScene();
    if (!pScene)
    {
        OutputDebugStringW(L"[Sparky] CreateElectricField failed - No scene\n");
        return;
    }
    
    OutputDebugStringW(L"[Sparky] Creating stationary electric field around body\n");
    
    // 방향 벡터는 0 (움직이지 않는 전기장)
    Vec2 vDirection = Vec2(0.f, 0.f);
    
    // 몸을 둘러싸는 큰 전기장 투사체 생성
    CProjectile* pElectricField = CProjectileFactory::CreateMonsterElectric(vCenterPos, vDirection, GROUP_TYPE::MONSTER);
    if (pElectricField)
    {
        // 속도 0으로 설정 (움직이지 않음)
        pElectricField->SetSpeed(0.f);
        
        // 크기를 2배로 설정 (가로세로 모두 2배)
        pElectricField->SetScale(Vec2(2.f, 2.f));
        
        // 씬에 투사체 추가
        pScene->AddObject(pElectricField, GROUP_TYPE::PROJ_MONSTER);
        
        OutputDebugStringW(L"[Sparky] Large electric field (2x scale) created successfully around body\n");
    }
    else
    {
        OutputDebugStringW(L"[Sparky] Failed to create electric field\n");
    }
}

void CSparky::CreateElectricSpark()
{
    // 지속적인 전기 스파크 생성 (공격 중 추가 효과)
    Vec2 vCenterPos = GetPos();
    
    CScene* pScene = CSceneMgr::GetInst()->GetCurScene();
    if (!pScene)
    {
        return;
    }
    
    // 랜덤한 방향으로 작은 스파크들 발사 (지속 효과)
    for (int i = 0; i < 3; ++i)
    {
        float fRandomAngle = (float)(rand() % 360) * 3.14159f / 180.0f;
        Vec2 vDirection = Vec2(cosf(fRandomAngle), sinf(fRandomAngle));
        
        // 랜덤한 거리에서 스파크 생성
        float fDistance = 30.f + (float)(rand() % 60);  // 30~90픽셀 거리
        Vec2 vSparkPos = vCenterPos + vDirection * fDistance;
        
        // 작은 전기 스파크 생성
        CProjectile* pSpark = CProjectileFactory::CreateMonsterElectric(vSparkPos, vDirection, GROUP_TYPE::MONSTER);
        if (pSpark)
        {
            pSpark->SetSpeed(80.f);   // 더 느린 속도
            
            // 씬에 투사체 추가 (매우 중요!)
            pScene->AddObject(pSpark, GROUP_TYPE::PROJ_MONSTER);
        }
    }
}

bool CSparky::CheckJumpCollision(const Vec2& targetPos)
{
    // 타일과의 충돌 검사
    CScene* pCurScene = CSceneMgr::GetInst()->GetCurScene();
    if (!pCurScene)
        return false;

    const vector<CObject*>& vecTiles = pCurScene->GetGroupObject(GROUP_TYPE::TILE);
    
    // 스파키의 콜라이더 크기 고려 (대략적으로 32x32 픽셀 가정)
    float fHalfWidth = 16.f;
    float fHalfHeight = 16.f;
    
    // 4개 모서리 지점에서 충돌 검사
    Vec2 checkPoints[4] = {
        Vec2(targetPos.x - fHalfWidth, targetPos.y - fHalfHeight),  // 좌상
        Vec2(targetPos.x + fHalfWidth, targetPos.y - fHalfHeight),  // 우상
        Vec2(targetPos.x - fHalfWidth, targetPos.y + fHalfHeight),  // 좌하
        Vec2(targetPos.x + fHalfWidth, targetPos.y + fHalfHeight)   // 우하
    };
    
    for (const Vec2& checkPoint : checkPoints)
    {
        for (CObject* pTileObj : vecTiles)
        {
            if (!pTileObj)
                continue;
                
            // 타일의 위치와 크기 확인
            Vec2 vTilePos = pTileObj->GetPos();
            Vec2 vTileScale = pTileObj->GetScale();
            
            // AABB 충돌 검사
            if (checkPoint.x >= vTilePos.x - vTileScale.x * 0.5f &&
                checkPoint.x <= vTilePos.x + vTileScale.x * 0.5f &&
                checkPoint.y >= vTilePos.y - vTileScale.y * 0.5f &&
                checkPoint.y <= vTilePos.y + vTileScale.y * 0.5f)
            {
                // 충돌 감지됨
                return true;
            }
        }
    }
    
    return false;
}

float CSparky::FindGroundHeight(float xPos)
{
    // 특정 X 위치에서 가장 높은 타일의 상단 높이 찾기
    CScene* pCurScene = CSceneMgr::GetInst()->GetCurScene();
    if (!pCurScene)
        return GetPos().y; // 씬이 없으면 현재 높이 반환

    const vector<CObject*>& vecTiles = pCurScene->GetGroupObject(GROUP_TYPE::TILE);
    
    float fHighestGroundY = 99999.f; // 매우 낮은 값으로 초기화 (높을수록 Y가 작음)
    bool bFoundGround = false;
    
    // 모든 타일을 검사해서 해당 X 위치와 겹치는 타일 중 가장 높은 것 찾기
    for (CObject* pTileObj : vecTiles)
    {
        if (!pTileObj)
            continue;
            
        Vec2 vTilePos = pTileObj->GetPos();
        Vec2 vTileScale = pTileObj->GetScale();
        
        // X축에서 겹치는지 확인
        float fTileLeft = vTilePos.x - vTileScale.x * 0.5f;
        float fTileRight = vTilePos.x + vTileScale.x * 0.5f;
        
        if (xPos >= fTileLeft && xPos <= fTileRight)
        {
            // 타일의 상단 높이 계산
            float fTileTop = vTilePos.y - vTileScale.y * 0.5f;
            
            // 가장 높은(Y가 작은) 타일 찾기
            if (fTileTop < fHighestGroundY)
            {
                fHighestGroundY = fTileTop;
                bFoundGround = true;
            }
        }
    }
    
    // 바닥을 찾았으면 그 높이에서 스파키 높이의 절반만큼 위로, 못 찾았으면 현재 높이 유지
    if (bFoundGround)
    {
        return fHighestGroundY - 16.f; // 스파키 높이의 절반(16픽셀) 만큼 위로
    }
    else
    {
        return GetPos().y; // 바닥이 없으면 현재 높이 유지
    }
}