#include "pch.h"
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
#include "CResMgr.h"
#include "CCore.h"
#include "CCamera.h"

#include "CPlayerStateMachine.h"
#include "CPlayerInhaleSystem.h"
#include "CPlayerMovement.h"
#include "CPlayerHealthSystem.h"
#include "CPlayerCollisionSystem.h"

CPlayer::CPlayer()
    : m_pStateMachine(nullptr)
    , m_pInhaleSystem(nullptr)
    , m_pMovement(nullptr)
    , m_pHealthSystem(nullptr)
    , m_pCollisionSystem(nullptr)
{
    SetType(OBJECT_TYPE::PLAYER);

    // === 컴포넌트들 생성 (기존 방식) ===
    CreateAnimator();
    CreateRigidBody();

    // 리지드바디 설정 (올바른 함수명 사용)
    CRigidBody* pRigidBody = GetRigidBody();
    pRigidBody->SetMass(1.f);
    pRigidBody->SetMaxVelocity(500.f);
    pRigidBody->SetFriction(0.1f);
    pRigidBody->SetUseGravity(true);

    // 콜라이더 생성 및 설정
    CreateCollider();
    GetCollider()->SetOffsetPos(Vec2(0.f, 0.f));
    GetCollider()->SetScale(Vec2(56.f, 56.f));

    // === 시스템들 생성 ===
    m_pStateMachine = new CPlayerStateMachine(this);
    m_pInhaleSystem = new CPlayerInhaleSystem(this);
    m_pMovement = new CPlayerMovement(this);
    m_pHealthSystem = new CPlayerHealthSystem(this);
    m_pCollisionSystem = new CPlayerCollisionSystem(this);

    // === 시스템들 초기화 ===
    m_pStateMachine->Init(GetAnimator(), GetRigidBody());
    m_pInhaleSystem->Init();
    m_pMovement->Init(GetRigidBody());
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
    // === 체력 시스템 업데이트 ===
    if (m_pHealthSystem)
        m_pHealthSystem->Update();

    // 게임오버 상태라면 다른 업데이트 중지
    if (m_pHealthSystem && m_pHealthSystem->IsGameOver())
        return;

    // === 흡입 관련 입력 처리 ===
    UpdateInhale();

    // === 이동 시스템 업데이트 ===
    if (m_pMovement)
        m_pMovement->Update();

    // === 흡입 시스템 업데이트 ===
    if (m_pInhaleSystem)
        m_pInhaleSystem->Update();

    // === 리지드바디 업데이트 ===
    CRigidBody* pRigidBody = GetRigidBody();
    if (pRigidBody)
        pRigidBody->Update();

    // === 상태 머신 업데이트 ===
    if (m_pStateMachine)
        m_pStateMachine->Update();

    // === 애니메이터 업데이트 ===
    CAnimator* pAnimator = GetAnimator();
    if (pAnimator)
        pAnimator->Update();
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
        pAnimator->Render(_dc);
    }
    else
    {
        // 애니메이터가 없으면 기본 오브젝트 렌더링
        CObject::Render(_dc);
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

void CPlayer::ChangeState(PLAYER_STATE _eState)
{
    if (m_pStateMachine)
        m_pStateMachine->ChangeState(_eState);
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

// === 업데이트 헬퍼 함수들 ===
void CPlayer::UpdateInhale()
{
    // 흡입 입력 처리
    if (KEY_HOLD(KEY::X))
    {
        if (!IsInhaling())
        {
            StartInhale();
        }
    }
    else
    {
        if (IsInhaling())
        {
            StopInhale();
        }
    }

    // 뱉기 입력 처리
    if (KEY_TAP(KEY::Z))
    {
        if (HasMouthful())
        {
            SpitOut();
        }
    }

    // 삼키기 입력 처리 (직접 시스템 접근)
    if (KEY_TAP(KEY::C))
    {
        if (m_pInhaleSystem && m_pInhaleSystem->HasMouthful())
        {
            CObject* pTarget = m_pInhaleSystem->GetMouthfulTarget();
            if (pTarget)
            {
                m_pInhaleSystem->SwallowTarget(pTarget);
            }
        }
    }
}

// === 애니메이션 생성 함수 ===
void CPlayer::CreateAnimation()
{
    // 커비 텍스처 로드
    CTexture* pKirbyTex = CResMgr::GetInst()->LoadTexture(L"KirbySprite", L"texture\\kirby\\kirby.bmp");

    // 기본 설정 - 32x32 픽셀, 시작 오프셋 8픽셀
    const int SPRITE_SIZE = 32;
    const int BORDER = 8;

    CAnimator* pAnimator = GetAnimator();

    // ===========================================
    // 1행: IDLE 상태 (1~2열, 깜빡임)
    // ===========================================
    CAnimation* pIdleAnim = new CAnimation();
    pIdleAnim->SetName(L"IDLE");
    pIdleAnim->SetTexture(pKirbyTex);
    pIdleAnim->SetLoop(true);

    // 깜빡임 시퀀스: 눈 뜨고 오래 대기 (1.5초) → 깜빡 (0.1초) → 눈 뜨고 짧은 대기 (1초) → 깜빡 (0.1초) → 눈 뜨고 짧은 대기 (0.2초) → 깜빡 (0.1초)

    // 1. 눈 뜨고 오래 대기 (1.5초)
    pIdleAnim->AddFrame(Vec2(BORDER, BORDER), Vec2(SPRITE_SIZE, SPRITE_SIZE), 1.5f);

    // 2. 첫 번째 깜빡임 (0.1초)
    pIdleAnim->AddFrame(Vec2(BORDER + SPRITE_SIZE, BORDER), Vec2(SPRITE_SIZE, SPRITE_SIZE), 0.1f);

    // 3. 눈 뜨고 짧은 대기 (1초)
    pIdleAnim->AddFrame(Vec2(BORDER, BORDER), Vec2(SPRITE_SIZE, SPRITE_SIZE), 1.0f);

    // 4. 두 번째 깜빡임 (0.1초)
    pIdleAnim->AddFrame(Vec2(BORDER + SPRITE_SIZE, BORDER), Vec2(SPRITE_SIZE, SPRITE_SIZE), 0.1f);

    // 5. 눈 뜨고 짧은 대기 (0.2초)
    pIdleAnim->AddFrame(Vec2(BORDER, BORDER), Vec2(SPRITE_SIZE, SPRITE_SIZE), 0.2f);

    // 6. 세 번째 깜빡임 (0.1초)
    pIdleAnim->AddFrame(Vec2(BORDER + SPRITE_SIZE, BORDER), Vec2(SPRITE_SIZE, SPRITE_SIZE), 0.1f);

    pAnimator->AddCustomAnimation(L"IDLE", pIdleAnim);

    // ===========================================
    // 2행: 납작 업드린 상태 (1~2열)
    // ===========================================
    pAnimator->CreateAnimation(L"CROUCH", pKirbyTex,
        Vec2(BORDER, BORDER + SPRITE_SIZE),
        Vec2(SPRITE_SIZE, SPRITE_SIZE),
        Vec2(SPRITE_SIZE, 0),
        0.2f, 2, true);

    // ===========================================
    // 3행: WALK 상태 (1~10열)
    // ===========================================
    pAnimator->CreateAnimation(L"WALK", pKirbyTex,
        Vec2(BORDER, BORDER + SPRITE_SIZE * 2),
        Vec2(SPRITE_SIZE, SPRITE_SIZE),
        Vec2(SPRITE_SIZE, 0),
        0.1f, 10, true);

    // ===========================================
    // 4행: RUN 상태 (1~8열) + 브레이크 (9열)
    // ===========================================
    pAnimator->CreateAnimation(L"RUN", pKirbyTex,
        Vec2(BORDER, BORDER + SPRITE_SIZE * 3),
        Vec2(SPRITE_SIZE, SPRITE_SIZE),
        Vec2(SPRITE_SIZE, 0),
        0.08f, 8, true);

    pAnimator->CreateAnimation(L"BRAKE", pKirbyTex,
        Vec2(BORDER + SPRITE_SIZE * 8, BORDER + SPRITE_SIZE * 3),
        Vec2(SPRITE_SIZE, SPRITE_SIZE),
        Vec2(SPRITE_SIZE, 0),
        0.1f, 1, false);

    // ===========================================
    // 5행: 점프 상태 (1~9열)
    // ===========================================
    pAnimator->CreateAnimation(L"JUMP", pKirbyTex,
        Vec2(BORDER, BORDER + SPRITE_SIZE * 4),
        Vec2(SPRITE_SIZE, SPRITE_SIZE),
        Vec2(SPRITE_SIZE, 0),
        0.1f, 9, false);

    // ===========================================
    // 6행: 낙하 상태 (1~5열) + 바운스 (6~13열)
    // ===========================================
    pAnimator->CreateAnimation(L"FALL", pKirbyTex,
        Vec2(BORDER, BORDER + SPRITE_SIZE * 5),
        Vec2(SPRITE_SIZE, SPRITE_SIZE),
        Vec2(SPRITE_SIZE, 0),
        0.1f, 5, true);

    pAnimator->CreateAnimation(L"BOUNCE", pKirbyTex,
        Vec2(BORDER + SPRITE_SIZE * 5, BORDER + SPRITE_SIZE * 5),
        Vec2(SPRITE_SIZE, SPRITE_SIZE),
        Vec2(SPRITE_SIZE, 0),
        0.08f, 8, false);

    // ===========================================
    // 7행: 공기 머금기 (1~5열) + 날아다니기 (6~11열)
    // ===========================================
    pAnimator->CreateAnimation(L"INFLATE", pKirbyTex,
        Vec2(BORDER, BORDER + SPRITE_SIZE * 6),
        Vec2(SPRITE_SIZE, SPRITE_SIZE),
        Vec2(SPRITE_SIZE, 0),
        0.1f, 5, false);

    pAnimator->CreateAnimation(L"FLY", pKirbyTex,
        Vec2(BORDER + SPRITE_SIZE * 5, BORDER + SPRITE_SIZE * 6),
        Vec2(SPRITE_SIZE, SPRITE_SIZE),
        Vec2(SPRITE_SIZE, 0),
        0.15f, 6, true);

    // ===========================================
    // 8행: 공기 내뱉기 (1~5열)
    // ===========================================
    pAnimator->CreateAnimation(L"DEFLATE", pKirbyTex,
        Vec2(BORDER, BORDER + SPRITE_SIZE * 7),
        Vec2(SPRITE_SIZE, SPRITE_SIZE),
        Vec2(SPRITE_SIZE, 0),
        0.08f, 5, false);

    // ===========================================
    // 9행: 게임오버 상태 (1~16열)
    // ===========================================
    pAnimator->CreateAnimation(L"GAME_OVER", pKirbyTex,
        Vec2(BORDER, BORDER + SPRITE_SIZE * 8),
        Vec2(SPRITE_SIZE, SPRITE_SIZE),
        Vec2(SPRITE_SIZE, 0),
        0.1f, 16, false);

    // ===========================================
    // 10행: 빨아들이기 상태 (1~8열)
    // ===========================================
    pAnimator->CreateAnimation(L"INHALE", pKirbyTex,
        Vec2(BORDER, BORDER + SPRITE_SIZE * 9),
        Vec2(SPRITE_SIZE, SPRITE_SIZE),
        Vec2(SPRITE_SIZE, 0),
        0.1f, 8, true);

    // ===========================================
    // 11행: 빨아들이기 공기 파티클 (1~8열)
    // ===========================================
    pAnimator->CreateAnimation(L"INHALE_EFFECT", pKirbyTex,
        Vec2(BORDER, BORDER + SPRITE_SIZE * 10),
        Vec2(SPRITE_SIZE, SPRITE_SIZE),
        Vec2(SPRITE_SIZE, 0),
        0.05f, 8, true);

    // ===========================================
    // 12~13행: 빨아들이기 성공 과정 (12행 1~5열 + 13행 1~4열)
    // ===========================================
    CAnimation* pInhaleSuccessAnim = new CAnimation();
    pInhaleSuccessAnim->SetName(L"INHALE_SUCCESS");
    pInhaleSuccessAnim->SetTexture(pKirbyTex);
    pInhaleSuccessAnim->SetLoop(false);

    // 12행 1~5열
    for (int i = 0; i < 5; ++i)
    {
        pInhaleSuccessAnim->AddFrame(
            Vec2(BORDER + i * SPRITE_SIZE, BORDER + SPRITE_SIZE * 11),
            Vec2(SPRITE_SIZE, SPRITE_SIZE),
            0.1f
        );
    }

    // 13행 1~4열
    for (int i = 0; i < 4; ++i)
    {
        pInhaleSuccessAnim->AddFrame(
            Vec2(BORDER + i * SPRITE_SIZE, BORDER + SPRITE_SIZE * 12),
            Vec2(SPRITE_SIZE, SPRITE_SIZE),
            0.1f
        );
    }

    pAnimator->AddCustomAnimation(L"INHALE_SUCCESS", pInhaleSuccessAnim);

    // ===========================================
    // 13행: 빨아들인 후 IDLE 상태 (1~2열) - 위에서 처리됨
    // ===========================================
    pAnimator->CreateAnimation(L"MOUTHFUL_IDLE", pKirbyTex,
        Vec2(BORDER, BORDER + SPRITE_SIZE * 12),
        Vec2(SPRITE_SIZE, SPRITE_SIZE),
        Vec2(SPRITE_SIZE, 0),
        0.3f, 2, true);

    // ===========================================
    // 14행: 빨아들인 후 WALK, RUN 상태 (1~14열)
    // ===========================================
    pAnimator->CreateAnimation(L"MOUTHFUL_WALK", pKirbyTex,
        Vec2(BORDER, BORDER + SPRITE_SIZE * 13),
        Vec2(SPRITE_SIZE, SPRITE_SIZE),
        Vec2(SPRITE_SIZE, 0),
        0.1f, 14, true);

    // 빨아들인 후 RUN은 WALK와 같은 애니메이션을 더 빠르게
    pAnimator->CreateAnimation(L"MOUTHFUL_RUN", pKirbyTex,
        Vec2(BORDER, BORDER + SPRITE_SIZE * 13),
        Vec2(SPRITE_SIZE, SPRITE_SIZE),
        Vec2(SPRITE_SIZE, 0),
        0.07f, 14, true);

    // ===========================================
    // 15행: 빨아들인 후 점프 상태 (2~6열)
    // ===========================================
    pAnimator->CreateAnimation(L"MOUTHFUL_JUMP", pKirbyTex,
        Vec2(BORDER + SPRITE_SIZE, BORDER + SPRITE_SIZE * 14),
        Vec2(SPRITE_SIZE, SPRITE_SIZE),
        Vec2(SPRITE_SIZE, 0),
        0.1f, 5, false);

    // ===========================================
    // 17~19행: 빨아들인 후 내뱉기 (17행 1~5열 + 18행 1~5열 + 19행 1~4열)
    // ===========================================
    CAnimation* pSpitOutAnim = new CAnimation();
    pSpitOutAnim->SetName(L"SPIT_OUT");
    pSpitOutAnim->SetTexture(pKirbyTex);
    pSpitOutAnim->SetLoop(false);

    // 17행 1~5열
    for (int i = 0; i < 5; ++i)
    {
        pSpitOutAnim->AddFrame(
            Vec2(BORDER + i * SPRITE_SIZE, BORDER + SPRITE_SIZE * 16),
            Vec2(SPRITE_SIZE, SPRITE_SIZE),
            0.08f
        );
    }

    // 18행 1~5열
    for (int i = 0; i < 5; ++i)
    {
        pSpitOutAnim->AddFrame(
            Vec2(BORDER + i * SPRITE_SIZE, BORDER + SPRITE_SIZE * 17),
            Vec2(SPRITE_SIZE, SPRITE_SIZE),
            0.08f
        );
    }

    // 19행 1~4열
    for (int i = 0; i < 4; ++i)
    {
        pSpitOutAnim->AddFrame(
            Vec2(BORDER + i * SPRITE_SIZE, BORDER + SPRITE_SIZE * 18),
            Vec2(SPRITE_SIZE, SPRITE_SIZE),
            0.08f
        );
    }

    pAnimator->AddCustomAnimation(L"SPIT_OUT", pSpitOutAnim);

    // ===========================================
    // 19행: 내뱉은 투사체(별) 스프라이트 (1~4열) - 위에서 처리됨
    // ===========================================
    pAnimator->CreateAnimation(L"STAR_PROJECTILE", pKirbyTex,
        Vec2(BORDER, BORDER + SPRITE_SIZE * 18),
        Vec2(SPRITE_SIZE, SPRITE_SIZE),
        Vec2(SPRITE_SIZE, 0),
        0.1f, 4, true);

    // ===========================================
    // 20행: 빨아들인 후 삼키기 상태 (1~6열)
    // ===========================================
    pAnimator->CreateAnimation(L"SWALLOW", pKirbyTex,
        Vec2(BORDER, BORDER + SPRITE_SIZE * 19),
        Vec2(SPRITE_SIZE, SPRITE_SIZE),
        Vec2(SPRITE_SIZE, 0),
        0.15f, 6, false);

    // ===========================================
    // 21행: 대미지를 받는 상태 (1~9열)
    // ===========================================
    pAnimator->CreateAnimation(L"DAMAGE", pKirbyTex,
        Vec2(BORDER, BORDER + SPRITE_SIZE * 20),
        Vec2(SPRITE_SIZE, SPRITE_SIZE),
        Vec2(SPRITE_SIZE, 0),
        0.1f, 9, false);

    // ===========================================
    // 22행: 빨아들인 후 대미지 상태 (1~4열)
    // ===========================================
    pAnimator->CreateAnimation(L"MOUTHFUL_DAMAGE", pKirbyTex,
        Vec2(BORDER, BORDER + SPRITE_SIZE * 21),
        Vec2(SPRITE_SIZE, SPRITE_SIZE),
        Vec2(SPRITE_SIZE, 0),
        0.1f, 4, false);

    // 초기 애니메이션 재생
    pAnimator->Play(L"IDLE", true);
}
