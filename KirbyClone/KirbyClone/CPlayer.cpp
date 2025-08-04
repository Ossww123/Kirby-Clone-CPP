#include "pch.h"
#include "CPlayer.h"
#include "CAnimator.h"
#include "CAnimation.h"
#include "CTexture.h"
#include "CRigidBody.h"
#include "CCollider.h"
#include "CPlayerStateMachine.h"
#include "CPlayerInhaleSystem.h"
#include "CPlayerMovement.h"
#include "CPlayerHealthSystem.h"
#include "CKeyMgr.h"
#include "CTimeMgr.h"
#include "CTile.h"
#include "CMonster.h"
#include "CResMgr.h"
#include "CCore.h"
#include "CCamera.h"

CPlayer::CPlayer()
    : m_pAnimator(nullptr)
    , m_pRigidBody(nullptr)
    , m_pStateMachine(nullptr)
    , m_pInhaleSystem(nullptr)
    , m_pMovement(nullptr)
    , m_pHealthSystem(nullptr)
{
    // === 컴포넌트들 생성 (기존 방식) ===
    CreateAnimator();
    m_pAnimator = GetAnimator();

    CreateRigidBody();
    m_pRigidBody = GetRigidBody();

    // 리지드바디 설정 (올바른 함수명 사용)
    m_pRigidBody->SetMass(1.f);
    m_pRigidBody->SetMaxVelocity(500.f);
    m_pRigidBody->SetFriction(0.1f);
    m_pRigidBody->SetUseGravity(true);

    // 콜라이더 생성 및 설정
    CreateCollider();
    GetCollider()->SetOffsetPos(Vec2(0.f, 0.f));
    GetCollider()->SetScale(Vec2(56.f, 56.f));

    // === 시스템들 생성 ===
    m_pStateMachine = new CPlayerStateMachine(this);
    m_pInhaleSystem = new CPlayerInhaleSystem(this);
    m_pMovement = new CPlayerMovement(this);
    m_pHealthSystem = new CPlayerHealthSystem(this);

    // === 시스템들 초기화 ===
    m_pStateMachine->Init(m_pAnimator, m_pRigidBody);
    m_pInhaleSystem->Init();
    m_pMovement->Init(m_pRigidBody);
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
    if (m_pRigidBody)
        m_pRigidBody->Update();

    // === 상태 머신 업데이트 ===
    if (m_pStateMachine)
        m_pStateMachine->Update();

    // === 애니메이터 업데이트 ===
    if (m_pAnimator)
        m_pAnimator->Update();
}

void CPlayer::UpdateInhale()
{
    // 물고 있는 것을 뱉기 (Z키)
    if (KEY_TAP(KEY::Z) && HasMouthful())
    {
        SpitOut();
        return;
    }

    // 흡입하기 관련 처리
    if (KEY_TAP(KEY::X))
    {
        if (!IsInhaling() && !HasMouthful())
        {
            StartInhale();
        }
    }

    if (KEY_AWAY(KEY::X))
    {
        if (IsInhaling())
        {
            StopInhale();
        }
    }
}

void CPlayer::CreateAnimation()
{
    // 새로운 커비 스프라이트 시트 로드
    CTexture* pKirbyTex = CResMgr::GetInst()->LoadTexture(L"KirbySprite", L"texture\\kirby\\kirby.bmp");

    // 각 스프라이트는 120x120이고, 실제 이미지는 2px 테두리 제외하고 116x116
    const int SPRITE_SIZE = 120;
    const int IMAGE_SIZE = 116;
    const int BORDER = 2;

    // ===========================================
    // 1행: IDLE 상태 (10프레임)
    // ===========================================
    CAnimation* pIdleAnim = new CAnimation();
    pIdleAnim->SetName(L"IDLE");
    pIdleAnim->SetTexture(pKirbyTex);
    pIdleAnim->SetLoop(true);

    // 깜빡임 시퀀스: 눈뜸(1.5초) → 깜빡(0.1초) → 눈뜸(1초) → 깜빡(0.1초) → 눈뜸(0.2초) → 깜빡(0.1초)

    // 1. 눈 뜨고 오래 대기 (1.5초)
    pIdleAnim->AddFrame(Vec2(BORDER, BORDER), Vec2(IMAGE_SIZE, IMAGE_SIZE), 1.5f);

    // 2. 첫 번째 깜빡임 (0.1초)
    pIdleAnim->AddFrame(Vec2(BORDER + SPRITE_SIZE, BORDER), Vec2(IMAGE_SIZE, IMAGE_SIZE), 0.1f);

    // 3. 눈 뜨고 중간 대기 (1초)
    pIdleAnim->AddFrame(Vec2(BORDER, BORDER), Vec2(IMAGE_SIZE, IMAGE_SIZE), 1.0f);

    // 4. 두 번째 깜빡임 (0.1초)
    pIdleAnim->AddFrame(Vec2(BORDER + SPRITE_SIZE, BORDER), Vec2(IMAGE_SIZE, IMAGE_SIZE), 0.1f);

    // 5. 눈 뜨고 짧은 대기 (0.2초)
    pIdleAnim->AddFrame(Vec2(BORDER, BORDER), Vec2(IMAGE_SIZE, IMAGE_SIZE), 0.2f);

    // 6. 세 번째 깜빡임 (0.1초)
    pIdleAnim->AddFrame(Vec2(BORDER + SPRITE_SIZE, BORDER), Vec2(IMAGE_SIZE, IMAGE_SIZE), 0.1f);

    // 애니메이터에 추가
    m_pAnimator->AddCustomAnimation(L"IDLE", pIdleAnim);

    // ===========================================
    // 2행: 걷기 상태 (10프레임)
    // ===========================================
    m_pAnimator->CreateAnimation(L"WALK", pKirbyTex,
        Vec2(BORDER, SPRITE_SIZE + BORDER),
        Vec2(IMAGE_SIZE, IMAGE_SIZE),
        Vec2(SPRITE_SIZE, 0),
        0.1f,                           // 걷기는 좀 더 빠르게
        10,
        true);

    // ===========================================
    // 3행: 뛰기 상태 (10프레임)
    // ===========================================
    m_pAnimator->CreateAnimation(L"RUN", pKirbyTex,
        Vec2(BORDER, SPRITE_SIZE * 2 + BORDER),
        Vec2(IMAGE_SIZE, IMAGE_SIZE),
        Vec2(SPRITE_SIZE, 0),
        0.08f,                          // 뛰기는 더 빠르게
        10,
        true);

    // ===========================================
    // 4행: 점프 상태 (10프레임)
    // ===========================================
    m_pAnimator->CreateAnimation(L"JUMP", pKirbyTex,
        Vec2(BORDER, SPRITE_SIZE * 3 + BORDER),
        Vec2(IMAGE_SIZE, IMAGE_SIZE),
        Vec2(SPRITE_SIZE, 0),
        0.1f,
        10,
        false);                         // 점프는 반복 안함

    // ===========================================
    // 5행: 낙하 - 땅에서 바운스 (10프레임)
    // ===========================================
    m_pAnimator->CreateAnimation(L"FALL", pKirbyTex,
        Vec2(BORDER, SPRITE_SIZE * 4 + BORDER),
        Vec2(IMAGE_SIZE, IMAGE_SIZE),
        Vec2(SPRITE_SIZE, 0),
        0.1f,
        10,
        true);

    // ===========================================
    // 6행: 공기머금기 (10프레임)
    // ===========================================
    m_pAnimator->CreateAnimation(L"INHALE_READY", pKirbyTex,
        Vec2(BORDER, SPRITE_SIZE * 5 + BORDER),
        Vec2(IMAGE_SIZE, IMAGE_SIZE),
        Vec2(SPRITE_SIZE, 0),
        0.1f,
        10,
        false);

    // ===========================================
    // 7행: 공기뱉기 (10프레임)
    // ===========================================
    m_pAnimator->CreateAnimation(L"EXHALE", pKirbyTex,
        Vec2(BORDER, SPRITE_SIZE * 6 + BORDER),
        Vec2(IMAGE_SIZE, IMAGE_SIZE),
        Vec2(SPRITE_SIZE, 0),
        0.08f,
        10,
        false);

    // ===========================================
    // 8행: 빨아들이기 1단계, 2단계, 숨참 (10프레임)
    // ===========================================
    // 1단계 (처음 3프레임)
    m_pAnimator->CreateAnimation(L"INHALE_1", pKirbyTex,
        Vec2(BORDER, SPRITE_SIZE * 7 + BORDER),
        Vec2(IMAGE_SIZE, IMAGE_SIZE),
        Vec2(SPRITE_SIZE, 0),
        0.15f,
        3,
        true);

    // 2단계 (다음 3프레임)
    m_pAnimator->CreateAnimation(L"INHALE_2", pKirbyTex,
        Vec2(BORDER + SPRITE_SIZE * 3, SPRITE_SIZE * 7 + BORDER),
        Vec2(IMAGE_SIZE, IMAGE_SIZE),
        Vec2(SPRITE_SIZE, 0),
        0.12f,
        3,
        true);

    // 숨참 (마지막 4프레임)
    m_pAnimator->CreateAnimation(L"INHALE_HOLD", pKirbyTex,
        Vec2(BORDER + SPRITE_SIZE * 6, SPRITE_SIZE * 7 + BORDER),
        Vec2(IMAGE_SIZE, IMAGE_SIZE),
        Vec2(SPRITE_SIZE, 0),
        0.2f,
        4,
        true);

    // ===========================================
    // 9행: 빨아들이기 공기 파티클 (10프레임) - 이펙트용
    // ===========================================
    m_pAnimator->CreateAnimation(L"INHALE_EFFECT", pKirbyTex,
        Vec2(BORDER, SPRITE_SIZE * 8 + BORDER),
        Vec2(IMAGE_SIZE, IMAGE_SIZE),
        Vec2(SPRITE_SIZE, 0),
        0.05f,
        10,
        true);

    // ===========================================
    // 10행: 삼키기 (3칸씩 사용하므로 3프레임)
    // ===========================================
    m_pAnimator->CreateAnimation(L"SWALLOW", pKirbyTex,
        Vec2(BORDER, SPRITE_SIZE * 9 + BORDER),
        Vec2(IMAGE_SIZE, IMAGE_SIZE),
        Vec2(SPRITE_SIZE, 0),
        0.15f,
        3,
        false);

    // ===========================================
    // 11행: 머금은 상태 IDLE (10프레임)
    // ===========================================
    m_pAnimator->CreateAnimation(L"MOUTHFUL_IDLE", pKirbyTex,
        Vec2(BORDER, SPRITE_SIZE * 10 + BORDER),
        Vec2(IMAGE_SIZE, IMAGE_SIZE),
        Vec2(SPRITE_SIZE, 0),
        0.15f,
        10,
        true);

    // ===========================================
    // 12행: 머금은 상태 걷기/뛰기 (10프레임)
    // ===========================================
    m_pAnimator->CreateAnimation(L"MOUTHFUL_WALK", pKirbyTex,
        Vec2(BORDER, SPRITE_SIZE * 11 + BORDER),
        Vec2(IMAGE_SIZE, IMAGE_SIZE),
        Vec2(SPRITE_SIZE, 0),
        0.1f,
        10,
        true);

    // 머금은 상태 뛰기는 걷기와 같은 애니메이션을 더 빠르게 재생
    m_pAnimator->CreateAnimation(L"MOUTHFUL_RUN", pKirbyTex,
        Vec2(BORDER, SPRITE_SIZE * 11 + BORDER),
        Vec2(IMAGE_SIZE, IMAGE_SIZE),
        Vec2(SPRITE_SIZE, 0),
        0.07f,                          // 더 빠르게
        10,
        true);

    // ===========================================
    // 13-14행: 머금은 상태 점프 (2행 사용, 20프레임)
    // ===========================================
    // 수동으로 프레임 생성 (2행에 걸쳐 있어서)
    CAnimation* pMouthfulJumpAnim = new CAnimation;
    pMouthfulJumpAnim->SetName(L"MOUTHFUL_JUMP");
    pMouthfulJumpAnim->SetTexture(pKirbyTex);
    pMouthfulJumpAnim->SetLoop(false);

    // 첫 번째 행 (10프레임)
    for (int i = 0; i < 10; ++i)
    {
        pMouthfulJumpAnim->AddFrame(
            Vec2(BORDER + i * SPRITE_SIZE, SPRITE_SIZE * 12 + BORDER),
            Vec2(IMAGE_SIZE, IMAGE_SIZE),
            0.1f
        );
    }

    // 두 번째 행 (10프레임) 
    for (int i = 0; i < 10; ++i)
    {
        pMouthfulJumpAnim->AddFrame(
            Vec2(BORDER + i * SPRITE_SIZE, SPRITE_SIZE * 13 + BORDER),
            Vec2(IMAGE_SIZE, IMAGE_SIZE),
            0.1f
        );
    }

    m_pAnimator->AddCustomAnimation(L"MOUTHFUL_JUMP", pMouthfulJumpAnim);

    // 초기 애니메이션 재생
    m_pAnimator->Play(L"IDLE", true);
}

void CPlayer::Render(HDC _dc)
{
    // 게임오버가 아닐 때만 플레이어 렌더링
    if (!m_pHealthSystem || !m_pHealthSystem->IsGameOver())
    {
        // 무적 상태 깜빡임 체크
        bool bShouldRender = true;
        if (m_pHealthSystem && m_pHealthSystem->IsInvincible())
        {
            bShouldRender = m_pHealthSystem->ShouldRenderBlink();
        }

        if (bShouldRender)
        {
            // 기존 애니메이션 렌더링 코드 (방향에 따른 렌더링)
            if (m_pAnimator)
            {
                CAnimation* pCurAnim = m_pAnimator->GetCurAnim();
                if (pCurAnim)
                {
                    Vec2 vPos = GetPos();

                    if (!IsFacingRight())
                    {
                        // 왼쪽을 보고 있을 때 - 스프라이트 뒤집기
                        Vec2 vRenderPos = CCamera::GetInst()->GetRenderPos(vPos);
                        RenderFlippedAnimation(_dc, pCurAnim, vRenderPos);
                    }
                    else
                    {
                        // 오른쪽을 보고 있을 때 - 기본 렌더링
                        pCurAnim->Render(_dc, vPos);
                    }
                }
            }

            // 충돌체 렌더링 (디버그용)
            if (GetCollider())
                GetCollider()->Render(_dc);
        }
    }

    // === 흡입 효과 렌더링 (무적 상태와 관계없이 항상 표시) ===
    if (m_pInhaleSystem)
    {
        m_pInhaleSystem->RenderInhaleEffect(_dc);
    }

    // 체력 시스템 UI 렌더링 (항상 표시)
    if (m_pHealthSystem)
        m_pHealthSystem->Render(_dc);
}

void CPlayer::RenderFlippedAnimation(HDC _dc, CAnimation* _pAnim, Vec2 _vRenderPos)
{
    if (!_pAnim) return;

    CTexture* pTex = _pAnim->GetTexture();
    if (!pTex) return;

    tAnimFrame& frame = _pAnim->GetFrame(_pAnim->GetCurFrame());

    // 투명 처리가 필요한 경우 (마젠타 배경 있음)
    HDC hTempDC = CreateCompatibleDC(_dc);
    HBITMAP hTempBitmap = CreateCompatibleBitmap(_dc, (int)frame.vSlice.x, (int)frame.vSlice.y);
    HBITMAP hOldBitmap = (HBITMAP)SelectObject(hTempDC, hTempBitmap);

    // 한 번에 뒤집어서 임시 DC에 그리기
    StretchBlt(hTempDC,
        (int)frame.vSlice.x - 1, 0,                    // 오른쪽 끝에서 시작
        -(int)frame.vSlice.x, (int)frame.vSlice.y,     // 음수 너비로 뒤집기
        pTex->GetDC(),                                 // 원본 kirby.bmp 사용
        (int)frame.vLT.x, (int)frame.vLT.y,
        (int)frame.vSlice.x, (int)frame.vSlice.y,
        SRCCOPY);

    // 투명 처리해서 화면에 그리기
    TransparentBlt(_dc,
        (int)(_vRenderPos.x - frame.vSlice.x / 2.f),
        (int)(_vRenderPos.y - frame.vSlice.y / 2.f),
        (int)frame.vSlice.x, (int)frame.vSlice.y,
        hTempDC, 0, 0,
        (int)frame.vSlice.x, (int)frame.vSlice.y,
        RGB(255, 0, 255));

    // 정리
    SelectObject(hTempDC, hOldBitmap);
    DeleteObject(hTempBitmap);
    DeleteDC(hTempDC);
}

void CPlayer::OnCollisionEnter(CCollider* _pOther)
{
    CObject* pOtherObj = _pOther->GetOwner();

    // === 타일과의 충돌 처리 (기존 코드 유지) ===
    CTile* pTile = dynamic_cast<CTile*>(pOtherObj);
    if (pTile && pTile->IsSolid())
    {
        Vec2 vPlayerPos = GetPos();
        Vec2 vTilePos = pTile->GetPos();
        Vec2 vVelocity = m_pRigidBody->GetVelocity();

        // 충돌체 크기 가져오기
        Vec2 vPlayerColliderScale = GetCollider()->GetScale();
        Vec2 vTileColliderScale = pTile->GetCollider()->GetScale();

        // 플레이어가 타일 위에서 아래로 떨어지고 있을 때만 착지
        if (vVelocity.y >= 0.f && vPlayerPos.y < vTilePos.y)
        {
            // 플레이어 충돌체의 바닥면과 타일 충돌체의 윗면이 맞닿도록 배치
            float tileTop = vTilePos.y - vTileColliderScale.y / 2.f;
            float playerHalfHeight = vPlayerColliderScale.y / 2.f;

            // 플레이어 중심을 타일 윗면에서 플레이어 충돌체 높이의 절반만큼 위에 배치
            float newY = tileTop - playerHalfHeight;

            vPlayerPos.y = newY;
            SetPos(vPlayerPos);

            // 수직 속도만 0으로 설정
            m_pRigidBody->SetVelocityY(0.f);
            m_pRigidBody->SetGround(true);

            SetWindowText(CCore::GetInst()->GetMainHwnd(), L"타일 위에 착지!");
        }
        return; // 타일 처리 후 리턴
    }

    // === 몬스터와의 충돌 처리 (체력 시스템 통합) ===
    CMonster* pMonster = dynamic_cast<CMonster*>(pOtherObj);
    if (pMonster)
    {
        // 흡어들이기 중이고 범위 내에 있다면 자동으로 흡수
        if (IsInhaling())
        {
            Vec2 vDiff = GetPos() - pMonster->GetPos();
            if (vDiff.Length() < 60.f)
            {
                SwallowTarget(pMonster);
                SetWindowText(CCore::GetInst()->GetMainHwnd(), L"몬스터 흡수!");
                return;
            }
        }

        // 입에 물고 있는 상태가 아니고 흡입 중이 아니라면 데미지 처리
        if (!HasMouthful() && !IsInhaling() && m_pHealthSystem)
        {
            // 체력 시스템이 무적 상태가 아닐 때만 데미지
            if (!m_pHealthSystem->IsInvincible())
            {
                // 넉백 방향 계산 (몬스터에서 플레이어 방향)
                Vec2 vMonsterPos = pMonster->GetPos();
                Vec2 vPlayerPos = GetPos();
                Vec2 vKnockbackDir = vPlayerPos - vMonsterPos;
                vKnockbackDir.Normalize();

                // 체력 시스템을 통해 데미지 적용 (넉백 포함)
                m_pHealthSystem->TakeDamage(1, vKnockbackDir);

                SetWindowText(CCore::GetInst()->GetMainHwnd(), L"몬스터와 충돌! 데미지!");
            }
            else
            {
                SetWindowText(CCore::GetInst()->GetMainHwnd(), L"무적 상태! 데미지 무시");
            }
        }
        return;
    }

    // === 기타 오브젝트와의 충돌 처리 ===
    // TODO: 아이템, 파워업 등의 충돌 처리를 여기에 추가
}

void CPlayer::OnCollision(CCollider* _pOther)
{
    // 지속적인 충돌 처리
    CObject* pOtherObj = _pOther->GetOwner();

    CTile* pTile = dynamic_cast<CTile*>(pOtherObj);
    if (pTile && pTile->IsSolid())
    {
        Vec2 vPlayerPos = GetPos();
        Vec2 vTilePos = pTile->GetPos();
        Vec2 vVelocity = m_pRigidBody->GetVelocity();

        // 충돌체 크기
        Vec2 vPlayerColliderScale = GetCollider()->GetScale();
        Vec2 vTileColliderScale = pTile->GetCollider()->GetScale();

        // 플레이어가 타일 위에 올바르게 서 있는지 확인
        float tileTop = vTilePos.y - vTileColliderScale.y / 2.f;
        float playerBottom = vPlayerPos.y + vPlayerColliderScale.y / 2.f;

        // 플레이어 바닥과 타일 윗면이 거의 맞닿아 있고, 플레이어가 위에 있다면
        if (abs(playerBottom - tileTop) < 8.f && vPlayerPos.y < vTilePos.y)
        {
            // 미세한 위치 조정
            if (playerBottom > tileTop + 5.f)
            {
                float correctedY = tileTop - vPlayerColliderScale.y / 2.f;
                vPlayerPos.y = correctedY;
                SetPos(vPlayerPos);
            }

            // Ground 상태가 아닐 때만 설정 (중복 설정 방지)
            if (!m_pRigidBody->IsGround())
            {
                m_pRigidBody->SetGround(true);
            }

            // 아래로 떨어지는 속도가 있다면 제거
            if (vVelocity.y > 0.f)
            {
                m_pRigidBody->SetVelocityY(0.f);
            }
        }
    }
}

void CPlayer::OnCollisionExit(CCollider* _pOther)
{
    CObject* pOtherObj = _pOther->GetOwner();

    CTile* pTile = dynamic_cast<CTile*>(pOtherObj);
    if (pTile && pTile->IsSolid())
    {
        // 점프나 이동으로 타일에서 벗어날 때만 Ground 해제
        Vec2 vVelocity = m_pRigidBody->GetVelocity();

        // 위쪽으로 빠르게 이동 중일 때만 (점프) Ground 해제
        // X축 속도 조건 제거 - 수평 이동 시에는 Ground 상태 유지
        if (vVelocity.y < -50.f)  // Y축 속도만 체크, 임계값도 높임
        {
            m_pRigidBody->SetGround(false);
        }

        SetWindowText(CCore::GetInst()->GetMainHwnd(), L"타일에서 벗어남");
    }
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

// === 이동 관련 래퍼 함수들 (CPlayerMovement로 위임) ===
bool CPlayer::IsFacingRight() const
{
    return m_pMovement ? m_pMovement->IsFacingRight() : true;
}

bool CPlayer::IsActuallyMoving() const
{
    return m_pMovement ? m_pMovement->IsActuallyMoving() : false;
}

bool CPlayer::IsInputPressed() const
{
    return m_pMovement ? m_pMovement->IsInputPressed() : false;
}

bool CPlayer::IsRunMode() const
{
    return m_pMovement ? m_pMovement->IsRunMode() : false;
}

void CPlayer::SetRunMode(bool _bRunMode)
{
    if (m_pMovement)
        m_pMovement->SetRunMode(_bRunMode);
}

float CPlayer::GetCurrentSpeed() const
{
    return m_pMovement ? m_pMovement->GetCurrentSpeed() : 0.f;
}

void CPlayer::SetFacingDirection(bool _bRight)
{
    if (m_pMovement)
        m_pMovement->SetFacingDirection(_bRight);
}

bool CPlayer::IsDecelerating() const
{
    return m_pMovement ? m_pMovement->IsDecelerating() : false;
}

// === 흡입 관련 래퍼 함수들 (CPlayerInhaleSystem으로 위임) ===
bool CPlayer::IsInhaling() const
{
    return m_pInhaleSystem ? m_pInhaleSystem->IsInhaling() : false;
}

bool CPlayer::HasMouthful() const
{
    return m_pInhaleSystem ? m_pInhaleSystem->HasMouthful() : false;
}

float CPlayer::GetInhaleTime() const
{
    return m_pInhaleSystem ? m_pInhaleSystem->GetInhaleTime() : 0.f;
}

const vector<CObject*>& CPlayer::GetInhaleTargets() const
{
    static vector<CObject*> emptyVec;
    return m_pInhaleSystem ? m_pInhaleSystem->GetInhaleTargets() : emptyVec;
}

OBJECT_TYPE CPlayer::GetMouthfulType() const
{
    return m_pInhaleSystem ? m_pInhaleSystem->GetMouthfulType() : OBJECT_TYPE::END;
}

void CPlayer::SetMouthful(bool _bMouthful)
{
    if (m_pInhaleSystem)
        m_pInhaleSystem->SetMouthful(_bMouthful);
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

void CPlayer::SwallowTarget(CObject* _pTarget)
{
    if (m_pInhaleSystem)
        m_pInhaleSystem->SwallowTarget(_pTarget);
}

void CPlayer::SpitOut()
{
    if (m_pInhaleSystem)
        m_pInhaleSystem->SpitOut();
}

void CPlayer::ReleaseMouthful()
{
    if (m_pInhaleSystem)
        m_pInhaleSystem->ReleaseMouthful();
}

void CPlayer::TakeDamage(int _iDamage, Vec2 _vKnockbackDir)
{
    if (m_pHealthSystem)
        m_pHealthSystem->TakeDamage(_iDamage, _vKnockbackDir);
}

void CPlayer::Heal(int _iHeal)
{
    if (m_pHealthSystem)
        m_pHealthSystem->Heal(_iHeal);
}

void CPlayer::SetHP(int _iHP)
{
    if (m_pHealthSystem)
        m_pHealthSystem->SetHP(_iHP);
}

int CPlayer::GetCurrentHP() const
{
    return m_pHealthSystem ? m_pHealthSystem->GetCurrentHP() : 0;
}

int CPlayer::GetMaxHP() const
{
    return m_pHealthSystem ? m_pHealthSystem->GetMaxHP() : 0;
}

float CPlayer::GetHPRatio() const
{
    return m_pHealthSystem ? m_pHealthSystem->GetHPRatio() : 0.f;
}

bool CPlayer::IsInvincible() const
{
    return m_pHealthSystem ? m_pHealthSystem->IsInvincible() : false;
}

bool CPlayer::IsGameOver() const
{
    return m_pHealthSystem ? m_pHealthSystem->IsGameOver() : false;
}

void CPlayer::SetGameOverY(float _fY)
{
    if (m_pHealthSystem)
        m_pHealthSystem->SetGameOverY(_fY);
}

void CPlayer::RestartStage()
{
    if (m_pHealthSystem)
        m_pHealthSystem->RestartStage();
}

bool CPlayer::ShouldRenderBlink() const
{
    return m_pHealthSystem ? m_pHealthSystem->ShouldRenderBlink() : false;
}