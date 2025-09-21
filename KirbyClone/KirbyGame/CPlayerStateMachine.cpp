#include "gamePCH.h"
#include "CPlayerStateMachine.h"
#include "CPlayerInhaleSystem.h"
#include "CPlayerMovement.h"
#include "CPlayer.h"
#include "CAnimator.h"
#include "CRigidBody.h"
#include "CAnimation.h"
#include "CEventMgr.h"
#include "CTimeMgr.h"
#include "CPlayerInputManager.h"
#include "CPlayerStateTransitionTable.h"
#include "CPlayerHealthSystem.h"
#include "CProjectileFactory.h"
#include "CProjectile.h"
#include "CScene.h"
#include "CSceneMgr.h"
#include "CAbilityStar.h"
#include "CFadeEffect.h"
#include "CSoundMgr.h"
#include "CCamera.h"

CPlayerStateMachine::CPlayerStateMachine ( CPlayer* _pOwner )
    : m_pOwner ( _pOwner )
    , m_pInputManager ( nullptr )
    , m_pTransitionTable ( nullptr )
    , m_eCurState ( PLAYER_STATE::IDLE )
    , m_ePrevState ( PLAYER_STATE::END )
    , m_fFallTime ( 0.0f )
    , m_fFall0Duration ( 0.3f )
    , m_fFallStartY ( 0.0f )
    , m_fFallDistanceThreshold ( 224.0f )
    , m_fBounceHeight ( 0.6f )
    , m_bWasGrounded ( true )
    , m_fSlideTimer ( 0.0f )
    , m_fSlideDuration ( 0.8f )
    , m_fSlideDistance ( 256.f )
    , m_fSlideSpeed ( 320.f )
    , m_vSlideStartPos ( Vec2 ( 0.f , 0.f ) )
    , m_iSlideDirection ( 1 )
    , m_bSlideGroundCheck ( true )
    , m_bSlideKickCreated ( false )
    , m_fRecoilTimer ( 0.0f )
    , m_fRecoilDuration ( 0.3f )
    , m_vRecoilVelocity ( Vec2 ( 0.f , 0.f ) )
    , m_eHoverSubState ( HOVER_SUBSTATE::END )
    , m_fHoverUpForce ( 300.f )            // 상승력 조금 줄임
    , m_fHoverFallSpeed ( 80.f )           // 천천히 낙하하는 속도
    , m_fHoverMoveSpeed ( 120.f )          // 걷기 속도 정도
    , m_fHoverSubStateTimer ( 0.0f )
    , m_fHoverEnterDuration ( 0.6f )       // ENTER 지속 시간 늘림
    , m_fHoverFlyUpDuration ( 0.4f )       // FLY_UP 지속 시간 늘림
    , m_fHoverAirFriction ( 5.0f )         // 공중 마찰 계수
    , m_bHoverCanMoveOnGround ( true )     // 땅에서도 이동 가능
    , m_fDamageTimer ( 0.0f )              // 피격 타이머
    , m_fDamageDuration ( 0.5f )           // 0.5초 피격 상태
    , m_bDamageCompleted ( false )
    , m_bAbilityAcquisitionAttack ( false )
    , m_eInhaleCount ( INHALE_COUNT::NONE )
    , m_eCopyAbility ( COPY_ABILITY::NONE )
    , m_ePendingCopyAbility ( COPY_ABILITY::NONE )
    , m_fInhaleTimer ( 0.0f )
    , m_fInhaleDuration ( 1.0f )
    , m_fInhaleSuccessTimer ( 0.0f )
    , m_fInhaleSuccessDuration ( 0.3f )
    , m_fExhaleTimer ( 0.0f )
    , m_fExhaleDuration ( 0.5f )
    , m_fSwallowTimer ( 0.0f )
    , m_fSwallowDuration ( 0.3f )
    , m_fAttackTimer ( 0.0f )
    , m_fAttackDuration ( 0.5f )
    , m_pElectricField ( nullptr )
{
    // 새로운 시스템들 생성
    m_pInputManager = new CPlayerInputManager ( );
    m_pTransitionTable = new CPlayerStateTransitionTable ( );
}

CPlayerStateMachine::~CPlayerStateMachine ( )
{
    if ( m_pInputManager )
    {
        delete m_pInputManager;
        m_pInputManager = nullptr;
    }

    if ( m_pTransitionTable )
    {
        delete m_pTransitionTable;
        m_pTransitionTable = nullptr;
    }
}

void CPlayerStateMachine::Init ( )
{
    // Movement 시스템에 InputManager 연결
    if ( m_pOwner && m_pOwner->GetMovement ( ) && m_pInputManager )
    {
        m_pOwner->GetMovement ( )->SetInputManager ( m_pInputManager );
    }

    // 전환 테이블 초기화
    InitializeTransitionTable ( );

    // 초기 상태 설정
    ChangeStateInternal ( PLAYER_STATE::IDLE );
}

void CPlayerStateMachine::Update ( )
{
    CRigidBody* pRigidBody = m_pOwner ? m_pOwner->GetRigidBody ( ) : nullptr;
    if ( !m_pOwner || !pRigidBody || !m_pInputManager || !m_pTransitionTable )
        return;

    // === 승리 시퀀스 대기 중에는 입력 처리 건너뛰기 ===
    if (m_pOwner->IsVictorySequenceWaiting())
    {
        // 입력 없는 상태로 상태 전환 테이블만 사용
        CPlayerInputManager::InputFlags noInput = 0;
        
        // 상태 전환 체크 (입력 없음으로 자연스러운 전환만 허용)
        PLAYER_STATE nextState = m_pTransitionTable->GetNextState(m_eCurState, noInput, m_pOwner);
        
        // 상태 변경 (전환 테이블 결과만 사용)
        if (nextState != m_eCurState)
        {
            ChangeStateInternal(nextState);
        }
        
        // 현재 상태 실행
        ExecuteCurrentState();
        
        // 이전 프레임 Ground 상태 업데이트
        m_bWasGrounded = pRigidBody->IsGround();
        return;
    }

    // 1. 입력 수집
    m_pInputManager->Update ( );
    CPlayerInputManager::InputFlags currentInput = m_pInputManager->GetCurrentFrameInput ( );

    // 2. 상태 전환 체크
    PLAYER_STATE nextState = m_pTransitionTable->GetNextState ( m_eCurState , currentInput , m_pOwner );

    // 3. 상태 변경
    if ( nextState != m_eCurState )
    {
        ChangeStateInternal ( nextState );
    }

    // 4. 능력 버리기 처리 (모든 상태에서 가능)
    HandleDropAbility();

    // 5. 현재 상태 실행 (입력 처리 없음, 순수 실행만)
    ExecuteCurrentState ( );

    // 6. 이전 프레임 Ground 상태 업데이트
    m_bWasGrounded = pRigidBody->IsGround ( );
}

void CPlayerStateMachine::ChangeStateInternal ( PLAYER_STATE _eState )
{
    // 유효성 검사
    if ( !CanChangeToState ( _eState ) )
    {
        return;
    }

    // 이전 상태 종료 처리
    OnStateExit ( m_eCurState );

    // 실제 상태 변경
    m_ePrevState = m_eCurState;
    m_eCurState = _eState;

    OnStateEnter ( _eState );

    // 애니메이션 설정
    SetAnimationForState ( _eState );
}

void CPlayerStateMachine::ExecuteCurrentState ( )
{
    switch ( m_eCurState )
    {
    case PLAYER_STATE::IDLE:
        ExecuteIdleState ( );
        break;
    case PLAYER_STATE::WALK:
    case PLAYER_STATE::RUN:
    case PLAYER_STATE::MOUTHFUL_IDLE:
    case PLAYER_STATE::MOUTHFUL_WALK:
    case PLAYER_STATE::MOUTHFUL_RUN:
        ExecuteMovementState ( );
        break;
    case PLAYER_STATE::JUMP:
    case PLAYER_STATE::MOUTHFUL_JUMP:
        ExecuteJumpState ( );
        break;
    case PLAYER_STATE::FALL0:
    case PLAYER_STATE::FALL1:
    case PLAYER_STATE::FALL2:
    case PLAYER_STATE::MOUTHFUL_FALL:
        ExecuteFallState ( );
        break;
    case PLAYER_STATE::CROUCH:
        ExecuteCrouchState ( );
        break;
    case PLAYER_STATE::HOVER:
        ExecuteHoverState ( );
        break;
    case PLAYER_STATE::HOVER_EXHALE:
        m_fHoverSubStateTimer += CTimeMgr::GetInst ( )->GetfDT ( );
        ExecuteHoverExhaleState ( );
        break;
    case PLAYER_STATE::SLIDE:
        ExecuteSlideState ( );
        break;
    case PLAYER_STATE::SLIDE_KICK_RECOIL:
        ExecuteSlideKickRecoilState ( );
        break;
    case PLAYER_STATE::DAMAGE:
    case PLAYER_STATE::MOUTHFUL_DAMAGE:
        ExecuteDamageState ( );
        break;
    case PLAYER_STATE::INHALE:
        ExecuteInhaleState ( );
        break;
    case PLAYER_STATE::INHALE_KEEP:
        ExecuteInhaleKeepState ( );
        break;
    case PLAYER_STATE::INHALE_SUCCESS:
        ExecuteInhaleSuccessState ( );
        break;
    case PLAYER_STATE::EXHALE:
        ExecuteExhaleState ( );
        break;
    case PLAYER_STATE::SWALLOW:
        ExecuteSwallowState ( );
        break;
    case PLAYER_STATE::ATTACK:
        ExecuteAttackState ( );
        break;
    case PLAYER_STATE::ATTACK_HOLD:
        ExecuteAttackHoldState ( );
        break;
    case PLAYER_STATE::BOUNCE:
        ExecuteBounceState ( );
        break;
    case PLAYER_STATE::DOOR_ENTER:
        ExecuteDoorEnterState();
        break;
    case PLAYER_STATE::VICTORY_DANCE:
        ExecuteVictoryDanceState();
        break;
    }
}

void CPlayerStateMachine::ExecuteIdleState ( )
{
    // IDLE 상태에서는 특별한 처리 없음
    // 이동은 Movement 시스템에서, 상태 전환은 TransitionTable에서 처리
}

void CPlayerStateMachine::ExecuteMovementState ( )
{
    // 이동 처리는 CPlayerMovement에서 담당
    // 여기서는 상태 관련 물리만 처리
}

void CPlayerStateMachine::ExecuteJumpState ( )
{
    // 점프 높이 조절
    if ( m_pInputManager && m_pInputManager->IsJumpHold ( ) )
    {
        float jumpRatio = m_pInputManager->GetJumpHoldRatio ( );
        // 점프 높이 조절 로직 (필요시 구현)
    }
}

void CPlayerStateMachine::ExecuteFallState ( )
{
    // 낙하 시간 누적
    m_fFallTime += CTimeMgr::GetInst ( )->GetfDT ( );
}

void CPlayerStateMachine::ExecuteCrouchState ( )
{
    // 크라우치 상태에서 방향 전환 및 감속 처리
    if ( m_pOwner->GetMovement ( ) )
    {
        m_pOwner->GetMovement()->HandleCrouchDirectionInput();
    }
}

void CPlayerStateMachine::ExecuteSlideState ( )
{
    CRigidBody* pRigidBody = m_pOwner ? m_pOwner->GetRigidBody ( ) : nullptr;
    if ( !pRigidBody )
        return;

    // 슬라이드 타이머 업데이트
    m_fSlideTimer += CTimeMgr::GetInst ( )->GetfDT ( );

    // 슬라이드 킥 투사체 생성 (첫 프레임에만)
    UpdateSlideKickProjectile ( );

    // 슬라이드 이동 처리
    UpdateSlideMovement ( );

    // 슬라이드 완료 체크 (속도 정리만)
    CheckSlideCompletion ( );
}

void CPlayerStateMachine::ExecuteInhaleState ( )
{
    // 빨아들이기 상태 타이머 업데이트
    m_fInhaleTimer += CTimeMgr::GetInst ( )->GetfDT ( );

    // 상태 전환은 전환 테이블에서 처리하도록 변경
    // 여기서는 타이머 관리만 담당
}

void CPlayerStateMachine::ExecuteInhaleKeepState ( )
{
    // INHALE_KEEP 상태에서는 지속적으로 빨아들이기 진행
    // 상태 전환은 전환 테이블에서 처리
    // 여기서는 빨아들이기 시스템이 계속 작동하도록 보장만 함
    
    if ( m_pOwner && m_pOwner->GetInhaleSystem ( ) )
    {
        // 빨아들이기 시스템이 중단되었다면 다시 시작
        if ( !m_pOwner->GetInhaleSystem ( )->IsInhaling ( ) )
        {
            m_pOwner->GetInhaleSystem ( )->StartInhale ( );
        }
    }
}

void CPlayerStateMachine::ExecuteInhaleSuccessState ( )
{
    // 타이머 업데이트 (상태 전환 테이블에서 사용)
    m_fInhaleSuccessTimer += CTimeMgr::GetInst ( )->GetfDT ( );
    
    // 전환은 상태 전환 테이블에서 처리됨 (애니메이션 완료 또는 타이머 기반)
}

void CPlayerStateMachine::ExecuteExhaleState ( )
{
    // 내뱉기 상태 타이머 업데이트
    m_fExhaleTimer += CTimeMgr::GetInst ( )->GetfDT ( );

    // 내뱉기 시작 시 빨아들이기 시스템 상태 초기화 (한 번만 실행)
    if (m_fExhaleTimer <= CTimeMgr::GetInst()->GetfDT() && m_pOwner )
    {
        if ( m_pOwner->GetInhaleSystem())
        {
            CSoundMgr::GetInst ( )->PlaySFX ( L"kirby_exhale_star" );

            m_pOwner->GetInhaleSystem()->SpitOut(); // 내뱉기 실행
        }
    }

    // 전환은 전환 테이블에 맡김 - 애니메이션 완료 시점에 상태 변경됨
}

void CPlayerStateMachine::ExecuteSwallowState ( )
{
    // 삼키기 상태 타이머 업데이트
    m_fSwallowTimer += CTimeMgr::GetInst ( )->GetfDT ( );

    // 삼키기 시작 시 빨아들이기 시스템 상태 초기화 (한 번만 실행)
    if (m_fSwallowTimer <= CTimeMgr::GetInst()->GetfDT() && m_pOwner)
    {
        if ( m_pOwner->GetInhaleSystem())
        {
            m_pOwner->GetInhaleSystem()->ReleaseMouthful(); // 입에 물고 있는 상태 해제
        }
    }

    // 삼키기 상태 시간 초과시 종료
    if ( m_fSwallowTimer >= m_fSwallowDuration )
    {
        // 카피 능력 획득 및 애니메이션 변경
        if ( m_pOwner && m_ePendingCopyAbility != COPY_ABILITY::NONE )
        {
            // pending에서 실제 능력으로 적용
            m_eCopyAbility = m_ePendingCopyAbility;
            m_ePendingCopyAbility = COPY_ABILITY::NONE;
            
            // 카피 능력 획득 효과음
            OutputDebugStringA("CopyAbility acquired: Playing copy sound\n");
            CSoundMgr::GetInst()->PlaySFX(L"copy");
            
            // 능력별 애니메이션 파일 로드
            m_pOwner->LoadCopyAbilityAnimations ( m_eCopyAbility );

            // 디버그 메시지 출력
            const char* abilityName = "";
            switch ( m_eCopyAbility )
            {
            case COPY_ABILITY::FIRE: abilityName = "FIRE"; break;
            case COPY_ABILITY::BEAM: abilityName = "BEAM"; break;
            case COPY_ABILITY::SPARK: abilityName = "SPARK"; break;
            default: abilityName = "NONE"; break;
            }

            // 빨아들이기 카운트 초기화
            m_eInhaleCount = INHALE_COUNT::NONE;

            // ATTACK 상태로 전환 (능력 시연)
            ChangeStateInternal ( PLAYER_STATE::ATTACK );
        }
        else
        {
            // 능력이 없어도 상태 초기화 및 복귀
            m_eInhaleCount = INHALE_COUNT::NONE;
            m_ePendingCopyAbility = COPY_ABILITY::NONE; // pending도 초기화
            ChangeStateInternal ( PLAYER_STATE::IDLE );
        }
    }
}

void CPlayerStateMachine::ExecuteAttackState ( )
{
    // 공격 상태 타이머 업데이트
    m_fAttackTimer += CTimeMgr::GetInst ( )->GetfDT ( );
    
    // 능력 획득 연출 중이면 연출용 공격, 아니면 일반 공격
    if (m_bAbilityAcquisitionAttack)
    {
        ExecutePresentationAttack();
    }
    else
    {
        // 카피 능력에 따른 일반 공격 실행
        switch ( m_eCopyAbility )
        {
        case COPY_ABILITY::FIRE:
            // 파이어는 0.5초만 실행 (ATTACK_HOLD로 전환됨)
            if ( m_fAttackTimer <= 0.5f )
            {
                ExecuteFireAttack ( );
            }
            break;
        case COPY_ABILITY::BEAM:
            // BEAM은 기존 방식 유지
            ExecuteBeamAttack ( );
            break;
        case COPY_ABILITY::SPARK:
            // 스파크는 0.5초만 실행 (ATTACK_HOLD로 전환됨)
            if ( m_fAttackTimer <= 0.5f )
            {
                ExecuteSparkAttack ( );
            }
            break;
        default:
            // 기본 공격이나 오류 처리
            break;
        }
    }
    
    // 능력 획득 연출 중인 공격이 끝나가면 페이드인 시작 (한 번만 실행)
    if (m_bAbilityAcquisitionAttack && 
        m_fAttackTimer >= m_fAttackDuration - 0.2f && m_fAttackTimer < m_fAttackDuration - 0.1f)
    {
        CFadeEffect::GetInst()->ReleaseFadeHold(); // 암전 유지 해제
        CFadeEffect::GetInst()->StartFadeIn(FADE_COLOR::BLACK, 0.3f); // 페이드인 시작
    }
    
    // 공격 상태 완료 시 능력 획득 연출 플래그 리셋
    float fCompletionDuration = m_fAttackDuration;
    
    if (m_fAttackTimer >= fCompletionDuration)
    {
        m_bAbilityAcquisitionAttack = false;
    }
    
    // 공격 상태 시간 초과시 종료 (상태 전환은 전환 테이블에서 처리)
}

void CPlayerStateMachine::ExecuteAttackHoldState ( )
{
    // ATTACK_HOLD 상태: 파이어/스파크 지속 공격
    // X키가 떼어질 때까지 계속 공격 유지
    
    // 카피 능력에 따른 지속 공격 실행
    switch ( m_eCopyAbility )
    {
    case COPY_ABILITY::FIRE:
        ExecuteFireHoldAttack ( );
        break;
    case COPY_ABILITY::SPARK:
        ExecuteSparkHoldAttack ( );
        break;
    default:
        // 파이어/스파크가 아니면 IDLE로 복귀 (BEAM은 이미 ATTACK에서 처리됨)
        ChangeStateInternal ( PLAYER_STATE::IDLE );
        break;
    }
}

void CPlayerStateMachine::ExecuteFireAttack ( )
{
    // 파이어 능력: X키 홀드로 전방 불 공격
    if ( !m_pOwner || !m_pInputManager )
        return;
    
    // X키가 홀드되어 있는 동안 지속적으로 불 공격 (투사체 생성)
    // 연출 중일 때는 1초간 가상 홀드
    bool bShouldAttack = m_pInputManager->HasInput ( ( uint32_t ) CPlayerInputManager::INPUT_TYPE::ACTION_HOLD );
    if ( m_bAbilityAcquisitionAttack && m_fAttackTimer <= 1.0f )
    {
        bShouldAttack = true; // 연출 중 1초간 가상 홀드
    }
    
    // 사운드 관련 static 변수들 (함수 스코프 전체에서 공유)
    static bool bFireSoundStarted = false;
    static float fireSoundTimer = 0.0f;
    
    if ( bShouldAttack )
    {
        // 파이어 공격 시작 시 사운드 재생
        if (!bFireSoundStarted)
        {
            CSoundMgr::GetInst()->PlaySFX(L"kirby_fire");
            bFireSoundStarted = true;
            fireSoundTimer = 0.0f;
        }
        
        // 사운드 재생 시간 추적 (대략 1초마다 재시작)
        fireSoundTimer += CTimeMgr::GetInst()->GetfDT();
        if (fireSoundTimer >= 0.2f)
        {
            CSoundMgr::GetInst()->PlaySFX(L"kirby_fire");
            fireSoundTimer = 0.0f;
        }
        
        // 0.1초마다 화염구 발사
        static float fireballTimer = 0.0f;
        fireballTimer += CTimeMgr::GetInst ( )->GetfDT ( );
        
        if ( fireballTimer >= 0.1f )
        {
            // 현재 씬의 불덩이 개수 확인 (최대 5개 제한)
            CScene* pCurScene = CSceneMgr::GetInst()->GetCurScene();
            if (pCurScene)
            {
                const vector<CObject*>& projList = pCurScene->GetGroupObject(GROUP_TYPE::PROJ_PLAYER);
                int fireballCount = 0;
                
                // 현재 활성화된 KIRBY_FIRE 투사체 개수 세기
                for (CObject* pObj : projList)
                {
                    CProjectile* pProj = dynamic_cast<CProjectile*>(pObj);
                    if (pProj && pProj->GetProjectileType() == PROJECTILE_TYPE::KIRBY_FIRE && !pProj->IsDead())
                    {
                        fireballCount++;
                    }
                }
                
                // 5개 미만일 때만 새로운 불덩이 생성
                if (fireballCount < 5)
                {
                    Vec2 kirbyPos = m_pOwner->GetPos();
                    CPlayerMovement* pMovement = m_pOwner->GetMovement();
                    
                    // 커비가 바라보는 방향 확인
                    int direction = 1; // 1: 오른쪽, -1: 왼쪽
                    if (pMovement && !pMovement->IsFacingRight())
                    {
                        direction = -1;
                    }
                    
                    // 발사 위치 (커비 앞쪽)
                    Vec2 firePos = kirbyPos + Vec2(direction * 32.0f, 0.0f);
                    
                    // 발사 방향
                    Vec2 fireDirection = Vec2((float)direction, 0.0f);
                    
                    // 불덩이 투사체 생성
                    CProjectile* pFireball = CProjectileFactory::Create(
                        PROJECTILE_TYPE::KIRBY_FIRE, 
                        firePos, 
                        fireDirection, 
                        GROUP_TYPE::PLAYER
                    );
                    
                    if (pFireball)
                    {
                        // 씬에 추가
                        CREATE_OBJECT(pFireball, GROUP_TYPE::PROJ_PLAYER);
                    }
                }
            }
            
            fireballTimer = 0.0f;
        }
    }
    else
    {
        // 파이어 공격이 끝나면 사운드 중단
            CSoundMgr::GetInst()->StopSFX(L"kirby_fire");
            bFireSoundStarted = false;
        
    }
}

void CPlayerStateMachine::ExecuteBeamAttack ( )
{
    // 빔 능력: 전방 원뿔 범위를 위에서부터 쓸어내리는 공격
    if ( !m_pOwner )
        return;
    
    // 공격 시작 시 한 번만 실행 - 5개의 빔 투사체 생성
    if ( m_fAttackTimer <= CTimeMgr::GetInst ( )->GetfDT ( ) )
    {
        // 빔 공격 효과음 재생
        CSoundMgr::GetInst()->PlaySFX(L"kirby_beam");
        Vec2 kirbyPos = m_pOwner->GetPos();
        CPlayerMovement* pMovement = m_pOwner->GetMovement();
        
        // 커비가 바라보는 방향 확인 (기본값: 오른쪽)
        int direction = 1; // 1: 오른쪽, -1: 왼쪽
        if (pMovement && !pMovement->IsFacingRight())
        {
            direction = -1;
        }
        
        // 회전 중심점 (커비 앞쪽 1타일)
        Vec2 rotationCenter = kirbyPos + Vec2(direction * 64.0f, 0.0f); // 64픽셀 = 1타일
        
        // 6개의 빔 투사체 생성
        for (int i = 0; i < 6; ++i)
        {
            // 각 투사체의 반지름 (바깥쪽부터 안쪽으로) - 1.2배 확장
            float radius = (128.0f - (i * 24.0f)) * 1.2f; // 153.6, 124.8, 96, 67.2, 38.4, 9.6 픽셀
            
            // 시작 각도 (위쪽 75도부터)
            float startAngle = -75.0f * (3.14159f / 180.0f); // 라디안 변환
            
            // 끝 각도 (아래쪽 45도까지)
            float endAngle = 45.0f * (3.14159f / 180.0f);
            
            // 왼쪽을 보고 있으면 각도 반전
            if (direction == -1)
            {
                startAngle = 180.0f * (3.14159f / 180.0f) + 75.0f * (3.14159f / 180.0f);
                endAngle = 180.0f * (3.14159f / 180.0f) - 45.0f * (3.14159f / 180.0f);
            }
            
            // 시작 위치 계산
            Vec2 startPos = rotationCenter + Vec2(
                cos(startAngle) * radius,
                sin(startAngle) * radius
            );
            
            // 끝 위치 계산
            Vec2 endPos = rotationCenter + Vec2(
                cos(endAngle) * radius,
                sin(endAngle) * radius
            );
            
            // 회전 빔 투사체 생성 - 특수 파라미터 전달
            CProjectile* pBeam = CProjectileFactory::CreateRotatingBeam(
                rotationCenter,     // 회전 중심점
                radius,             // 회전 반지름
                startAngle,         // 시작 각도
                endAngle,           // 끝 각도
                0.6f,               // 회전 지속시간
                m_pOwner
            );
            
            if (pBeam)
            {
                CREATE_OBJECT(pBeam, GROUP_TYPE::PROJ_PLAYER);
            }
        }
    }
}

void CPlayerStateMachine::ExecuteSparkAttack ( )
{
    // 스파크 능력: 몸 주변으로 전기장을 생성하는 공격
    if ( !m_pOwner || !m_pInputManager )
        return;
    
    // X키가 홀드되어 있는 동안 전기장 유지
    // 연출 중일 때는 1초간 가상 홀드 (파이어와 동일)
    bool bShouldAttack = m_pInputManager->HasInput ( ( uint32_t ) CPlayerInputManager::INPUT_TYPE::ACTION_HOLD );
    if ( m_bAbilityAcquisitionAttack && m_fAttackTimer <= 1.0f )
    {
        bShouldAttack = true; // 연출 중 1초간 가상 홀드
    }
    
    // 사운드 관련 static 변수들 (함수 스코프 전체에서 공유)
    static bool bSparkSoundStarted = false;
    static float sparkSoundTimer = 0.0f;
    
    if ( bShouldAttack )
    {
        // 스파크 공격 시작 시 사운드 재생
        if (!bSparkSoundStarted)
        {
            CSoundMgr::GetInst()->PlaySFX(L"kirby_spark");
            bSparkSoundStarted = true;
            sparkSoundTimer = 0.0f;
        }
        
        // 사운드 재생 시간 추적 (대략 1초마다 재시작)
        sparkSoundTimer += CTimeMgr::GetInst()->GetfDT();
        if (sparkSoundTimer >= 0.1f)
        {
            CSoundMgr::GetInst()->PlaySFX(L"kirby_spark");
            sparkSoundTimer = 0.0f;
        }
        
        // 전기장이 없으면 생성
        if ( !m_pElectricField || m_pElectricField->IsDead() )
        {
            Vec2 kirbyPos = m_pOwner->GetPos();
            
            // 커비 주위 3x3 타일 크기의 전기장 생성
            CProjectile* pElectricField = CProjectileFactory::Create(
                PROJECTILE_TYPE::KIRBY_ELECTRIC_FIELD, 
                kirbyPos, 
                Vec2(0.0f, 0.0f),  // 이동하지 않음
                GROUP_TYPE::PLAYER
            );
            
            if (pElectricField)
            {
                // 씬에 추가
                CREATE_OBJECT(pElectricField, GROUP_TYPE::PROJ_PLAYER);
                m_pElectricField = pElectricField;
            }
        }
        else
        {
            // 전기장이 이미 있으면 커비 위치에 맞춰 위치 업데이트
            Vec2 kirbyPos = m_pOwner->GetPos();
            m_pElectricField->SetPos(kirbyPos);
        }
    }
    else
    {
        // X키를 떼면 전기장 즉시 삭제
        if ( m_pElectricField && !m_pElectricField->IsDead() )
        {
            m_pElectricField->SetDead();
            m_pElectricField = nullptr;
        }
        

            CSoundMgr::GetInst ( )->StopSFX ( L"kirby_spark" );
            bSparkSoundStarted = false;
        
    }
}

void CPlayerStateMachine::ExecuteFireHoldAttack ( )
{
    // 파이어 홀드 공격: 지속적으로 화염구 발사
    if ( !m_pOwner || !m_pInputManager )
        return;
    
    // 0.1초마다 화염구 발사
    static float fireballTimer = 0.0f;
    fireballTimer += CTimeMgr::GetInst ( )->GetfDT ( );
    
    if ( fireballTimer >= 0.1f )
    {
        // 현재 씬의 불덩이 개수 확인 (최대 5개 제한)
        CScene* pCurScene = CSceneMgr::GetInst()->GetCurScene();
        if (pCurScene)
        {
            const vector<CObject*>& projObjects = pCurScene->GetGroupObject(GROUP_TYPE::PROJ_PLAYER);
            int fireballCount = 0;
            
            for (CObject* pObj : projObjects)
            {
                CProjectile* pProj = dynamic_cast<CProjectile*>(pObj);
                if (pProj && pProj->GetProjectileType() == PROJECTILE_TYPE::KIRBY_FIRE && !pProj->IsDead())
                {
                    fireballCount++;
                }
            }
            
            // 5개 미만일 때만 새로운 화염구 생성
            if (fireballCount < 5)
            {
                Vec2 kirbyPos = m_pOwner->GetPos();
                CPlayerMovement* pMovement = m_pOwner->GetMovement();
                
                // 커비가 바라보는 방향 확인
                int direction = 1; // 1: 오른쪽, -1: 왼쪽
                if (pMovement && !pMovement->IsFacingRight())
                {
                    direction = -1;
                }
                
                // 발사 위치 (커비 앞쪽)
                Vec2 firePos = kirbyPos + Vec2(direction * 32.0f, 0.0f);
                
                // 발사 방향
                Vec2 fireDirection = Vec2((float)direction, 0.0f);
                
                CProjectile* pFireball = CProjectileFactory::Create(
                    PROJECTILE_TYPE::KIRBY_FIRE,
                    firePos,
                    fireDirection,
                    GROUP_TYPE::PLAYER
                );
                
                if (pFireball)
                {
                    CREATE_OBJECT(pFireball, GROUP_TYPE::PROJ_PLAYER);
                }
            }
        }
        
        fireballTimer = 0.0f;
    }
}

void CPlayerStateMachine::ExecuteSparkHoldAttack ( )
{
    // 스파크 홀드 공격: 전기장 유지
    if ( !m_pOwner || !m_pInputManager )
        return;
    
    // 전기장이 없으면 생성
    if ( !m_pElectricField || m_pElectricField->IsDead() )
    {
        Vec2 kirbyPos = m_pOwner->GetPos();
        
        // 커비 주위 3x3 타일 크기의 전기장 생성
        CProjectile* pElectricField = CProjectileFactory::Create(
            PROJECTILE_TYPE::KIRBY_ELECTRIC_FIELD, 
            kirbyPos, 
            Vec2(0.0f, 0.0f),  // 이동하지 않음
            GROUP_TYPE::PLAYER
        );
        
        if (pElectricField)
        {
            // 씬에 추가
            CREATE_OBJECT(pElectricField, GROUP_TYPE::PROJ_PLAYER);
            m_pElectricField = pElectricField;
        }
    }
    else
    {
        // 전기장이 이미 있으면 커비 위치에 맞춰 위치 업데이트
        Vec2 kirbyPos = m_pOwner->GetPos();
        m_pElectricField->SetPos(kirbyPos);
    }
}

void CPlayerStateMachine::ExecuteBounceState ( )
{
}

void CPlayerStateMachine::ExecuteHoverState ( )
{
    CRigidBody* pRigidBody = m_pOwner ? m_pOwner->GetRigidBody ( ) : nullptr;
    if ( !pRigidBody || !m_pInputManager )
        return;

    // 서브스테이트 타이머 업데이트
    m_fHoverSubStateTimer += CTimeMgr::GetInst ( )->GetfDT ( );

    // 현재 서브스테이트에 따른 처리
    switch ( m_eHoverSubState )
    {
    case HOVER_SUBSTATE::ENTER:
        UpdateHoverEnter ( );
        break;
    case HOVER_SUBSTATE::FLY_UP:
        UpdateHoverFlyUp ( );
        break;
    case HOVER_SUBSTATE::FLOAT:
        UpdateHoverFloat ( );
        break;
    case HOVER_SUBSTATE::GROUNDED:
        UpdateHoverGrounded ( );
        break;
    }

    // 공통 처리
    UpdateHoverMovement ( );
    UpdateHoverPhysics ( );
}

void CPlayerStateMachine::ExecuteHoverExhaleState ( )
{
    // 정확히 한 번만 투사체 생성 (상태 진입 첫 프레임에만)
    static bool bProjectileFired = false;
    static float lastTimer = -1.0f;

    // 타이머가 리셋되었을 때 (새로운 HOVER_EXHALE 상태 진입)
    if ( m_fHoverSubStateTimer < lastTimer )
    {
        bProjectileFired = false;
    }
    lastTimer = m_fHoverSubStateTimer;

    // 아직 발사하지 않았고, 타이머가 매우 작을 때만 발사
    if ( !bProjectileFired && m_fHoverSubStateTimer <= 0.02f )
    {
        // 플레이어 위치와 방향 가져오기
        Vec2 vPlayerPos = m_pOwner->GetPos ( );
        Vec2 vDirection;

        // 플레이어가 보고 있는 방향 확인 (CPlayerMovement의 IsFacingRight 사용)
        CPlayerMovement* pMovement = m_pOwner->GetMovement ( );
        if ( pMovement && !pMovement->IsFacingRight ( ) )
        {
            vDirection = Vec2 ( -1.f , 0.f );  // 왼쪽
            vPlayerPos.x -= 32.f;  // 투사체 시작 위치 조정
        }
        else
        {
            vDirection = Vec2 ( 1.f , 0.f );   // 오른쪽  
            vPlayerPos.x += 32.f;  // 투사체 시작 위치 조정
        }

        // 공기 투사체 생성
        CProjectile* pAirPuff = CProjectileFactory::CreateAirPuff (
            vPlayerPos ,
            vDirection ,
            GROUP_TYPE::PLAYER
        );

        if ( pAirPuff )
        {
            // 씬에 추가
            CREATE_OBJECT ( pAirPuff , GROUP_TYPE::PROJ_PLAYER );
            bProjectileFired = true;  // 발사했음을 표시
        }
    }
}

void CPlayerStateMachine::ExecuteDamageState ( )
{
    // 피격 타이머 업데이트
    m_fDamageTimer += CTimeMgr::GetInst ( )->GetfDT ( );

    // 피격 시간이 끝나면 완료 플래그 설정 (전환은 테이블에서)
    if ( m_fDamageTimer >= m_fDamageDuration )
    {
        m_bDamageCompleted = true;
    }
}

void CPlayerStateMachine::ExecuteSlideKickRecoilState ( )
{
    CRigidBody* pRigidBody = m_pOwner ? m_pOwner->GetRigidBody ( ) : nullptr;
    if ( !pRigidBody )
        return;

    // 반동 타이머 업데이트
    m_fRecoilTimer += CTimeMgr::GetInst ( )->GetfDT ( );

    // 반동 물리 처리 (감속 적용)
    float decayFactor = 1.0f - ( m_fRecoilTimer / m_fRecoilDuration );
    if ( decayFactor < 0.0f ) decayFactor = 0.0f;

    Vec2 currentVelocity = m_vRecoilVelocity * decayFactor;
    pRigidBody->SetVelocity ( currentVelocity );
}

void CPlayerStateMachine::OnStateExit ( PLAYER_STATE _eState )
{
    switch ( _eState )
    {
    case PLAYER_STATE::INHALE:
    case PLAYER_STATE::INHALE_KEEP:
        // INHALE 또는 INHALE_KEEP 상태 종료 시 빨아들이기 시스템 중지
        if ( m_pOwner && m_pOwner->GetInhaleSystem ( ) )
        {
            m_pOwner->GetInhaleSystem ( )->StopInhale ( );
        }
        break;
    case PLAYER_STATE::EXHALE:
        // EXHALE 상태 종료 시 빨아들이기 카운트 초기화
        m_eInhaleCount = INHALE_COUNT::NONE;
        m_eCopyAbility = COPY_ABILITY::NONE;
        break;
    }
}

void CPlayerStateMachine::OnStateEnter ( PLAYER_STATE _eState )
{
    switch ( _eState )
    {
    case PLAYER_STATE::IDLE:
        OnEnterIdleState ( );
        break;
    case PLAYER_STATE::JUMP:
        OnEnterJumpState ( );
        break;
    case PLAYER_STATE::WALK:
        OnEnterWalkState ( );
        break;
    case PLAYER_STATE::SLIDE:
        OnEnterSlideState ( );
        break;
    case PLAYER_STATE::SLIDE_KICK_RECOIL:
        OnEnterSlideKickRecoilState ( );
        break;
    case PLAYER_STATE::INHALE:
        OnEnterInhaleState ( );
        break;
    case PLAYER_STATE::INHALE_KEEP:
        OnEnterInhaleKeepState ( );
        break;
    case PLAYER_STATE::INHALE_SUCCESS:
        OnEnterInhaleSuccessState ( );
        break;
    case PLAYER_STATE::EXHALE:
        OnEnterExhaleState ( );
        break;
    case PLAYER_STATE::SWALLOW:
        OnEnterSwallowState ( );
        break;
    case PLAYER_STATE::ATTACK:
        OnEnterAttackState ( );
        break;
    case PLAYER_STATE::MOUTHFUL_IDLE:
        OnEnterMouthfulState ( );
        break;
    case PLAYER_STATE::RUN:
        OnEnterRunState();
        break;
    case PLAYER_STATE::MOUTHFUL_WALK:
    case PLAYER_STATE::MOUTHFUL_RUN:
        OnEnterRunState();
        break;
    case PLAYER_STATE::MOUTHFUL_FALL:
    case PLAYER_STATE::MOUTHFUL_DAMAGE:
        break;
    case PLAYER_STATE::MOUTHFUL_JUMP:
        OnEnterJumpState ( );
        break;
    case PLAYER_STATE::BOUNCE:
        OnEnterBounceState ( );
        break;
    case PLAYER_STATE::FALL0:
    case PLAYER_STATE::FALL1:
        OnEnterFallState ( );
        break;
    case PLAYER_STATE::HOVER:
        OnEnterHoverState ( );
        break;
    case PLAYER_STATE::HOVER_EXHALE:
        OnEnterHoverExhaleState ( );
        break;
    case PLAYER_STATE::DAMAGE:
        OnEnterDamageState ( );
        break;
    case PLAYER_STATE::VICTORY_DANCE:
        OnEnterVictoryDanceState ( );
        break;
    }
}

void CPlayerStateMachine::OnEnterJumpState ( )
{
    if ( m_pOwner && m_pOwner->GetMovement ( ) )
    {
        m_pOwner->GetMovement ( )->Jump ( );
    }
    
    // 점프 효과음
    OutputDebugStringA("OnEnterJumpState: Playing kirby_jump sound\n");
    CSoundMgr::GetInst()->PlaySFX(L"kirby_jump");
}

void CPlayerStateMachine::OnEnterIdleState ( )
{
    CSoundMgr::GetInst ( )->StopSFX ( L"kirby_spark" );
    CSoundMgr::GetInst ( )->StopSFX ( L"kirby_fire" );

    // IDLE 상태 진입 시 RUN 모드 해제 (확실하게)
    if ( m_pOwner && m_pOwner->GetMovement ( ) )
    {
        m_pOwner->GetMovement ( )->SetRunMode ( false );
    }
    
    // 더블탭 상태도 리셋 (RUN 후 IDLE로 올 때 더블탭 감속 플래그 제거)
    if ( m_pInputManager )
    {
        m_pInputManager->ResetDoubleTapState ( );
    }
}

void CPlayerStateMachine::OnEnterWalkState ( )
{
    // WALK 상태 진입 시 RUN 모드 해제
    if ( m_pOwner && m_pOwner->GetMovement ( ) )
    {
        m_pOwner->GetMovement ( )->SetRunMode ( false );
    }
}

void CPlayerStateMachine::OnEnterRunState ( )
{
    // RUN 효과음
    OutputDebugStringA("OnEnterRunState: Playing kirby_run sound\n");
    CSoundMgr::GetInst()->PlaySFX(L"kirby_run");
}

void CPlayerStateMachine::OnEnterSlideState ( )
{
    InitiateSlide ( );
    
    // 슬라이드 효과음
    CSoundMgr::GetInst()->PlaySFX(L"kirby_slide");
}

void CPlayerStateMachine::OnEnterSlideKickRecoilState ( )
{
    m_fRecoilTimer = 0.0f;

    // 반동 요청 플래그 클리어
    if ( m_pOwner )
    {
        m_pOwner->ClearSlideKickRecoilRequest ( );
    }

    // 반동 속도 설정 (반대방향 대각 위로)
    float recoilX = -m_iSlideDirection * 200.0f;
    float recoilY = -200.0f;

    m_vRecoilVelocity = Vec2 ( recoilX , recoilY );

    if ( m_pOwner && m_pOwner->GetRigidBody ( ) )
    {
        m_pOwner->GetRigidBody ( )->SetVelocity ( m_vRecoilVelocity );
        m_pOwner->GetRigidBody ( )->SetUseGravity ( true );
    }
}

void CPlayerStateMachine::OnEnterInhaleState ( )
{
    // 빨아들이기 타이머 초기화
    m_fInhaleTimer = 0.0f;

    // 빨아들이기 시스템 시작
    if ( m_pOwner && m_pOwner->GetInhaleSystem ( ) )
    {
        m_pOwner->GetInhaleSystem ( )->StartInhale ( );
    }
}

void CPlayerStateMachine::OnEnterInhaleKeepState ( )
{
    // INHALE_KEEP 상태 진입 시에는 별도의 초기화가 필요 없음
    // 이미 INHALE에서 빨아들이기 시스템이 시작되었으므로 지속만 하면 됨
    
    // 하지만 빨아들이기 시스템이 계속 작동하도록 보장
    if ( m_pOwner && m_pOwner->GetInhaleSystem ( ) )
    {
        // 빨아들이기 시스템이 중단되었다면 다시 시작
        if ( !m_pOwner->GetInhaleSystem ( )->IsInhaling ( ) )
        {
            m_pOwner->GetInhaleSystem ( )->StartInhale ( );
        }
    }
}

void CPlayerStateMachine::OnEnterInhaleSuccessState ( )
{
    // 빨아들이기 성공 타이머 초기화
    m_fInhaleSuccessTimer = 0.0f;
}

void CPlayerStateMachine::OnEnterExhaleState ( )
{
    // 내뱉기 타이머 초기화
    m_fExhaleTimer = 0.0f;

    // 투사체 생성 (빨아들인 것이 있을 경우에만)
    if ( m_pOwner && m_eInhaleCount == INHALE_COUNT::ONE )
    {
        // 플레이어 위치와 방향 가져오기
        Vec2 vPlayerPos = m_pOwner->GetPos ( );
        Vec2 vDirection;

        // 플레이어가 보고 있는 방향 확인
        CPlayerMovement* pMovement = m_pOwner->GetMovement ( );
        if ( pMovement && !pMovement->IsFacingRight ( ) )
        {
            vDirection = Vec2 ( -1.f , 0.f );  // 왼쪽
            vPlayerPos.x -= 32.f;  // 투사체 시작 위치 조정
        }
        else
        {
            vDirection = Vec2 ( 1.f , 0.f );   // 오른쪽  
            vPlayerPos.x += 32.f;  // 투사체 시작 위치 조정
        }

        // KIRBY_STAR 투사체 생성
        CProjectile* pStar = CProjectileFactory::CreateStar (
            vPlayerPos ,
            vDirection ,
            GROUP_TYPE::PLAYER
        );

        if ( pStar )
        {
            // 씬에 추가
            CREATE_OBJECT ( pStar , GROUP_TYPE::PROJ_PLAYER );
        }
    }
}

void CPlayerStateMachine::OnEnterSwallowState ( )
{
    // 삼키기 사운드 재생
    CSoundMgr::GetInst()->PlaySFX(L"kirby_swallow");
    
    // 삼키기 타이머 초기화
    m_fSwallowTimer = 0.0f;
    
    // 능력이 있는 경우에만 페이드아웃 시작
    if (m_ePendingCopyAbility != COPY_ABILITY::NONE)
    {
        CFadeEffect::GetInst()->StartFadeOut(FADE_COLOR::BLACK, 0.3f, 25); // 0.3초 페이드아웃, 알파 25
        m_bAbilityAcquisitionAttack = true; // 능력 획득 연출 공격 플래그 설정
    }
}

void CPlayerStateMachine::ExecutePresentationAttack()
{
    // 연출용 공격 실행 - private Execute 함수들을 내부에서 호출
    switch (m_eCopyAbility)
    {
    case COPY_ABILITY::FIRE:
        ExecuteFireAttack();
        break;
    case COPY_ABILITY::BEAM:
        ExecuteBeamAttack();
        break;
    case COPY_ABILITY::SPARK:
        ExecuteSparkAttack();
        break;
    default:
        break;
    }
}

void CPlayerStateMachine::OnEnterAttackState ( )
{
    // 공격 타이머 초기화
    m_fAttackTimer = 0.0f;
    
    // 능력 획득 연출 중인 경우 암전 유지 설정
    if (m_bAbilityAcquisitionAttack)
    {
        CFadeEffect::GetInst()->HoldCurrentFade();
    }
    
    // 능력별 초기화 처리 및 공격 시간 설정
    switch ( m_eCopyAbility )
    {
    case COPY_ABILITY::FIRE:
        // 파이어 공격 초기화
        m_fAttackDuration = 0.5f; // 기본 공격 시간
        break;
    case COPY_ABILITY::BEAM:
        // 빔 공격 초기화
        m_fAttackDuration = 0.7f; // 빔 공격은 더 긴 시간
        break;
    case COPY_ABILITY::SPARK:
        // 스파크 공격 초기화
        m_fAttackDuration = 0.5f; // 파이어와 같은 공격 시간
        break;
    default:
        m_fAttackDuration = 0.5f; // 기본 공격 시간
        break;
    }
}

void CPlayerStateMachine::OnEnterMouthfulState ( )
{
    // 입가득한 상태 진입 처리
}

void CPlayerStateMachine::OnEnterBounceState ( )
{
    PerformBounce ( );
    
    // 바운스 효과음 재생
    CSoundMgr::GetInst()->PlaySFX(L"kirby_bounce");
}

void CPlayerStateMachine::OnEnterFallState ( )
{
    // FALL0/FALL1 진입 시: 낙하 시작 높이 기록 + 타이머 초기화
    if ( m_pOwner )
    {
        m_fFallStartY = m_pOwner->GetPos ( ).y;
        m_fFallTime = 0.f;
    }
}

void CPlayerStateMachine::OnEnterHoverState ( )
{
    // HOVER 상태 진입 시 초기화
    m_eHoverSubState = HOVER_SUBSTATE::ENTER;
    ResetHoverSubStateTimer ( );

    // 중력 비활성화
    DisableGravityForHover ( );
    
    // HOVER 시작 효과음
    OutputDebugStringA("OnEnterHoverState: Playing kirby_hover sound\n");
    CSoundMgr::GetInst()->PlaySFX(L"kirby_hover");
}

void CPlayerStateMachine::OnEnterHoverExhaleState ( )
{
    // 타이머 초기화
    m_fHoverSubStateTimer = 0.0f;

    // HOVER 종료 시 중력 재활성화
    CRigidBody* pRigidBody = m_pOwner ? m_pOwner->GetRigidBody ( ) : nullptr;
    if ( pRigidBody )
    {
        pRigidBody->SetUseGravity ( true );
    }
    
    // 공기 내뱉기 효과음
    OutputDebugStringA("OnEnterHoverExhaleState: Playing exhale_air_puff sound\n");
    CSoundMgr::GetInst()->PlaySFX(L"exhale_air_puff");
}

void CPlayerStateMachine::OnEnterDamageState ( )
{
    m_fDamageTimer = 0.0f;
    m_bDamageCompleted = false;

    // 커비 데미지 효과음
    OutputDebugStringA("OnEnterDamageState: Playing damage sound\n");
    CSoundMgr::GetInst()->PlaySFX(L"damage");
    
    // 카메라 흔들림 효과 (0.3초간 15픽셀 강도)
    CCamera::GetInst()->CameraShake(0.3f, 15.f);

    // 피격 플래그 정리
    if ( m_pOwner )
    {
        m_pOwner->ClearDamageRequest ( );
    }
    
    // 카피 능력이 있는 상태에서 데미지를 받으면 능력별 생성
    if (m_eCopyAbility != COPY_ABILITY::NONE && m_pOwner)
    {
        COPY_ABILITY currentAbility = m_eCopyAbility;
        Vec2 kirbyPos = m_pOwner->GetPos();
        Vec2 starPos = kirbyPos + Vec2(0.f, -30.f);
        
        // 능력별 생성
        CAbilityStar* pAbilityStar = new CAbilityStar(currentAbility);
        pAbilityStar->SetPos(starPos);
        
        // 데미지 받을 때는 반대 방향으로 튕겨나감
        Vec2 initialVelocity = m_pOwner->IsFacingRight() ? 
            Vec2(-180.f, -480.f) : Vec2(180.f, -480.f);
        pAbilityStar->SetInitialVelocity(initialVelocity);
        
        CREATE_OBJECT(pAbilityStar, GROUP_TYPE::ITEM);
        
        // 커비의 능력 제거
        m_eCopyAbility = COPY_ABILITY::NONE;
        
        // 스파크 능력의 경우 전기장도 즉시 제거
        if (currentAbility == COPY_ABILITY::SPARK && m_pElectricField && !m_pElectricField->IsDead())
        {
            m_pElectricField->SetDead();
            m_pElectricField = nullptr;
        }
        
        // 기본 애니메이션으로 복구 (NONE 능력 상태)
        m_pOwner->LoadCopyAbilityAnimations(COPY_ABILITY::NONE);
        
        // 디버그 출력
        const char* abilityName = "";
        switch (currentAbility)
        {
        case COPY_ABILITY::FIRE: abilityName = "FIRE"; break;
        case COPY_ABILITY::BEAM: abilityName = "BEAM"; break;
        case COPY_ABILITY::SPARK: abilityName = "SPARK"; break;
        default: abilityName = "UNKNOWN"; break;
        }
    }
}

void CPlayerStateMachine::OnEnterVictoryDanceState ( )
{
    // 승리 춤 효과음
    OutputDebugStringA("OnEnterVictoryDanceState: Playing kirbydance_short sound\n");
    CSoundMgr::GetInst()->PlaySFX(L"kirbydance_short");
}

void CPlayerStateMachine::InitializeTransitionTable ( )
{
    if ( !m_pTransitionTable )
        return;

    // 기본 전환들 추가
    AddBasicMovementTransitions ( );
    AddJumpAndFallTransitions ( );
    AddCrouchAndSlideTransitions ( );
    AddInhaleTransitions ( );
    AddSpecialTransitions ( );
    AddHoverTransitions ( );
    AddDamageTransitions ( );
}

void CPlayerStateMachine::AddBasicMovementTransitions ( )
{
    using INPUT = CPlayerInputManager::INPUT_TYPE;
    
    // 공통 조건 함수들
    auto IsGrounded = [] ( CPlayer* p ) {
        return p->GetRigidBody ( ) && p->GetRigidBody ( )->IsGround ( );
    };
    
    auto IsStoppedCompletely = [] ( CPlayer* p ) {
        if ( !p->GetMovement ( ) ) return false;
        CPlayerInputManager* pInputMgr = p->GetStateMachine ( )->GetInputManager ( );
        if ( !pInputMgr ) return false;
        
        bool hasLeftInput = pInputMgr->IsMovingLeft ( );
        bool hasRightInput = pInputMgr->IsMovingRight ( );
        bool isMoving = p->GetMovement ( )->IsActuallyMoving ( );
        bool isDecelerating = p->GetMovement ( )->IsDecelerating ( );
        
        return !hasLeftInput && !hasRightInput && !isMoving && !isDecelerating;
    };

    // IDLE <-> WALK 전환
    m_pTransitionTable->AddTransition (
        PLAYER_STATE::IDLE ,
        ( uint32_t ) INPUT::MOVE_LEFT | ( uint32_t ) INPUT::MOVE_RIGHT ,
        PLAYER_STATE::WALK ,
        IsGrounded ,
        100
    );

    m_pTransitionTable->AddTransition (
        PLAYER_STATE::WALK ,
        0 ,
        PLAYER_STATE::IDLE ,
        IsStoppedCompletely ,
        50 ,
        ( uint32_t ) INPUT::MOVE_LEFT | ( uint32_t ) INPUT::MOVE_RIGHT
    );

    // 더블탭 RUN 전환
    m_pTransitionTable->AddTransition (
        PLAYER_STATE::WALK ,
        ( uint32_t ) INPUT::DOUBLE_TAP_LEFT | ( uint32_t ) INPUT::DOUBLE_TAP_RIGHT ,
        PLAYER_STATE::RUN ,
        IsGrounded ,
        150
    );

    // RUN -> WALK
    m_pTransitionTable->AddTransition (
        PLAYER_STATE::RUN ,
        ( uint32_t ) INPUT::MOVE_LEFT | ( uint32_t ) INPUT::MOVE_RIGHT ,
        PLAYER_STATE::WALK ,
        [ ] ( CPlayer* p ) {
            if ( !p->GetMovement ( ) ) return false;
            return !p->GetMovement ( )->IsRunMode ( );
        } ,
        80
    );

    // RUN -> IDLE
    m_pTransitionTable->AddTransition (
        PLAYER_STATE::RUN ,
        0 ,
        PLAYER_STATE::IDLE ,
        [ ] ( CPlayer* p ) {
            if ( !p->GetMovement ( ) ) return false;
            return !p->GetMovement ( )->IsActuallyMoving ( ) &&
                !p->GetMovement ( )->IsDecelerating ( );
        } ,
        60
    );
}

void CPlayerStateMachine::AddJumpAndFallTransitions ( )
{
    using INPUT = CPlayerInputManager::INPUT_TYPE;
    
    // 공통 조건 함수들
    auto IsGrounded = [] ( CPlayer* p ) {
        return p->GetRigidBody ( ) && p->GetRigidBody ( )->IsGround ( );
    };

    // 점프 시작
    std::vector<PLAYER_STATE> jumpableStates = {
        PLAYER_STATE::IDLE, PLAYER_STATE::WALK, PLAYER_STATE::RUN
    };

    for ( PLAYER_STATE state : jumpableStates )
    {
        m_pTransitionTable->AddTransition (
            state ,
            ( uint32_t ) INPUT::JUMP_TAP ,
            PLAYER_STATE::JUMP ,
            IsGrounded ,
            200
        );
    }

    // 점프 -> FALL0
    m_pTransitionTable->AddTransition (
        PLAYER_STATE::JUMP ,
        0 ,
        PLAYER_STATE::FALL0 ,
        [ this ] ( CPlayer* p ) {
            if ( !p->GetRigidBody ( ) ) return false;
            return p->GetRigidBody ( )->GetVelocity ( ).y > 50.f;
        } ,
        300
    );

    // FALL0 -> FALL1 (0.3초 후)
    m_pTransitionTable->AddTransition (
        PLAYER_STATE::FALL0 ,
        0 ,
        PLAYER_STATE::FALL1 ,
        [ this ] ( CPlayer* p ) {
            return m_fFallTime >= m_fFall0Duration;
        } ,
        350
    );

    // FALL0/FALL1 -> 착지
    std::vector<PLAYER_STATE> fallStates = { PLAYER_STATE::FALL0, PLAYER_STATE::FALL1 };
    for ( PLAYER_STATE state : fallStates )
    {
        m_pTransitionTable->AddTransition (
            state ,
            0 ,
            PLAYER_STATE::IDLE ,
            [ this ] ( CPlayer* p ) {
                if ( !p->GetRigidBody ( ) ) return false;
                return p->GetRigidBody ( )->IsGround ( ) && !m_bWasGrounded;
            } ,
            300
        );
    }

    // FALL0/FALL1 -> FALL2 (거리 기반)
    for ( PLAYER_STATE state : fallStates )
    {
        m_pTransitionTable->AddTransition (
            state ,
            0 ,
            PLAYER_STATE::FALL2 ,
            [ this ] ( CPlayer* p ) {
                if ( !p ) return false;
                float currentY = p->GetPos ( ).y;
                float fallDistance = currentY - m_fFallStartY;
                return fallDistance >= m_fFallDistanceThreshold;
            } ,
            330
        );
    }

    // FALL2 -> BOUNCE
    m_pTransitionTable->AddTransition (
        PLAYER_STATE::FALL2 ,
        0 ,
        PLAYER_STATE::BOUNCE ,
        [ ] ( CPlayer* p ) { return p->GetRigidBody ( ) && p->GetRigidBody ( )->IsGround ( ); } ,
        350
    );

    // 지상 -> 낙하
    std::vector<PLAYER_STATE> groundStates = {
        PLAYER_STATE::IDLE, PLAYER_STATE::WALK, PLAYER_STATE::RUN, PLAYER_STATE::CROUCH
    };

    for ( PLAYER_STATE state : groundStates )
    {
        m_pTransitionTable->AddTransition (
            state ,
            0 ,
            PLAYER_STATE::FALL1 ,
            [ ] ( CPlayer* p ) {
                CRigidBody* pRB = p->GetRigidBody ( );
                return pRB && !pRB->IsGround ( );
            } ,
            500
        );
    }
}

void CPlayerStateMachine::AddCrouchAndSlideTransitions ( )
{
    using INPUT = CPlayerInputManager::INPUT_TYPE;

    // 크라우치 진입
    m_pTransitionTable->AddTransition (
        PLAYER_STATE::IDLE ,
        ( uint32_t ) INPUT::MOVE_DOWN ,
        PLAYER_STATE::CROUCH ,
        [ ] ( CPlayer* p ) {
            return p->GetRigidBody ( ) && p->GetRigidBody ( )->IsGround ( ) &&
                !p->GetStateMachine ( )->IsMouthfulState ( );
        } ,
        150
    );

    // 크라우치 해제
    m_pTransitionTable->AddTransition (
        PLAYER_STATE::CROUCH ,
        0 ,
        PLAYER_STATE::IDLE ,
        [ this ] ( CPlayer* p ) {
            if ( !m_pInputManager ) return false;
            return !m_pInputManager->IsMovingDown ( );
        } ,
        100
    );

    // 슬라이드 시작
    m_pTransitionTable->AddTransition (
        PLAYER_STATE::CROUCH ,
        ( uint32_t ) INPUT::JUMP_TAP | ( uint32_t ) INPUT::ACTION_TAP ,
        PLAYER_STATE::SLIDE ,
        [ ] ( CPlayer* p ) { return p->GetRigidBody ( ) && p->GetRigidBody ( )->IsGround ( ); } ,
        250
    );

    // 슬라이드킥 반동 전환
    m_pTransitionTable->AddTransition (
        PLAYER_STATE::SLIDE ,
        0 ,
        PLAYER_STATE::SLIDE_KICK_RECOIL ,
        [ ] ( CPlayer* p ) {
            return p && p->IsSlideKickRecoilRequested ( );
        } ,
        500
    );

    // 반동 완료 -> FALL1
    m_pTransitionTable->AddTransition (
        PLAYER_STATE::SLIDE_KICK_RECOIL ,
        0 ,
        PLAYER_STATE::FALL1 ,
        [ this ] ( CPlayer* p ) {
            return m_fRecoilTimer >= m_fRecoilDuration ||
                ( p->GetRigidBody ( ) && p->GetRigidBody ( )->GetVelocity ( ).Length ( ) < 50.0f );
        } ,
        400
    );

    // 슬라이드 완료 전환들
    m_pTransitionTable->AddTransition (
        PLAYER_STATE::SLIDE ,
        0 ,
        PLAYER_STATE::CROUCH ,
        [ this ] ( CPlayer* p ) {
            if ( !IsSlideCompleted ( ) ) return false;
            return m_pInputManager && m_pInputManager->IsMovingDown ( );
        } ,
        280
    );

    m_pTransitionTable->AddTransition (
        PLAYER_STATE::SLIDE ,
        0 ,
        PLAYER_STATE::IDLE ,
        [ this ] ( CPlayer* p ) {
            if ( !IsSlideCompleted ( ) ) return false;
            return !m_pInputManager || !m_pInputManager->IsMovingDown ( );
        } ,
        270
    );

    // 슬라이드 -> 낙하
    m_pTransitionTable->AddTransition (
        PLAYER_STATE::SLIDE ,
        0 ,
        PLAYER_STATE::FALL1 ,
        [ this ] ( CPlayer* p ) {
            CRigidBody* pRB = p->GetRigidBody ( );
            return pRB && !pRB->IsGround ( ) && m_bSlideGroundCheck;
        } ,
        400
    );
}

void CPlayerStateMachine::AddInhaleTransitions ( )
{
    using INPUT = CPlayerInputManager::INPUT_TYPE;

    // 빨아들이기 가능한 상태들에서 X키 누르면 INHALE로 전환
    std::vector<PLAYER_STATE> inhalableStates = {
        PLAYER_STATE::IDLE, PLAYER_STATE::JUMP, 
        PLAYER_STATE::FALL0, PLAYER_STATE::FALL1, PLAYER_STATE::FALL2,
        PLAYER_STATE::BOUNCE
    };

    for ( PLAYER_STATE state : inhalableStates )
    {
        // 카피 능력이 있을 때: X키 홀드로 ATTACK 실행 (높은 우선순위)
        m_pTransitionTable->AddTransition (
            state ,
            ( uint32_t ) INPUT::ACTION_HOLD ,
            PLAYER_STATE::ATTACK ,
            [ ] ( CPlayer* p ) -> bool {
                if ( !p || !p->GetStateMachine ( ) )
                    return false;
                // 카피 능력이 있을 때만 ATTACK
                COPY_ABILITY currentAbility = p->GetStateMachine ( )->GetCopyAbility ( );
                bool hasAbility = currentAbility != COPY_ABILITY::NONE;
                
                return hasAbility;
            } ,
            250  // INHALE보다 높은 우선순위
        );
        
        // 카피 능력이 없을 때: X키 TAP/HOLD로 INHALE 실행
        m_pTransitionTable->AddTransition (
            state ,
            ( uint32_t ) INPUT::ACTION_TAP | ( uint32_t ) INPUT::ACTION_HOLD ,
            PLAYER_STATE::INHALE ,
            [ ] ( CPlayer* p ) -> bool {
                if ( !p || !p->GetStateMachine ( ) )
                    return false;
                // 카피 능력이 없을 때만 INHALE
                COPY_ABILITY currentAbility = p->GetStateMachine ( )->GetCopyAbility ( );
                bool noAbility = currentAbility == COPY_ABILITY::NONE;
                
                return noAbility;
            } ,
            200
        );
    }

    // INHALE -> INHALE_KEEP (X키를 홀드하고 일정 시간 후)
    m_pTransitionTable->AddTransition (
        PLAYER_STATE::INHALE ,
        ( uint32_t ) INPUT::ACTION_HOLD ,
        PLAYER_STATE::INHALE_KEEP ,
        [ this ] ( CPlayer* p ) {
            // 0.1초 이상 홀드하면 INHALE_KEEP으로 전환
            return m_fInhaleTimer >= 0.1f;
        } ,
        350  // 가장 높은 우선순위
    );

    // INHALE -> INHALE_SUCCESS
    m_pTransitionTable->AddTransition (
        PLAYER_STATE::INHALE ,
        0 ,
        PLAYER_STATE::INHALE_SUCCESS ,
        [ ] ( CPlayer* p ) {
            // TODO: 빨아들이기 시스템에서 성공 상태 체크
            // 임시로 StateMachine의 InhaleCount를 체크
            CPlayerStateMachine* pSM = p->GetStateMachine ( );
            return pSM && pSM->GetInhaleCount ( ) != INHALE_COUNT::NONE;
        } ,
        300
    );

    // INHALE_KEEP -> INHALE_SUCCESS
    m_pTransitionTable->AddTransition (
        PLAYER_STATE::INHALE_KEEP ,
        0 ,
        PLAYER_STATE::INHALE_SUCCESS ,
        [ ] ( CPlayer* p ) {
            // 빨아들이기 시스템에서 성공 상태 체크
            CPlayerStateMachine* pSM = p->GetStateMachine ( );
            return pSM && pSM->GetInhaleCount ( ) != INHALE_COUNT::NONE;
        } ,
        300
    );

    // INHALE_KEEP -> IDLE (X키를 떼면)
    m_pTransitionTable->AddTransition (
        PLAYER_STATE::INHALE_KEEP ,
        ( uint32_t ) INPUT::ACTION_AWAY ,
        PLAYER_STATE::IDLE ,
        [ ] ( CPlayer* p ) {
            return p->GetRigidBody ( ) && p->GetRigidBody ( )->IsGround ( );
        } ,
        250
    );

    // INHALE_KEEP -> FALL1 (공중에서 X키를 떼면)
    m_pTransitionTable->AddTransition (
        PLAYER_STATE::INHALE_KEEP ,
        ( uint32_t ) INPUT::ACTION_AWAY ,
        PLAYER_STATE::FALL1 ,
        [ ] ( CPlayer* p ) {
            return p->GetRigidBody ( ) && !p->GetRigidBody ( )->IsGround ( );
        } ,
        250
    );

    // INHALE -> 원래 상태로 복귀 (X키 떼면, 단 빨아들여지는 몬스터가 없을 때만)
    m_pTransitionTable->AddTransition (
        PLAYER_STATE::INHALE ,
        ( uint32_t ) INPUT::ACTION_AWAY ,
        PLAYER_STATE::IDLE ,
        [ ] ( CPlayer* p ) {
            // 땅에 있고, 빨아들여지는 중인 몬스터가 없을 때만
            return p->GetRigidBody ( ) && p->GetRigidBody ( )->IsGround ( ) &&
                   p->GetInhaleSystem ( ) && !p->GetInhaleSystem ( )->HasBeingInhaledMonsters ( );
        } ,
        250
    );

    m_pTransitionTable->AddTransition (
        PLAYER_STATE::INHALE ,
        ( uint32_t ) INPUT::ACTION_AWAY ,
        PLAYER_STATE::FALL1 ,
        [ ] ( CPlayer* p ) {
            // 공중에 있고, 빨아들여지는 중인 몬스터가 없을 때만
            return p->GetRigidBody ( ) && !p->GetRigidBody ( )->IsGround ( ) &&
                   p->GetInhaleSystem ( ) && !p->GetInhaleSystem ( )->HasBeingInhaledMonsters ( );
        } ,
        250
    );

    // INHALE_SUCCESS -> MOUTHFUL_IDLE (애니메이션 완료 시 자동 전환)
    m_pTransitionTable->AddTransition (
        PLAYER_STATE::INHALE_SUCCESS ,
        0 ,
        PLAYER_STATE::MOUTHFUL_IDLE ,
        [ this ] ( CPlayer* p ) {
            // 애니메이션이 끝났거나 시간 경과시 자동 전환
            CAnimator* pAnimator = p->GetAnimator();
            bool bAnimFinished = false;
            if (pAnimator && pAnimator->GetCurAnim())
            {
                bAnimFinished = pAnimator->GetCurAnim()->IsFinish();
            }
            bool bTimeElapsed = m_fInhaleSuccessTimer >= m_fInhaleSuccessDuration;
            return bAnimFinished || bTimeElapsed;
        } ,
        400
    );

    // MOUTHFUL_IDLE -> EXHALE
    m_pTransitionTable->AddTransition (
        PLAYER_STATE::MOUTHFUL_IDLE ,
        ( uint32_t ) INPUT::ACTION_TAP ,
        PLAYER_STATE::EXHALE ,
        nullptr ,
        300
    );

    // MOUTHFUL_IDLE -> SWALLOW (DOWN 키)
    m_pTransitionTable->AddTransition (
        PLAYER_STATE::MOUTHFUL_IDLE ,
        ( uint32_t ) INPUT::MOVE_DOWN ,
        PLAYER_STATE::SWALLOW ,
        nullptr ,
        350  // EXHALE보다 높은 우선순위
    );

    // MOUTHFUL_WALK -> EXHALE
    m_pTransitionTable->AddTransition (
        PLAYER_STATE::MOUTHFUL_WALK ,
        ( uint32_t ) INPUT::ACTION_TAP ,
        PLAYER_STATE::EXHALE ,
        nullptr ,
        300
    );

    // MOUTHFUL_RUN -> EXHALE
    m_pTransitionTable->AddTransition (
        PLAYER_STATE::MOUTHFUL_RUN ,
        ( uint32_t ) INPUT::ACTION_TAP ,
        PLAYER_STATE::EXHALE ,
        nullptr ,
        300
    );

    // MOUTHFUL_JUMP -> EXHALE
    m_pTransitionTable->AddTransition (
        PLAYER_STATE::MOUTHFUL_JUMP ,
        ( uint32_t ) INPUT::ACTION_TAP ,
        PLAYER_STATE::EXHALE ,
        nullptr ,
        300
    );

    // MOUTHFUL_FALL -> EXHALE
    m_pTransitionTable->AddTransition (
        PLAYER_STATE::MOUTHFUL_FALL ,
        ( uint32_t ) INPUT::ACTION_TAP ,
        PLAYER_STATE::EXHALE ,
        nullptr ,
        300
    );


    // SWALLOW -> IDLE
    m_pTransitionTable->AddTransition (
        PLAYER_STATE::SWALLOW ,
        0 ,
        PLAYER_STATE::IDLE ,
        [ ] ( CPlayer* p ) -> bool {
            if ( !p || !p->GetStateMachine ( ) )
                return false;
            
            // 삼키기 타이머 완료 시 IDLE로 전환
            return p->GetStateMachine ( )->m_fSwallowTimer >= p->GetStateMachine ( )->m_fSwallowDuration;
        } ,
        400
    );
}

void CPlayerStateMachine::AddSpecialTransitions ( )
{
    using INPUT = CPlayerInputManager::INPUT_TYPE;

    // 애니메이션 기반 전환들 (타이머 백업 조건 포함)
    m_pTransitionTable->AddTransition (
        PLAYER_STATE::EXHALE ,
        0 ,
        PLAYER_STATE::IDLE ,
        [ this ] ( CPlayer* p ) {
            // 현재 상태가 실제로 EXHALE인지 확인 (안전장치)
            if ( m_eCurState != PLAYER_STATE::EXHALE )
                return false;
                
            // 애니메이션 완료 조건
            CAnimator* pAnimator = p->GetAnimator ( );
            if ( pAnimator )
            {
                CAnimation* pCurAnim = pAnimator->GetCurAnim ( );
                if ( pCurAnim && pCurAnim->IsFinish ( ) )
                {
                    return true;
                }
            }
            
            // 타이머 백업 조건 (0.5초 후 강제 전환)
            return m_fExhaleTimer >= m_fExhaleDuration;
        } ,
        400
    );

    // ATTACK -> ATTACK_HOLD (파이어/스파크: 0.5초 후 X키 홀드 시)
    m_pTransitionTable->AddTransition (
        PLAYER_STATE::ATTACK ,
        ( uint32_t ) INPUT::ACTION_HOLD ,
        PLAYER_STATE::ATTACK_HOLD ,
        [ this ] ( CPlayer* p ) -> bool {
            if ( !p || !p->GetStateMachine ( ) )
                return false;
            
            // 파이어/스파크만 ATTACK_HOLD로 전환 가능, 0.5초 경과 시
            return ( m_eCopyAbility == COPY_ABILITY::FIRE || m_eCopyAbility == COPY_ABILITY::SPARK ) &&
                   m_fAttackTimer >= 0.5f;
        } ,
        450  // 높은 우선순위
    );

    // ATTACK -> IDLE (공격 완료 시 또는 X키를 뗄 때)
    m_pTransitionTable->AddTransition (
        PLAYER_STATE::ATTACK ,
        0 ,
        PLAYER_STATE::IDLE ,
        [ this ] ( CPlayer* p ) -> bool {
            if ( !p || !p->GetStateMachine ( ) )
                return false;
            
            // 공격 타이머 완료 시 IDLE로 전환
            bool bTimerExpired = m_fAttackTimer >= m_fAttackDuration;
            
            // 파이어/스파크의 경우 0.5초 후 X키를 떼면 IDLE로 전환
            bool bKeyReleased = false;
            if ( m_eCopyAbility == COPY_ABILITY::FIRE || m_eCopyAbility == COPY_ABILITY::SPARK )
            {
                bKeyReleased = ( m_fAttackTimer >= 0.5f ) && 
                               !m_pInputManager->HasInput ( ( uint32_t ) CPlayerInputManager::INPUT_TYPE::ACTION_HOLD );
            }
            
            return bTimerExpired || bKeyReleased;
        } ,
        400
    );

    // ATTACK_HOLD -> IDLE (X키를 뗄 때)
    m_pTransitionTable->AddTransition (
        PLAYER_STATE::ATTACK_HOLD ,
        0 ,
        PLAYER_STATE::IDLE ,
        [ this ] ( CPlayer* p ) -> bool {
            if ( !p || !p->GetStateMachine ( ) || !m_pInputManager )
                return false;
            
            // X키를 떼면 즉시 IDLE로 전환
            return !m_pInputManager->HasInput ( ( uint32_t ) CPlayerInputManager::INPUT_TYPE::ACTION_HOLD );
        } ,
        500  // 가장 높은 우선순위
    );

    m_pTransitionTable->AddTransition (
        PLAYER_STATE::SWALLOW ,
        0 ,
        PLAYER_STATE::MOUTHFUL_IDLE ,
        [ ] ( CPlayer* p ) {
            CAnimator* pAnimator = p->GetAnimator ( );
            if ( !pAnimator ) return false;
            CAnimation* pCurAnim = pAnimator->GetCurAnim ( );
            return pCurAnim && pCurAnim->IsFinish ( );
        } ,
        400
    );

    // BOUNCE -> FALL
    m_pTransitionTable->AddTransition (
        PLAYER_STATE::BOUNCE ,
        0 ,
        PLAYER_STATE::FALL1 ,
        [ this ] ( CPlayer* p ) {
            CRigidBody* pRB = p->GetRigidBody ( );
            if ( !pRB ) return false;
            Vec2 vVelocity = pRB->GetVelocity ( );
            bool isGrounded = pRB->IsGround ( );
            return !isGrounded && vVelocity.y >= 0.f;
        } ,
        350
    );

    // 입가득한 상태들 간의 전환
    m_pTransitionTable->AddTransition (
        PLAYER_STATE::MOUTHFUL_IDLE ,
        ( uint32_t ) INPUT::MOVE_LEFT | ( uint32_t ) INPUT::MOVE_RIGHT ,
        PLAYER_STATE::MOUTHFUL_WALK ,
        [ ] ( CPlayer* p ) {
            return p->GetRigidBody ( ) && p->GetRigidBody ( )->IsGround ( );
        } ,
        100
    );

    m_pTransitionTable->AddTransition (
        PLAYER_STATE::MOUTHFUL_WALK ,
        0 ,
        PLAYER_STATE::MOUTHFUL_IDLE ,
        [ ] ( CPlayer* p ) {
            if ( !p->GetMovement ( ) ) return false;
            CPlayerInputManager* pInputMgr = p->GetStateMachine ( )->GetInputManager ( );
            if ( !pInputMgr ) return false;
            
            bool hasLeftInput = pInputMgr->IsMovingLeft ( );
            bool hasRightInput = pInputMgr->IsMovingRight ( );
            bool isMoving = p->GetMovement ( )->IsActuallyMoving ( );
            bool isDecelerating = p->GetMovement ( )->IsDecelerating ( );
            
            return !hasLeftInput && !hasRightInput && !isMoving && !isDecelerating;
        } ,
        50 ,
        ( uint32_t ) INPUT::MOVE_LEFT | ( uint32_t ) INPUT::MOVE_RIGHT
    );

    // MOUTHFUL_IDLE -> MOUTHFUL_RUN (더블탭으로 바로 달리기)
    m_pTransitionTable->AddTransition (
        PLAYER_STATE::MOUTHFUL_IDLE ,
        ( uint32_t ) INPUT::DOUBLE_TAP_LEFT | ( uint32_t ) INPUT::DOUBLE_TAP_RIGHT ,
        PLAYER_STATE::MOUTHFUL_RUN ,
        [ ] ( CPlayer* p ) {
            return p->GetRigidBody ( ) && p->GetRigidBody ( )->IsGround ( );
        } ,
        150
    );

    // MOUTHFUL_WALK -> MOUTHFUL_RUN (더블탭으로 달리기)
    m_pTransitionTable->AddTransition (
        PLAYER_STATE::MOUTHFUL_WALK ,
        ( uint32_t ) INPUT::DOUBLE_TAP_LEFT | ( uint32_t ) INPUT::DOUBLE_TAP_RIGHT ,
        PLAYER_STATE::MOUTHFUL_RUN ,
        [ ] ( CPlayer* p ) {
            return p->GetRigidBody ( ) && p->GetRigidBody ( )->IsGround ( );
        } ,
        150
    );

    // MOUTHFUL_RUN -> MOUTHFUL_WALK (달리기 해제)
    m_pTransitionTable->AddTransition (
        PLAYER_STATE::MOUTHFUL_RUN ,
        ( uint32_t ) INPUT::MOVE_LEFT | ( uint32_t ) INPUT::MOVE_RIGHT ,
        PLAYER_STATE::MOUTHFUL_WALK ,
        [ ] ( CPlayer* p ) {
            if ( !p->GetMovement ( ) ) return false;
            return !p->GetMovement ( )->IsRunMode ( );
        } ,
        100
    );

    // MOUTHFUL_RUN -> MOUTHFUL_IDLE (정지)
    m_pTransitionTable->AddTransition (
        PLAYER_STATE::MOUTHFUL_RUN ,
        0 ,
        PLAYER_STATE::MOUTHFUL_IDLE ,
        [ ] ( CPlayer* p ) {
            if ( !p->GetMovement ( ) ) return false;
            return !p->GetMovement ( )->IsActuallyMoving ( ) &&
                !p->GetMovement ( )->IsDecelerating ( );
        } ,
        60
    );

    // MOUTHFUL 점프 전환들
    std::vector<PLAYER_STATE> mouthfulGroundStates = {
        PLAYER_STATE::MOUTHFUL_IDLE, PLAYER_STATE::MOUTHFUL_WALK, PLAYER_STATE::MOUTHFUL_RUN
    };

    for ( PLAYER_STATE state : mouthfulGroundStates )
    {
        m_pTransitionTable->AddTransition (
            state ,
            ( uint32_t ) INPUT::JUMP_TAP ,
            PLAYER_STATE::MOUTHFUL_JUMP ,
            [ ] ( CPlayer* p ) {
                return p->GetRigidBody ( ) && p->GetRigidBody ( )->IsGround ( );
            } ,
            200
        );
    }

    // MOUTHFUL_JUMP -> MOUTHFUL_FALL
    m_pTransitionTable->AddTransition (
        PLAYER_STATE::MOUTHFUL_JUMP ,
        0 ,
        PLAYER_STATE::MOUTHFUL_FALL ,
        [ ] ( CPlayer* p ) {
            CRigidBody* pRB = p->GetRigidBody ( );
            return pRB && !pRB->IsGround ( ) && pRB->GetVelocity ( ).y >= 0.f;
        } ,
        150
    );

    // MOUTHFUL_FALL -> MOUTHFUL_IDLE
    m_pTransitionTable->AddTransition (
        PLAYER_STATE::MOUTHFUL_FALL ,
        0 ,
        PLAYER_STATE::MOUTHFUL_IDLE ,
        [ ] ( CPlayer* p ) {
            return p->GetRigidBody ( ) && p->GetRigidBody ( )->IsGround ( );
        } ,
        180
    );
}

void CPlayerStateMachine::AddHoverTransitions ( )
{
    using INPUT = CPlayerInputManager::INPUT_TYPE;

    // HOVER 진입 전환
    std::vector<PLAYER_STATE> hoverableStates = {
        PLAYER_STATE::JUMP, PLAYER_STATE::FALL1, PLAYER_STATE::FALL2
    };

    for ( PLAYER_STATE state : hoverableStates )
    {
        m_pTransitionTable->AddTransition (
            state ,
            ( uint32_t ) INPUT::JUMP_TAP ,
            PLAYER_STATE::HOVER ,
            [ ] ( CPlayer* p ) {
                return p->GetRigidBody ( ) && !p->GetRigidBody ( )->IsGround ( );
            } ,
            250
        );
    }

    // HOVER -> HOVER_EXHALE
    m_pTransitionTable->AddTransition (
        PLAYER_STATE::HOVER ,
        ( uint32_t ) INPUT::ACTION_TAP ,
        PLAYER_STATE::HOVER_EXHALE ,
        nullptr ,
        400
    );

    // HOVER_EXHALE 종료 전환들
    m_pTransitionTable->AddTransition (
        PLAYER_STATE::HOVER_EXHALE ,
        0 ,
        PLAYER_STATE::IDLE ,
        [ ] ( CPlayer* p ) {
            CAnimator* pAnim = p->GetAnimator ( );
            CRigidBody* pRigidBody = p->GetRigidBody ( );

            if ( !pAnim || !pRigidBody ) return false;

            CAnimation* pCurAnim = pAnim->GetCurAnim ( );
            if ( !pCurAnim ) return false;

            bool animFinished = pCurAnim->IsFinish ( );
            bool lastFrameReached = ( pCurAnim->GetCurFrame ( ) >= pCurAnim->GetMaxFrame ( ) - 1 );
            bool isGrounded = pRigidBody->IsGround ( );

            return ( animFinished || lastFrameReached ) && isGrounded;
        } ,
        350
    );

    m_pTransitionTable->AddTransition (
        PLAYER_STATE::HOVER_EXHALE ,
        0 ,
        PLAYER_STATE::FALL1 ,
        [ ] ( CPlayer* p ) {
            CAnimator* pAnim = p->GetAnimator ( );
            CRigidBody* pRigidBody = p->GetRigidBody ( );

            if ( !pAnim || !pRigidBody ) return false;

            CAnimation* pCurAnim = pAnim->GetCurAnim ( );
            if ( !pCurAnim ) return false;

            bool animFinished = pCurAnim->IsFinish ( );
            bool lastFrameReached = ( pCurAnim->GetCurFrame ( ) >= pCurAnim->GetMaxFrame ( ) - 1 );
            bool isAirborne = !pRigidBody->IsGround ( );

            return ( animFinished || lastFrameReached ) && isAirborne;
        } ,
        340
    );

    // 백업 전환 (시간 기반)
    m_pTransitionTable->AddTransition (
        PLAYER_STATE::HOVER_EXHALE ,
        0 ,
        PLAYER_STATE::FALL1 ,
        [ this ] ( CPlayer* p ) {
            return m_fHoverSubStateTimer > 1.0f;
        } ,
        100
    );
}

void CPlayerStateMachine::AddDamageTransitions ( )
{
    using INPUT = CPlayerInputManager::INPUT_TYPE;

    // 피격 조건 정의
    auto damageCondition = [ ] ( CPlayer* p ) -> bool {
        if ( !p ) return false;
        if ( p->GetHealthSystem ( ) && p->GetHealthSystem ( )->IsGameOver ( ) )
            return false;
        return p->IsDamageRequested ( );
        };

    // 모든 상태에서 DAMAGE로의 전환
    std::vector<PLAYER_STATE> allStates = {
        PLAYER_STATE::IDLE, PLAYER_STATE::WALK, PLAYER_STATE::RUN,
        PLAYER_STATE::JUMP, PLAYER_STATE::FALL1, PLAYER_STATE::FALL2,
        PLAYER_STATE::CROUCH, PLAYER_STATE::SLIDE, PLAYER_STATE::HOVER,
        PLAYER_STATE::INHALE, PLAYER_STATE::INHALE_SUCCESS, PLAYER_STATE::EXHALE, PLAYER_STATE::SWALLOW
    };

    for ( PLAYER_STATE state : allStates )
    {
        m_pTransitionTable->AddTransition (
            state ,
            0 ,
            PLAYER_STATE::DAMAGE ,
            damageCondition ,
            2000
        );
    }

    // MOUTHFUL 상태들에서 MOUTHFUL_DAMAGE로의 전환
    std::vector<PLAYER_STATE> mouthfulStates = {
        PLAYER_STATE::MOUTHFUL_IDLE, PLAYER_STATE::MOUTHFUL_WALK, PLAYER_STATE::MOUTHFUL_RUN,
        PLAYER_STATE::MOUTHFUL_JUMP, PLAYER_STATE::MOUTHFUL_FALL
    };

    for ( PLAYER_STATE state : mouthfulStates )
    {
        m_pTransitionTable->AddTransition (
            state ,
            0 ,
            PLAYER_STATE::MOUTHFUL_DAMAGE ,
            damageCondition ,
            2000
        );
    }

    // DAMAGE 상태에서의 전환 (완료 플래그 기반)
    m_pTransitionTable->AddTransition (
        PLAYER_STATE::DAMAGE ,
        0 ,
        PLAYER_STATE::IDLE ,
        [ ] ( CPlayer* p ) -> bool {
            if ( !p || !p->GetRigidBody ( ) || !p->GetStateMachine ( ) )
                return false;
            return p->GetStateMachine ( )->IsDamageCompleted ( ) &&
                p->GetRigidBody ( )->IsGround ( );
        } ,
        1000
    );

    m_pTransitionTable->AddTransition (
        PLAYER_STATE::MOUTHFUL_DAMAGE ,
        0 ,
        PLAYER_STATE::MOUTHFUL_IDLE ,
        [ ] ( CPlayer* p ) -> bool {
            if ( !p || !p->GetRigidBody ( ) || !p->GetStateMachine ( ) )
                return false;
            return p->GetStateMachine ( )->IsDamageCompleted ( ) &&
                p->GetRigidBody ( )->IsGround ( );
        } ,
        1000
    );

    m_pTransitionTable->AddTransition (
        PLAYER_STATE::DAMAGE ,
        0 ,
        PLAYER_STATE::FALL1 ,
        [ ] ( CPlayer* p ) -> bool {
            if ( !p || !p->GetRigidBody ( ) || !p->GetStateMachine ( ) )
                return false;
            return p->GetStateMachine ( )->IsDamageCompleted ( ) &&
                !p->GetRigidBody ( )->IsGround ( );
        } ,
        1000
    );

    // 입력 무시 규칙들
    std::vector<uint32_t> ignoredInputs = {
        ( uint32_t ) INPUT::JUMP_TAP, ( uint32_t ) INPUT::ACTION_TAP,
        ( uint32_t ) INPUT::MOVE_LEFT, ( uint32_t ) INPUT::MOVE_RIGHT, ( uint32_t ) INPUT::MOVE_DOWN,
        ( uint32_t ) INPUT::DOUBLE_TAP_LEFT, ( uint32_t ) INPUT::DOUBLE_TAP_RIGHT
    };

    for ( uint32_t input : ignoredInputs )
    {
        m_pTransitionTable->AddTransition (
            PLAYER_STATE::DAMAGE ,
            input ,
            PLAYER_STATE::DAMAGE ,
            nullptr ,
            900
        );
    }
}

void CPlayerStateMachine::InitiateSlide ( )
{
    CRigidBody* pRigidBody = m_pOwner ? m_pOwner->GetRigidBody ( ) : nullptr;
    if ( !m_pOwner || !pRigidBody )
        return;

    m_fSlideTimer = 0.0f;
    m_vSlideStartPos = m_pOwner->GetPos ( );
    m_bSlideGroundCheck = true;
    m_bSlideKickCreated = false;

    CPlayerMovement* pMovement = m_pOwner->GetMovement ( );
    if ( pMovement )
    {
        m_iSlideDirection = pMovement->IsFacingRight ( ) ? 1 : -1;
    }
    else
    {
        m_iSlideDirection = 1;
    }

    pRigidBody->SetVelocityX ( m_fSlideSpeed * m_iSlideDirection );
}

void CPlayerStateMachine::UpdateSlideMovement ( )
{
    CRigidBody* pRigidBody = m_pOwner ? m_pOwner->GetRigidBody ( ) : nullptr;
    if ( !pRigidBody )
        return;

    float slideProgress = m_fSlideTimer / m_fSlideDuration;

    if ( slideProgress <= 1.0f )
    {
        float speedMultiplier = 1.0f - ( slideProgress * slideProgress );
        float currentSlideSpeed = m_fSlideSpeed * speedMultiplier;
        currentSlideSpeed = max ( currentSlideSpeed , m_fSlideSpeed * 0.3f );

        pRigidBody->SetVelocityX ( currentSlideSpeed * m_iSlideDirection );
    }
}

void CPlayerStateMachine::CheckSlideCompletion ( )
{
    CRigidBody* pRigidBody = m_pOwner ? m_pOwner->GetRigidBody ( ) : nullptr;
    if ( !pRigidBody ) return;

    if ( IsSlideCompleted ( ) )
    {
        pRigidBody->SetVelocityX ( 0.f );
    }
}

bool CPlayerStateMachine::IsSlideCompleted ( ) const
{
    if ( m_eCurState != PLAYER_STATE::SLIDE ) return false;

    CRigidBody* pRigidBody = m_pOwner ? m_pOwner->GetRigidBody ( ) : nullptr;
    if ( !pRigidBody ) return true;

    bool timeCompleted = ( m_fSlideTimer >= m_fSlideDuration );

    Vec2 currentPos = m_pOwner->GetPos ( );
    float distanceTraveled = abs ( currentPos.x - m_vSlideStartPos.x );
    bool distanceCompleted = ( distanceTraveled >= m_fSlideDistance );

    float currentSpeedX = abs ( pRigidBody->GetVelocity ( ).x );
    bool speedTooSlow = ( currentSpeedX < 50.f );

    return timeCompleted || distanceCompleted || speedTooSlow;
}

void CPlayerStateMachine::UpdateSlideKickProjectile ( )
{
    if ( m_bSlideKickCreated || !m_pOwner )
        return;

    if ( m_fSlideTimer <= 0.02f )
    {
        Vec2 vPlayerPos = m_pOwner->GetPos ( );
        Vec2 vDirection = Vec2 ( static_cast< float >( m_iSlideDirection ) , 0.f );

        Vec2 vKickPos = vPlayerPos;
        vKickPos.x += ( 28.f * m_iSlideDirection );
        vKickPos.y += 20.f;

        CProjectile* pSlideKick = CProjectileFactory::CreateSlideKick (
            vKickPos ,
            vDirection ,
            GROUP_TYPE::PLAYER
        );

        if ( pSlideKick )
        {
            CREATE_OBJECT ( pSlideKick , GROUP_TYPE::PROJ_PLAYER );
            m_bSlideKickCreated = true;
        }
    }
}

// 기존 함수들
bool CPlayerStateMachine::IsInhaleState ( ) const
{
    return ( m_eCurState == PLAYER_STATE::INHALE ||
        m_eCurState == PLAYER_STATE::INHALE_SUCCESS );
}

bool CPlayerStateMachine::IsMovingState ( ) const
{
    return ( m_eCurState == PLAYER_STATE::WALK ||
        m_eCurState == PLAYER_STATE::RUN ||
        m_eCurState == PLAYER_STATE::MOUTHFUL_WALK ||
        m_eCurState == PLAYER_STATE::MOUTHFUL_RUN );
}

bool CPlayerStateMachine::IsMouthfulState ( ) const
{
    return ( m_eCurState == PLAYER_STATE::MOUTHFUL_IDLE ||
        m_eCurState == PLAYER_STATE::MOUTHFUL_WALK ||
        m_eCurState == PLAYER_STATE::MOUTHFUL_RUN ||
        m_eCurState == PLAYER_STATE::MOUTHFUL_JUMP ||
        m_eCurState == PLAYER_STATE::MOUTHFUL_FALL ||
        m_eCurState == PLAYER_STATE::MOUTHFUL_DAMAGE );
}

bool CPlayerStateMachine::IsGroundedState ( ) const
{
    return ( m_eCurState != PLAYER_STATE::JUMP &&
        m_eCurState != PLAYER_STATE::FALL0 &&
        m_eCurState != PLAYER_STATE::FALL1 &&
        m_eCurState != PLAYER_STATE::FALL2 &&
        m_eCurState != PLAYER_STATE::BOUNCE &&
        m_eCurState != PLAYER_STATE::MOUTHFUL_JUMP &&
        m_eCurState != PLAYER_STATE::MOUTHFUL_FALL &&
        m_eCurState != PLAYER_STATE::HOVER &&
        m_eCurState != PLAYER_STATE::HOVER_EXHALE );
}

bool CPlayerStateMachine::IsHoverState ( ) const
{
    return ( m_eCurState == PLAYER_STATE::HOVER ||
        m_eCurState == PLAYER_STATE::HOVER_EXHALE );
}

bool CPlayerStateMachine::IsHoverGrounded ( ) const
{
    return ( m_eCurState == PLAYER_STATE::HOVER &&
        m_eHoverSubState == HOVER_SUBSTATE::GROUNDED );
}

bool CPlayerStateMachine::IsHoverFloating ( ) const
{
    return ( m_eCurState == PLAYER_STATE::HOVER &&
        ( m_eHoverSubState == HOVER_SUBSTATE::FLOAT ||
            m_eHoverSubState == HOVER_SUBSTATE::FLY_UP ) );
}

void CPlayerStateMachine::ForceStateChange ( PLAYER_STATE _eState )
{
    if ( IsValidStateTransition ( m_eCurState , _eState ) )
    {
        ChangeStateInternal ( _eState );
    }
}

void CPlayerStateMachine::ForceStateForSystemReset ( PLAYER_STATE _eState )
{
    if ( _eState == PLAYER_STATE::IDLE ||
        _eState == PLAYER_STATE::END )
    {
        ChangeStateInternal ( _eState );

        m_fFallTime = 0.0f;
        m_fFallStartY = 0.0f;
        m_fSlideTimer = 0.0f;
        m_fDamageTimer = 0.0f;
        m_bDamageCompleted = false;
    }
}

bool CPlayerStateMachine::CanChangeToState ( PLAYER_STATE _eState ) const
{
    return IsValidStateTransition ( m_eCurState , _eState );
}

void CPlayerStateMachine::PerformBounce ( )
{
    CRigidBody* pRigidBody = m_pOwner ? m_pOwner->GetRigidBody ( ) : nullptr;
    if ( !m_pOwner || !pRigidBody )
        return;

    CPlayerMovement* pMovement = m_pOwner->GetMovement ( );
    if ( pMovement )
    {
        float baseJumpPower = pMovement->GetJumpPower ( );
        float bounceJumpPower = baseJumpPower * m_fBounceHeight;

        pRigidBody->SetVelocityY ( -bounceJumpPower );
        pRigidBody->SetGround ( false );
    }
}

// HOVER 관련 함수들
void CPlayerStateMachine::UpdateHoverEnter ( )
{
    if ( m_fHoverSubStateTimer < 0.05f )
    {
        ApplyHoverUpForce ( );
    }

    if ( m_fHoverSubStateTimer >= m_fHoverEnterDuration )
    {
        ChangeHoverSubState ( HOVER_SUBSTATE::FLOAT );
    }
    else
    {
        CRigidBody* pRigidBody = m_pOwner->GetRigidBody ( );
        if ( pRigidBody )
        {
            Vec2 vVelocity = pRigidBody->GetVelocity ( );
            if ( vVelocity.y >= -10.f && m_fHoverSubStateTimer > 0.2f )
            {
                ChangeHoverSubState ( HOVER_SUBSTATE::FLOAT );
            }
        }
    }
}

void CPlayerStateMachine::UpdateHoverFlyUp ( )
{
    if ( m_fHoverSubStateTimer < 0.05f )
    {
        ApplyHoverUpForce ( );
    }

    if ( m_fHoverSubStateTimer >= m_fHoverFlyUpDuration )
    {
        ChangeHoverSubState ( HOVER_SUBSTATE::FLOAT );
    }
    else
    {
        CRigidBody* pRigidBody = m_pOwner->GetRigidBody ( );
        if ( pRigidBody )
        {
            Vec2 vVelocity = pRigidBody->GetVelocity ( );
            if ( vVelocity.y >= -10.f && m_fHoverSubStateTimer > 0.1f )
            {
                ChangeHoverSubState ( HOVER_SUBSTATE::FLOAT );
            }
        }
    }
}

void CPlayerStateMachine::UpdateHoverFloat ( )
{
    if ( m_pInputManager->IsJumpTap ( ) )
    {
        ChangeHoverSubState ( HOVER_SUBSTATE::FLY_UP );
    }

    CRigidBody* pRigidBody = m_pOwner->GetRigidBody ( );
    if ( pRigidBody && pRigidBody->IsGround ( ) )
    {
        ChangeHoverSubState ( HOVER_SUBSTATE::GROUNDED );
    }
}

void CPlayerStateMachine::UpdateHoverGrounded ( )
{
    if ( m_pInputManager->IsJumpTap ( ) )
    {
        CRigidBody* pRigidBody = m_pOwner->GetRigidBody ( );
        if ( pRigidBody )
        {
            pRigidBody->SetGround ( false );
        }
        ChangeHoverSubState ( HOVER_SUBSTATE::FLY_UP );
    }
}

void CPlayerStateMachine::UpdateHoverMovement ( )
{
    if ( !m_pInputManager ) return;

    CRigidBody* pRigidBody = m_pOwner ? m_pOwner->GetRigidBody ( ) : nullptr;
    if ( !pRigidBody ) return;

    // 땅에서 이동 불가 설정이면 GROUNDED 상태에서 이동 제한
    if ( !m_bHoverCanMoveOnGround && m_eHoverSubState == HOVER_SUBSTATE::GROUNDED )
        return;

    // 좌우 이동 입력 처리
    int horizontalInput = m_pInputManager->GetHorizontalInput ( );
    if ( horizontalInput != 0 )
    {
        float moveVelocity = m_fHoverMoveSpeed * horizontalInput;
        pRigidBody->SetVelocityX ( moveVelocity );

        // 방향 업데이트
        if ( m_pOwner->GetMovement ( ) )
        {
            m_pOwner->GetMovement ( )->SetFacingDirection ( horizontalInput > 0 );
        }
    }
    else
    {
        // 입력이 없으면 X축 속도 감속 (공중 마찰)
        Vec2 vVel = pRigidBody->GetVelocity ( );
        float deltaTime = CTimeMgr::GetInst ( )->GetfDT ( );
        float newVelX = vVel.x * ( 1.0f - m_fHoverAirFriction * deltaTime );

        if ( abs ( newVelX ) < 10.f ) newVelX = 0.f;
        pRigidBody->SetVelocityX ( newVelX );
    }
}

void CPlayerStateMachine::UpdateHoverPhysics ( )
{
    CRigidBody* pRigidBody = m_pOwner ? m_pOwner->GetRigidBody ( ) : nullptr;
    if ( !pRigidBody ) return;

    // 서브스테이트에 따른 물리 처리
    switch ( m_eHoverSubState )
    {
    case HOVER_SUBSTATE::ENTER:
        // ENTER 상태에서는 초기 상승 후 서서히 감속
    {
        Vec2 vVelocity = pRigidBody->GetVelocity ( );
        if ( vVelocity.y < 0 && m_fHoverSubStateTimer > 0.1f )
        {
            // 상승 속도를 서서히 감소시킴
            float deceleration = 600.f;
            float deltaTime = CTimeMgr::GetInst ( )->GetfDT ( );
            float newVelY = vVelocity.y + deceleration * deltaTime;

            if ( newVelY > 0 ) newVelY = 0;
            pRigidBody->SetVelocityY ( newVelY );
        }
    }
    break;

    case HOVER_SUBSTATE::FLY_UP:
        // FLY_UP 상태에서도 같은 처리
    {
        Vec2 vVelocity = pRigidBody->GetVelocity ( );
        if ( vVelocity.y < 0 && m_fHoverSubStateTimer > 0.1f )
        {
            float deceleration = 600.f;
            float deltaTime = CTimeMgr::GetInst ( )->GetfDT ( );
            float newVelY = vVelocity.y + deceleration * deltaTime;

            if ( newVelY > 0 ) newVelY = 0;
            pRigidBody->SetVelocityY ( newVelY );
        }
    }
    break;

    case HOVER_SUBSTATE::FLOAT:
        // FLOAT 상태에서는 천천히 낙하
        ApplyHoverFallSpeed ( );
        break;

    case HOVER_SUBSTATE::GROUNDED:
        // GROUNDED 상태에서는 낙하 없음 (땅에 붙어있음)
    {
        Vec2 vVelocity = pRigidBody->GetVelocity ( );
        if ( vVelocity.y > 0 )
        {
            pRigidBody->SetVelocityY ( 0 );
        }
    }
    break;
    }
}

void CPlayerStateMachine::ApplyHoverUpForce ( )
{
    CRigidBody* pRigidBody = m_pOwner ? m_pOwner->GetRigidBody ( ) : nullptr;
    if ( pRigidBody )
    {
        pRigidBody->SetVelocityY ( -m_fHoverUpForce );
    }
}

void CPlayerStateMachine::ApplyHoverFallSpeed ( )
{
    CRigidBody* pRigidBody = m_pOwner ? m_pOwner->GetRigidBody ( ) : nullptr;
    if ( !pRigidBody ) return;

    Vec2 vVelocity = pRigidBody->GetVelocity ( );

    // 현재 Y 속도가 HOVER 낙하 속도보다 작으면 낙하 속도로 설정
    if ( vVelocity.y < m_fHoverFallSpeed )
    {
        // 서서히 낙하 속도까지 증가
        float deltaTime = CTimeMgr::GetInst ( )->GetfDT ( );
        float acceleration = 200.f;
        float newVelY = vVelocity.y + acceleration * deltaTime;

        // 최대 낙하 속도 제한
        if ( newVelY > m_fHoverFallSpeed )
        {
            newVelY = m_fHoverFallSpeed;
        }

        pRigidBody->SetVelocityY ( newVelY );
    }
}

void CPlayerStateMachine::DisableGravityForHover ( )
{
    CRigidBody* pRigidBody = m_pOwner ? m_pOwner->GetRigidBody ( ) : nullptr;
    if ( pRigidBody )
    {
        pRigidBody->SetUseGravity ( false );
    }
}

void CPlayerStateMachine::RestoreGravityFromHover ( )
{
    CRigidBody* pRigidBody = m_pOwner ? m_pOwner->GetRigidBody ( ) : nullptr;
    if ( pRigidBody )
    {
        pRigidBody->SetUseGravity ( true );
    }
}

void CPlayerStateMachine::ChangeHoverSubState ( HOVER_SUBSTATE _eNewSubState )
{
    if ( m_eHoverSubState == _eNewSubState )
        return;

    m_eHoverSubState = _eNewSubState;
    ResetHoverSubStateTimer ( );

    // FLY_UP 상태로 변경될 때 떠오르는 사운드 재생
    if ( _eNewSubState == HOVER_SUBSTATE::FLY_UP )
    {
        OutputDebugStringA("ChangeHoverSubState: FLY_UP - Playing kirby_hover sound\n");
        CSoundMgr::GetInst()->PlaySFX(L"kirby_hover");
    }

    // 현재 HOVER 상태라면 애니메이션 재설정
    if ( m_eCurState == PLAYER_STATE::HOVER )
    {
        SetAnimationForState ( PLAYER_STATE::HOVER );
    }
}

void CPlayerStateMachine::SetAnimationForState ( PLAYER_STATE _eState )
{
    CAnimator* pAnimator = m_pOwner ? m_pOwner->GetAnimator ( ) : nullptr;
    if ( !pAnimator )
        return;

    switch ( _eState )
    {
    case PLAYER_STATE::IDLE:
        pAnimator->Play ( L"IDLE" , true );
        break;
    case PLAYER_STATE::WALK:
        pAnimator->Play ( L"WALK" , true );
        break;
    case PLAYER_STATE::RUN:
        pAnimator->Play ( L"RUN" , true );
        break;
    case PLAYER_STATE::CROUCH:
        pAnimator->Play ( L"CROUCH" , true );
        break;
    case PLAYER_STATE::SLIDE:
        pAnimator->Play ( L"SLIDE" , false );
        break;
    case PLAYER_STATE::SLIDE_KICK_RECOIL:
        pAnimator->Play ( L"FALL1" , true );  // FALL1과 동일한 애니메이션
        break;
    case PLAYER_STATE::JUMP:
        pAnimator->Play ( L"JUMP" , false );
        break;
    case PLAYER_STATE::FALL0:
        pAnimator->Play ( L"FALL0" , true );
        break;
    case PLAYER_STATE::FALL1:
        pAnimator->Play ( L"FALL1" , true );
        break;
    case PLAYER_STATE::FALL2:
        pAnimator->Play ( L"FALL2" , true );
        break;
    case PLAYER_STATE::BOUNCE:
        pAnimator->Play ( L"BOUNCE" , false );
        break;
    case PLAYER_STATE::HOVER_EXHALE:
        pAnimator->Play ( L"HOVER_EXHALE" , false );
        break;
    case PLAYER_STATE::HOVER:
        switch ( m_eHoverSubState )
        {
        case HOVER_SUBSTATE::ENTER:
            pAnimator->Play ( L"HOVER_ENTER" , false );
            break;
        case HOVER_SUBSTATE::FLY_UP:
            pAnimator->Play ( L"HOVER_FLY" , false );
            break;
        case HOVER_SUBSTATE::FLOAT:
            pAnimator->Play ( L"HOVER_FLOAT" , true );
            break;
        case HOVER_SUBSTATE::GROUNDED:
            pAnimator->Play ( L"HOVER_GROUND" , true );
            break;
        default:
            pAnimator->Play ( L"HOVER_FLOAT" , true );
            break;
        }
        break;
    case PLAYER_STATE::DAMAGE:
        pAnimator->Play ( L"DAMAGE" , false );
        break;
    case PLAYER_STATE::INHALE:
        pAnimator->Play ( L"INHALE" , true );
        break;
    case PLAYER_STATE::INHALE_KEEP:
        pAnimator->Play ( L"INHALE" , true );
        break;
    case PLAYER_STATE::INHALE_SUCCESS:
        pAnimator->Play ( L"INHALE_SUCCESS" , false );
        break;
    case PLAYER_STATE::EXHALE:
        pAnimator->Play ( L"EXHALE" , false );
        break;
    case PLAYER_STATE::SWALLOW:
        pAnimator->Play ( L"SWALLOW" , false );
        break;
    case PLAYER_STATE::MOUTHFUL_IDLE:
        pAnimator->Play ( L"MOUTHFUL_IDLE" , true );
        break;
    case PLAYER_STATE::MOUTHFUL_WALK:
        pAnimator->Play ( L"MOUTHFUL_WALK" , true );
        break;
    case PLAYER_STATE::MOUTHFUL_RUN:
        pAnimator->Play ( L"MOUTHFUL_RUN" , true );
        break;
    case PLAYER_STATE::MOUTHFUL_JUMP:
        pAnimator->Play ( L"MOUTHFUL_JUMP" , false );
        break;
    case PLAYER_STATE::MOUTHFUL_FALL:
        pAnimator->Play ( L"MOUTHFUL_FALL" , true );
        break;
    case PLAYER_STATE::MOUTHFUL_DAMAGE:
        pAnimator->Play ( L"MOUTHFUL_DAMAGE" , false );
        break;
    case PLAYER_STATE::ATTACK:
        // 카피 능력에 따른 공격 애니메이션
        switch ( m_eCopyAbility )
        {
        case COPY_ABILITY::FIRE:
        case COPY_ABILITY::BEAM:
        case COPY_ABILITY::SPARK:
        default:
            pAnimator->Play ( L"ATTACK" , false );      // 기본 공격
            break;
        }
        break;
    case PLAYER_STATE::ATTACK_HOLD:
        // 카피 능력에 따른 공격 애니메이션
        switch ( m_eCopyAbility )
        {
        case COPY_ABILITY::FIRE:
        case COPY_ABILITY::BEAM:
        case COPY_ABILITY::SPARK:
        default:
            pAnimator->Play ( L"ATTACK_HOLD" , true );
            break;
        }
        break;
    default:
        char buffer[ 256 ];
        sprintf_s ( buffer , "WARNING: No animation case for state: %d\n" , ( int ) _eState );
        OutputDebugStringA ( buffer );
        break;
    }
}

bool CPlayerStateMachine::IsValidStateTransition ( PLAYER_STATE _from , PLAYER_STATE _to ) const
{
    // 같은 상태로의 전환은 허용하지 않음
    if ( _from == _to )
        return false;

    // 특정 상태에서는 특정 상태로만 전환 가능
    switch ( _from )
    {
    case PLAYER_STATE::SWALLOW:
        return ( _to == PLAYER_STATE::IDLE || _to == PLAYER_STATE::ATTACK );

    case PLAYER_STATE::EXHALE:
        return ( _to == PLAYER_STATE::IDLE );

    case PLAYER_STATE::ATTACK:
        return ( _to == PLAYER_STATE::IDLE || _to == PLAYER_STATE::ATTACK_HOLD );

    case PLAYER_STATE::ATTACK_HOLD:
        return ( _to == PLAYER_STATE::IDLE );

    case PLAYER_STATE::SLIDE:
        return ( _to == PLAYER_STATE::FALL0 ||
            _to == PLAYER_STATE::FALL1 ||
            _to == PLAYER_STATE::CROUCH ||
            _to == PLAYER_STATE::IDLE ||
            _to == PLAYER_STATE::SLIDE_KICK_RECOIL );

    case PLAYER_STATE::HOVER:
        return ( _to == PLAYER_STATE::HOVER_EXHALE );

    case PLAYER_STATE::HOVER_EXHALE:
        // 모든 전환 허용 - 전환 테이블에서 조건 관리
        return true;
    }

    // 기본적으로 모든 전환 허용
    return true;
}

void CPlayerStateMachine::HandleDropAbility()
{
    // 능력이 없으면 처리하지 않음
    if (m_eCopyAbility == COPY_ABILITY::NONE || !m_pOwner || !m_pInputManager)
        return;
    
    // 데미지 상태에서는 능력 버리기 불가 (OnEnterDamageState에서 이미 처리됨)
    if (m_eCurState == PLAYER_STATE::DAMAGE)
        return;
    
    // 백스페이스 키 TAP 체크
    if (m_pInputManager->HasInput((uint32_t)CPlayerInputManager::INPUT_TYPE::DROP_ABILITY))
    {
        // 현재 능력 저장
        COPY_ABILITY currentAbility = m_eCopyAbility;
        
        // 능력별 생성 위치 (커비 위쪽)
        Vec2 kirbyPos = m_pOwner->GetPos();
        Vec2 starPos = kirbyPos + Vec2(0.f, -30.f);
        
        // 능력별 생성
        CAbilityStar* pAbilityStar = new CAbilityStar(currentAbility);
        pAbilityStar->SetPos(starPos);
        
        // 포물선 초기 속도 설정 (커비가 바라보는 반대방향으로)
        Vec2 initialVelocity = m_pOwner->IsFacingRight() ? 
            Vec2(-180.f, -480.f) : Vec2(180.f, -480.f);
        pAbilityStar->SetInitialVelocity(initialVelocity);
        
        // 씬에 추가
        CREATE_OBJECT(pAbilityStar, GROUP_TYPE::ITEM);
        
        // 커비의 능력 제거
        m_eCopyAbility = COPY_ABILITY::NONE;
        
        // 스파크 능력의 경우 전기장도 즉시 제거
        if (currentAbility == COPY_ABILITY::SPARK && m_pElectricField && !m_pElectricField->IsDead())
        {
            m_pElectricField->SetDead();
            m_pElectricField = nullptr;
        }
        
        // 기본 애니메이션으로 복구 (NONE 능력 상태)
        m_pOwner->LoadCopyAbilityAnimations(COPY_ABILITY::NONE);
        
        // 디버그 출력
        const char* abilityName = "";
        switch (currentAbility)
        {
        case COPY_ABILITY::FIRE: abilityName = "FIRE"; break;
        case COPY_ABILITY::BEAM: abilityName = "BEAM"; break;
        case COPY_ABILITY::SPARK: abilityName = "SPARK"; break;
        default: abilityName = "UNKNOWN"; break;
        }
    }
}

void CPlayerStateMachine::SetCopyAbility(COPY_ABILITY _eAbility)
{
    COPY_ABILITY oldAbility = m_eCopyAbility;
    m_eCopyAbility = _eAbility;
    
    // 애니메이션 로드 (상태 복원 시에도 필요)
    if (m_pOwner)
    {
        m_pOwner->LoadCopyAbilityAnimations(_eAbility);
    }
    
    // 디버깅: 능력 변경 로그
    const char* oldName = "";
    const char* newName = "";
    
    switch (oldAbility)
    {
    case COPY_ABILITY::NONE: oldName = "NONE"; break;
    case COPY_ABILITY::FIRE: oldName = "FIRE"; break;
    case COPY_ABILITY::BEAM: oldName = "BEAM"; break;
    case COPY_ABILITY::SPARK: oldName = "SPARK"; break;
    default: oldName = "UNKNOWN"; break;
    }
    
    switch (_eAbility)
    {
    case COPY_ABILITY::NONE: newName = "NONE"; break;
    case COPY_ABILITY::FIRE: newName = "FIRE"; break;
    case COPY_ABILITY::BEAM: newName = "BEAM"; break;
    case COPY_ABILITY::SPARK: newName = "SPARK"; break;
    default: newName = "UNKNOWN"; break;
    }
}

void CPlayerStateMachine::ExecuteDoorEnterState()
{
    // 상태 진입 시 한 번만 애니메이션 설정
    static PLAYER_STATE s_lastDoorState = PLAYER_STATE::END;
    if (s_lastDoorState != PLAYER_STATE::DOOR_ENTER)
    {
        s_lastDoorState = PLAYER_STATE::DOOR_ENTER;
        
        // DOOR_ENTER 애니메이션 재생
        if (CAnimator* pAnimator = m_pOwner->GetAnimator())
        {
            pAnimator->Play(L"DOOR_ENTER", false);  // 한 번만 재생
        }
    }
    
    // DOOR_ENTER 상태에서는 플레이어 움직임 정지
    if (CRigidBody* pRigidBody = m_pOwner->GetRigidBody())
    {
        pRigidBody->SetVelocityX(0.f);
        pRigidBody->SetVelocityY(0.f);
    }
}

void CPlayerStateMachine::ExecuteVictoryDanceState()
{
    OutputDebugString(L"[DEBUG] ExecuteVictoryDanceState() called\n");
    
    // 상태 진입 시 한 번만 애니메이션 설정
    static PLAYER_STATE s_lastVictoryState = PLAYER_STATE::END;
    static bool s_bFadeStarted = false;
    static bool s_bNeedsReset = false;
    
    // 게임 재시작 시 static 변수 초기화
    if (s_bNeedsReset)
    {
        OutputDebugString(L"[DEBUG] Resetting static variables\n");
        s_lastVictoryState = PLAYER_STATE::END;
        s_bFadeStarted = false;
        s_bNeedsReset = false;
    }
    
    if (s_lastVictoryState != PLAYER_STATE::VICTORY_DANCE && !s_bFadeStarted)
    {
        OutputDebugString(L"[DEBUG] Starting KIRBY_DANCE animation\n");
        s_lastVictoryState = PLAYER_STATE::VICTORY_DANCE;
        
        // VICTORY_DANCE 애니메이션 재생 (KIRBY_DANCE)
        if (CAnimator* pAnimator = m_pOwner->GetAnimator())
        {
            pAnimator->Play(L"KIRBY_DANCE", false);  // 한 번만 재생
            OutputDebugString(L"[DEBUG] KIRBY_DANCE animation started\n");
        }
        else
        {
            OutputDebugString(L"[DEBUG] ERROR: No Animator found!\n");
        }
    }
    
    // VICTORY_DANCE 상태에서는 플레이어 움직임 정지
    if (CRigidBody* pRigidBody = m_pOwner->GetRigidBody())
    {
        pRigidBody->SetVelocityX(0.f);
        pRigidBody->SetVelocityY(0.f);
    }
    
    // 애니메이션이 끝났는지 체크
    if (CAnimator* pAnimator = m_pOwner->GetAnimator())
    {
        if (CAnimation* pCurrentAnim = pAnimator->GetCurAnim())
        {
            // 현재 애니메이션 정보 출력
            wchar_t debugStr[300];
            swprintf_s(debugStr, L"[DEBUG] Current animation: %s, CurFrame: %d, MaxFrame: %d, FadeStarted: %s\n", 
                pCurrentAnim->GetName().c_str(),
                pCurrentAnim->GetCurFrame(),
                pCurrentAnim->GetMaxFrame(),
                s_bFadeStarted ? L"true" : L"false");
            OutputDebugString(debugStr);
            
            // 마지막 프레임에 도달했을 때 5초 페이드아웃 시작
            if (pCurrentAnim->GetCurFrame() >= pCurrentAnim->GetMaxFrame() - 1 && !s_bFadeStarted)
            {
                OutputDebugString(L"[DEBUG] Last frame reached! Starting 5-second fade out to StartScene\n");
                s_bFadeStarted = true;
                
                // 흰색 페이드 아웃 시작 (5초, 콜백 데이터 4 = 스타트 씬 이동)
                CFadeEffect::GetInst()->StartFadeOut(FADE_COLOR::WHITE, 5.0f, (DWORD_PTR)4);
                OutputDebugString(L"[DEBUG] 5-second fade out started with callback data 4\n");
            }
        }
        else
        {
            OutputDebugString(L"[DEBUG] ERROR: No current animation found!\n");
        }
    }
    else
    {
        OutputDebugString(L"[DEBUG] ERROR: No Animator found in animation check!\n");
    }
}