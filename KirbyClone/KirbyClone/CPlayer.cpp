#include "gamePCH.h"
#include "CPlayer.h"
#include "CAnimator.h"
#include "CAnimation.h"
#include "CTexture.h"
#include "CRigidBody.h"
#include "CCollider.h"
#include "CKeyMgr.h"
#include "CTimeMgr.h"
#include "CTile.h"
#include "CMonster.h"
#include "CAnimationDataMgr.h"
#include "CResMgr.h"
#include "CCore.h"
#include "CCamera.h"
#include "CEventMgr.h"

#include "CPlayerStateMachine.h"
#include "CPlayerInhaleSystem.h"
#include "CPlayerMovement.h"
#include "CPlayerHealthSystem.h"
#include "CPlayerCollisionSystem.h"
#include "CPlayerDataMgr.h"

CPlayer::CPlayer()
    : m_pStateMachine(nullptr)
    , m_pInhaleSystem(nullptr)
    , m_pMovement(nullptr)
    , m_pHealthSystem(nullptr)
    , m_pCollisionSystem(nullptr)
    , m_bDamageRequested(false)
    , m_vDamageKnockback(Vec2(0.f, 0.f))
    , m_bSlideKickRecoilRequested(false)
    , m_bGameOverSequence(false)
    , m_fGameOverTimer(0.f)
    , m_iGameOverPhase(0)
    , m_bBossDefeatWaiting(false)
    , m_bVictorySequenceWaiting(false)
{
    SetType(OBJECT_TYPE::PLAYER);

    // === 컴포넌트들 생성 ===
    CreateAnimator();
    CreateRigidBody();

    // 리지드바디 설정
    CRigidBody* pRigidBody = GetRigidBody();
    pRigidBody->SetMass(1.f);
    pRigidBody->SetMaxVelocity(640.f);
    pRigidBody->SetFriction(0.1f);
    pRigidBody->SetUseGravity(true);

    // 콜라이더 생성 및 설정
    CreateCollider();
    GetCollider()->SetOffsetPos(Vec2(0.f, 0.f));

    // === 충돌체 크기 설정 ===
    m_vNormalColliderScale = Vec2(56.f, 56.f);
    m_vCrouchColliderScale = Vec2(56.f, 28.f);
    GetCollider()->SetScale(m_vNormalColliderScale);

    // === 시스템들 생성 ===
    m_pStateMachine = new CPlayerStateMachine(this);
    m_pInhaleSystem = new CPlayerInhaleSystem(this);
    m_pMovement = new CPlayerMovement(this);
    m_pHealthSystem = new CPlayerHealthSystem(this);
    m_pCollisionSystem = new CPlayerCollisionSystem(this);

    // === 시스템들 초기화 ===
    m_pStateMachine->Init();
    m_pInhaleSystem->Init();
    m_pMovement->Init();
    m_pHealthSystem->Init();

    // === 애니메이션 생성 ===
    CreateAnimation();

    // === 기본 설정 ===
    SetPos(Vec2(640.f, 384.f));
    SetScale(Vec2(64.f, 64.f));
}

CPlayer::~CPlayer()
{
    if (m_pStateMachine)
    {
        delete m_pStateMachine;
        m_pStateMachine = nullptr;
    }

    if (m_pInhaleSystem)
    {
        delete m_pInhaleSystem;
        m_pInhaleSystem = nullptr;
    }

    if (m_pMovement)
    {
        delete m_pMovement;
        m_pMovement = nullptr;
    }

    if (m_pHealthSystem)
    {
        delete m_pHealthSystem;
        m_pHealthSystem = nullptr;
    }

    if (m_pCollisionSystem)
    {
        delete m_pCollisionSystem;
        m_pCollisionSystem = nullptr;
    }
}

void CPlayer::Update()
{
    // === 보스 격파 대기 중에는 업데이트 중지 ===
    if (m_bBossDefeatWaiting)
    {
        return;  // 입력 차단 및 업데이트 중지
    }
    
    // === 승리 시퀀스 대기 중에는 입력만 차단, StateMachine은 계속 ===
    if (m_bVictorySequenceWaiting)
    {
        // 물리, 애니메이션, StateMachine 업데이트 (입력 처리만 건너뛰기)
        GetRigidBody()->Update();
        GetAnimator()->Update();
        
        // StateMachine 업데이트 (상태 전환 허용)
        if (m_pStateMachine)
        {
            m_pStateMachine->Update();
        }
        return;
    }
    
    // === 게임 오버 시퀀스 처리 ===
    if (m_bGameOverSequence)
    {
        UpdateGameOverSequence();
        return;  // 게임 오버 중에는 다른 업데이트 중지
    }

    // === 체력 시스템 업데이트 ===
    if (m_pHealthSystem)
        m_pHealthSystem->Update();

    // 게임오버 상태라면 다른 업데이트 중지
    if (m_pHealthSystem && m_pHealthSystem->IsGameOver())
        return;

    // === 충돌체 크기 업데이트 ===
    UpdateColliderSize();

    // === Ground 상태 업데이트 ===
    if (m_pCollisionSystem)
        m_pCollisionSystem->UpdateGroundState();

    // === 상태 머신 업데이트 (입력 처리 + 상태 전환 + 상태 실행) ===
    if (m_pStateMachine)
        m_pStateMachine->Update();

    // === 이동 시스템 업데이트 (물리적 이동만) ===
    if (m_pMovement)
        m_pMovement->Update();

    // === 흡입 시스템 업데이트 ===
    if (m_pInhaleSystem)
        m_pInhaleSystem->Update();

    // === 리지드바디 업데이트 ===
    CRigidBody* pRigidBody = GetRigidBody();
    if (pRigidBody)
        pRigidBody->Update();

    // === 애니메이터 업데이트 ===
    CAnimator* pAnimator = GetAnimator();
    if (pAnimator)
        pAnimator->Update();

    // === 플레이어 위치를 카메라 경계 내로 제한 ===
    Vec2 vCurrentPos = GetPos();
    CCamera::GetInst()->ClampPositionToCameraBounds(vCurrentPos);
    SetPos(vCurrentPos);
}

void CPlayer::Render(HDC _dc)
{
    // 무적 상태에서는 깜빡거리는 렌더링
    if (m_pHealthSystem && m_pHealthSystem->ShouldRenderBlink())
    {
        RenderInvincible(_dc);
        return;
    }

    // 기본 렌더링
    CAnimator* pAnimator = GetAnimator();
    if (pAnimator)
    {
        // 커비는 기본적으로 오른쪽을 보므로, 왼쪽을 볼 때 플립
        bool shouldFlip = !IsFacingRight();
        pAnimator->SetFlipX(shouldFlip);
        pAnimator->Render(_dc);
    }
    else
    {
        // 애니메이터가 없으면 기본 오브젝트 렌더링
        CObject::Render(_dc);
    }

    // 충돌체 렌더링 추가
    if (GetCollider())
    {
        //GetCollider()->Render(_dc);  // 항상 보이는 초록색
        // 또는
        GetCollider()->RenderScaled(_dc, 1.0f);  // TAB키 필요
    }

    // 흡입 시스템 이펙트 렌더링
    if (m_pInhaleSystem)
    {
        m_pInhaleSystem->RenderInhaleEffect(_dc);
    }
}

void CPlayer::OnCollisionEnter(CCollider* _pOther)
{
    if (m_pCollisionSystem)
        m_pCollisionSystem->HandleCollisionEnter(_pOther);
}

void CPlayer::OnCollision(CCollider* _pOther)
{
    if (m_pCollisionSystem)
        m_pCollisionSystem->HandleCollision(_pOther);
}

void CPlayer::OnCollisionExit(CCollider* _pOther)
{
    if (m_pCollisionSystem)
        m_pCollisionSystem->HandleCollisionExit(_pOther);
}

// === 상태 관련 인터페이스 구현 ===
PLAYER_STATE CPlayer::GetCurrentState() const
{
    return m_pStateMachine ? m_pStateMachine->GetCurrentState() : PLAYER_STATE::IDLE;
}

PLAYER_STATE CPlayer::GetPreviousState() const
{
    return m_pStateMachine ? m_pStateMachine->GetPreviousState() : PLAYER_STATE::END;
}

// === 필수 래퍼 함수들 구현 ===

// 흡입 관련 필수 기능
bool CPlayer::IsInhaling() const
{
    return m_pInhaleSystem ? m_pInhaleSystem->IsInhaling() : false;
}

bool CPlayer::HasMouthful() const
{
    return m_pInhaleSystem ? m_pInhaleSystem->HasMouthful() : false;
}

void CPlayer::StartInhale()
{
    if (m_pInhaleSystem)
        m_pInhaleSystem->StartInhale();
}

void CPlayer::StopInhale()
{
    if (m_pInhaleSystem)
        m_pInhaleSystem->StopInhale();
}

void CPlayer::SpitOut()
{
    if (m_pInhaleSystem)
        m_pInhaleSystem->SpitOut();
}

// 이동 관련 필수 기능
bool CPlayer::IsFacingRight() const
{
    return m_pMovement ? m_pMovement->IsFacingRight() : true;
}

// 체력 관련 필수 기능
void CPlayer::TakeDamage(int _iDamage, Vec2 _vKnockbackDir)
{
    if (m_pHealthSystem)
        m_pHealthSystem->TakeDamage(_iDamage, _vKnockbackDir);
}

bool CPlayer::IsGameOver() const
{
    return m_pHealthSystem ? m_pHealthSystem->IsGameOver() : false;
}

bool CPlayer::ShouldRenderBlink() const
{
    return m_pHealthSystem ? m_pHealthSystem->ShouldRenderBlink() : false;
}

void CPlayer::RequestDamage(Vec2 _vKnockbackDir)
{
    m_bDamageRequested = true;
    m_vDamageKnockback = _vKnockbackDir;
}

void CPlayer::ClearDamageRequest()
{
    m_bDamageRequested = false;
    m_vDamageKnockback = Vec2(0.f, 0.f);
}

void CPlayer::RequestSlideKickRecoil()
{
    m_bSlideKickRecoilRequested = true;
}

void CPlayer::ClearSlideKickRecoilRequest()
{
    m_bSlideKickRecoilRequested = false;
}

// === 상태 저장/로드 ===
void CPlayer::LoadFromSavedData()
{
    // CPlayerDataMgr에서 저장된 데이터가 있는지 확인
    if (CPlayerDataMgr::GetInst()->HasSavedData())
    {
        // 저장된 상태 복원
        CPlayerDataMgr::GetInst()->LoadPlayerState(this);
        
        // 데이터 사용 완료 후 클리어
        CPlayerDataMgr::GetInst()->ClearSavedData();
    }
}

// === 렌더링 헬퍼 함수들 ===
void CPlayer::RenderInvincible(HDC _dc)
{
    // 무적 상태 확인
    bool bShouldRender = true;
    if (m_pHealthSystem && m_pHealthSystem->ShouldRenderBlink())
    {
        // 깜빡거리는 효과 - 일정 시간마다 렌더링 건너뛰기
        static float fBlinkTime = 0.f;
        fBlinkTime += CTimeMgr::GetInst()->GetfDT();

        if (fBlinkTime > 0.1f)  // 0.1초마다 토글
        {
            bShouldRender = !bShouldRender;
            fBlinkTime = 0.f;
        }
    }

    if (bShouldRender)
    {
        // 기본 렌더링
        CAnimator* pAnimator = GetAnimator();
        if (pAnimator)
        {
            pAnimator->Render(_dc);
        }
        else
        {
            CObject::Render(_dc);
        }
    }
}

// === 충돌체 크기 업데이트 함수 ===
void CPlayer::UpdateColliderSize()
{
    if (!GetCollider())
        return;

    PLAYER_STATE currentState = GetCurrentState();
    Vec2 targetScale = m_vNormalColliderScale;

    // 크라우치 상태에서는 충돌체 크기 축소
    if (currentState == PLAYER_STATE::CROUCH || currentState == PLAYER_STATE::SLIDE)
    {
        targetScale = m_vCrouchColliderScale;
    }

    // 현재 충돌체 크기와 다르면 업데이트
    Vec2 currentScale = GetCollider()->GetScale();
    if (currentScale != targetScale)
    {
        GetCollider()->SetScale(targetScale);

        // 크기 변경 시 위치 보정 (바닥에 맞춤)
        AdjustPositionForColliderResize(currentScale, targetScale);
    }
}

// === 충돌체 크기 변경 시 위치 보정 ===
void CPlayer::AdjustPositionForColliderResize(const Vec2& _vOldScale, const Vec2& _vNewScale)
{
    if (!GetRigidBody() || !GetRigidBody()->IsGround())
        return;

    // Y축 크기 변화량 계산
    float scaleDifference = _vOldScale.y - _vNewScale.y;

    if (abs(scaleDifference) > 0.1f)  // 유의미한 변화만 처리
    {
        // 크기가 작아지면 (크라우치) 아래로 이동
        // 크기가 커지면 (일반상태) 위로 이동
        Vec2 currentPos = GetPos();
        currentPos.y += scaleDifference * 0.5f;  // 절반만큼 이동 (중심점 기준)
        SetPos(currentPos);
    }
}

// === 애니메이션 생성 함수 ===
void CPlayer::CreateAnimation()
{
    // 애니메이션 매니저를 통한 파일 기반 로딩
    CAnimator* pAnimator = GetAnimator();
    if (!pAnimator)
        return;

    // 플레이어 애니메이션 파일 로드
    wstring animationFilePath = L"player_animations.json";
    CAnimationDataMgr::GetInst()->LoadAnimationsIntoAnimator(pAnimator, animationFilePath);

    // 기본 애니메이션 설정
    pAnimator->Play(L"IDLE", true);
}

void CPlayer::LoadCopyAbilityAnimations(COPY_ABILITY _eCopyAbility)
{
    CAnimator* pAnimator = GetAnimator();
    if (!pAnimator)
        return;

    // 능력별 애니메이션 파일 경로 결정
    wstring abilityAnimationFile;
    switch (_eCopyAbility)
    {
    case COPY_ABILITY::FIRE:
        abilityAnimationFile = L"player_fire_animations.json";
        break;
    case COPY_ABILITY::BEAM:
        abilityAnimationFile = L"player_beam_animations.json";
        break;
    case COPY_ABILITY::SPARK:
        abilityAnimationFile = L"player_spark_animations.json";
        break;
    case COPY_ABILITY::NONE:
        // 기본 애니메이션으로 복구
        abilityAnimationFile = L"player_animations.json";
        break;
    default:
        // 알 수 없는 능력은 기본 애니메이션 사용
        abilityAnimationFile = L"player_animations.json";
        break;
    }

    // 새 애니메이션 로드 (기존 애니메이션은 자동으로 교체됨)
    CAnimationDataMgr::GetInst()->LoadAnimationsIntoAnimator(pAnimator, abilityAnimationFile);
    
    // IDLE 애니메이션으로 시작
    pAnimator->Play(L"IDLE", true);
}

void CPlayer::StartGameOverSequence()
{
    m_bGameOverSequence = true;
    m_fGameOverTimer = 0.f;
    m_iGameOverPhase = 0;
    
    // GAMEOVER 애니메이션 시작
    if (GetAnimator())
    {
        GetAnimator()->Play(L"GAMEOVER", false);
    }
    
    // 물리 효과 초기화 (중력 비활성화)
    if (GetRigidBody())
    {
        GetRigidBody()->SetVelocity(Vec2(0.f, 0.f));
        GetRigidBody()->SetUseGravity(false);
    }
}

void CPlayer::UpdateGameOverSequence()
{
    float fDT = CTimeMgr::GetInst()->GetfDT();
    m_fGameOverTimer += fDT;
    
    Vec2 vPos = GetPos();
    
    switch (m_iGameOverPhase)
    {
    case 0: // 0.5초간 정지 (더 빠르게)
        if (m_fGameOverTimer >= 0.5f)
        {
            m_iGameOverPhase = 1;
            m_fGameOverTimer = 0.f;
        }
        break;
        
    case 1: // 위로 상승 (0.5초간, 더 빠르게)
        {
            float fUpSpeed = 800.f;  // 위쪽으로 800픽셀/초 (더 빠르게)
            vPos.y -= fUpSpeed * fDT;
            SetPos(vPos);
            
            if (m_fGameOverTimer >= 0.2f)
            {
                m_iGameOverPhase = 2;
                m_fGameOverTimer = 0.f;
                // 중력 다시 활성화
                if (GetRigidBody())
                {
                    GetRigidBody()->SetUseGravity(true);
                    GetRigidBody()->SetVelocity(Vec2(0.f, 0.f));
                }
            }
        }
        break;
        
    case 2: // 아래로 낙하
        // 중력으로 자연스럽게 낙하
        if (m_fGameOverTimer >= 1.0f)  // 1초 후 게임 오버 이벤트 발생 (더 빠르게)
        {
            // 게임 오버 이벤트 발생 (한 번만)
            tEvent gameOverEvent = {};
            gameOverEvent.eType = EVENT_TYPE::GAME_OVER;
            gameOverEvent.wParam = (DWORD_PTR)this;
            CEventMgr::GetInst()->AddEvent(gameOverEvent);
            
            m_iGameOverPhase = 3;  // 완료 상태로 변경하여 중복 실행 방지
        }
        break;
        
    case 3: // 완료 상태 (더 이상 이벤트 발생 안 함)
        // 아무것도 하지 않음 (대기 상태)
        break;
    }
    
    // 애니메이터는 계속 업데이트
    if (GetAnimator())
        GetAnimator()->Update();
}

void CPlayer::ResetGameOverSequence()
{
    m_bGameOverSequence = false;
    m_fGameOverTimer = 0.f;
    m_iGameOverPhase = 0;
    
    // 물리 효과 정상화
    if (GetRigidBody())
    {
        GetRigidBody()->SetUseGravity(true);
        GetRigidBody()->SetVelocity(Vec2(0.f, 0.f));
    }
}