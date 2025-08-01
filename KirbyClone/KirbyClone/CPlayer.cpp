#include "pch.h"
#include "CPlayer.h"

#include "CKeyMgr.h"
#include "CTimeMgr.h"
#include "CCollider.h"
#include "CEventMgr.h"
#include "CCamera.h"
#include "CResMgr.h"
#include "CTexture.h"
#include "CAnimator.h"
#include "CRigidBody.h"
#include "CTile.h"
#include "CMonster.h"
#include "CAnimation.h"

#include "CCore.h"

CPlayer::CPlayer()
    : m_pAnimator(nullptr)
    , m_pRigidBody(nullptr)
    , m_eCurState(PLAYER_STATE::IDLE)
    , m_ePrevState(PLAYER_STATE::END)
    , m_fSpeed(200.f)
    , m_fJumpPower(400.f)
{
    // 충돌체 생성
    CreateCollider();
    GetCollider()->SetScale(Vec2(56.f, 56.f));

    // 애니메이터 생성
    CreateAnimator();
    m_pAnimator = GetAnimator();

    // 리지드바디 생성
    CreateRigidBody();
    m_pRigidBody = GetRigidBody();

    // 리지드바디 설정
    m_pRigidBody->SetMass(1.f);
    m_pRigidBody->SetMaxVelocity(500.f);
    m_pRigidBody->SetFriction(10.f);
    m_pRigidBody->SetUseGravity(true);

    // 애니메이션 생성
    CreateAnimation();

    // 초기 애니메이션 재생
    m_pAnimator->Play(L"IDLE", true);
}

CPlayer::~CPlayer()
{
    // 부모 클래스에서 이미 m_pAnimator를 삭제하므로
    // 여기서는 nullptr로만 설정
    m_pAnimator = nullptr;
    m_pRigidBody = nullptr;
}

void CPlayer::CreateAnimation()
{
    // 커비 IDLE 스프라이트 로드
    CTexture* pIdleTex = CResMgr::GetInst()->LoadTexture(L"KirbyIDLE", L"texture\\kirby\\kirby_IDLE.bmp");

    // 복잡한 IDLE 애니메이션 수동 생성
    CAnimation* pIdleAnim = new CAnimation;
    pIdleAnim->SetName(L"IDLE");
    pIdleAnim->SetTexture(pIdleTex);
    pIdleAnim->SetLoop(true);

    // 1번 프레임 5번 반복 (0.5초)
    for (int i = 0; i < 5; ++i)
    {
        pIdleAnim->AddFrame(Vec2(4, 4), Vec2(80, 72), 0.1f);
    }

    // 2번 프레임 1번 (0.1초)
    pIdleAnim->AddFrame(Vec2(100, 4), Vec2(80, 72), 0.1f);

    // 1번 프레임 5번 반복 (0.5초)
    for (int i = 0; i < 5; ++i)
    {
        pIdleAnim->AddFrame(Vec2(4, 4), Vec2(80, 72), 0.1f);
    }

    // 2번 프레임 1번 (0.1초)
    pIdleAnim->AddFrame(Vec2(100, 4), Vec2(80, 72), 0.1f);

    // 1번 프레임 1번 (0.1초)
    pIdleAnim->AddFrame(Vec2(4, 4), Vec2(80, 72), 0.1f);

    // 2번 프레임 1번 (0.1초)
    pIdleAnim->AddFrame(Vec2(100, 4), Vec2(80, 72), 0.1f);

    // 애니메이터에 수동 애니메이션 등록
    m_pAnimator->AddCustomAnimation(L"IDLE", pIdleAnim);

    // 기존 텍스처로 다른 애니메이션들 생성 (임시)
    CTexture* pTex = CResMgr::GetInst()->LoadTexture(L"KirbySprite", L"texture\\kirby_sprite.bmp");

    // WALK 애니메이션 (기존과 동일)
    m_pAnimator->CreateAnimation(L"WALK", pTex, Vec2(0, 32), Vec2(32, 32), Vec2(32, 0), 0.15f, 8, true);

    // JUMP 애니메이션 (기존과 동일)
    m_pAnimator->CreateAnimation(L"JUMP", pTex, Vec2(0, 64), Vec2(32, 32), Vec2(32, 0), 0.1f, 1, false);
}

void CPlayer::Update()
{
    UpdateMove();
    UpdateState();

    // 리지드바디 업데이트
    if (nullptr != m_pRigidBody)
        m_pRigidBody->Update();

    // 애니메이터 업데이트
    if (nullptr != m_pAnimator)
        m_pAnimator->Update();

    // C키로 카메라가 플레이어를 따라가도록 설정/해제
    if (KEY_TAP(KEY::C))
    {
        if (CCamera::GetInst()->GetTarget() == this)
        {
            CCamera::GetInst()->SetTarget(nullptr);
            SetWindowText(CCore::GetInst()->GetMainHwnd(), L"카메라 추적 해제");
        }
        else
        {
            CCamera::GetInst()->SetTarget(this);
            SetWindowText(CCore::GetInst()->GetMainHwnd(), L"카메라 추적 시작");
        }
    }
}

void CPlayer::UpdateMove()
{
    if (nullptr == m_pRigidBody)
        return;

    // 점프 (바닥에 있을 때만)
    if (KEY_TAP(KEY::SPACE) && m_pRigidBody->IsGround())
    {
        // 점프 직전에 중력 영향을 받지 않도록 즉시 바닥 상태 해제
        m_pRigidBody->SetGround(false);
        m_pRigidBody->SetVelocityY(-m_fJumpPower);
    }

    // 좌우 이동 (리지드바디의 X 속도 직접 설정)
    if (KEY_HOLD(KEY::LEFT))
    {
        m_pRigidBody->SetVelocityX(-m_fSpeed);
    }
    else if (KEY_HOLD(KEY::RIGHT))
    {
        m_pRigidBody->SetVelocityX(m_fSpeed);
    }
    else
    {
        // 키를 누르지 않으면 X축 속도를 0으로 (마찰로 자연스럽게 감속)
        if (m_pRigidBody->IsGround())
        {
            m_pRigidBody->SetVelocityX(0.f);
        }
    }
}

void CPlayer::UpdateState()
{
    if (nullptr == m_pRigidBody)
        return;

    PLAYER_STATE eNewState = m_eCurState;
    Vec2 vVelocity = m_pRigidBody->GetVelocity();

    if (!m_pRigidBody->IsGround())
    {
        eNewState = PLAYER_STATE::JUMP;
    }
    else if (abs(vVelocity.x) > 10.f)  // 속도가 일정 이상일 때만 걷기 상태
    {
        eNewState = PLAYER_STATE::WALK;
    }
    else
    {
        eNewState = PLAYER_STATE::IDLE;
    }

    // 상태가 변경되었다면 애니메이션 변경
    if (m_eCurState != eNewState)
    {
        ChangeState(eNewState);
    }
}


void CPlayer::ChangeState(PLAYER_STATE _eState)
{
    m_ePrevState = m_eCurState;
    m_eCurState = _eState;

    // 상태에 맞는 애니메이션 재생
    switch (m_eCurState)
    {
    case PLAYER_STATE::IDLE:
        m_pAnimator->Play(L"IDLE", true);
        break;
    case PLAYER_STATE::WALK:
        m_pAnimator->Play(L"WALK", true);
        break;
    case PLAYER_STATE::JUMP:
        m_pAnimator->Play(L"JUMP", false);
        break;
    }
}

void CPlayer::Render(HDC _dc)
{
    // 애니메이션 렌더링
    if (nullptr != m_pAnimator)
    {
        m_pAnimator->Render(_dc);
    }

    // 충돌체가 있으면 충돌체도 렌더링
    if (nullptr != GetCollider())
        GetCollider()->Render(_dc);
}



void CPlayer::OnCollisionEnter(CCollider* _pOther)
{
    CObject* pOtherObj = _pOther->GetOwner();

    // 타일과의 충돌 처리
    CTile* pTile = dynamic_cast<CTile*>(pOtherObj);
    if (pTile && pTile->IsSolid())
    {
        Vec2 vPlayerPos = GetPos();
        Vec2 vTilePos = pTile->GetPos();
        Vec2 vVelocity = m_pRigidBody->GetVelocity();

        // 충돌체 크기 가져오기 (실제 충돌체 크기 사용)
        Vec2 vPlayerColliderScale = GetCollider()->GetScale();
        Vec2 vTileColliderScale = pTile->GetCollider()->GetScale();

        // 플레이어가 타일 위에서 아래로 떨어지고 있을 때만 착지
        if (vVelocity.y >= 0.f && vPlayerPos.y < vTilePos.y)
        {
            // 플레이어 충돌체의 바닥면과 타일 충돌체의 윗면이 맞닿도록 배치
            float tileTop = vTilePos.y - vTileColliderScale.y / 2.f;
            float playerHalfHeight = vPlayerColliderScale.y / 2.f;

            // 플레이어 중심을 타일 윗면에서 플레이어 충돌체 높이의 절반만큼 위에 배치
            // 이렇게 하면 플레이어 충돌체 바닥이 타일 충돌체 윗면과 정확히 맞닿음
            float newY = tileTop - playerHalfHeight;

            vPlayerPos.y = newY;
            SetPos(vPlayerPos);

            // 수직 속도만 0으로 설정
            m_pRigidBody->SetVelocityY(0.f);
            m_pRigidBody->SetGround(true);

            SetWindowText(CCore::GetInst()->GetMainHwnd(), L"타일 위에 착지! (충돌체 맞닿음)");
        }
    }

    // 몬스터와의 충돌 처리
    CMonster* pMonster = dynamic_cast<CMonster*>(pOtherObj);
    if (pMonster)
    {
        CCamera::GetInst()->CameraShake(0.3f, 10.f);
        SetWindowText(CCore::GetInst()->GetMainHwnd(), L"몬스터와 충돌!");
    }
}

void CPlayer::OnCollision(CCollider* _pOther)
{
    // 지속적인 충돌 처리 - 충돌체가 계속 맞닿아 있어야 함
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
            // 미세한 위치 조정 (중력에 의한 약간의 침투 보정)
            if (playerBottom > tileTop + 2.f)  // 2픽셀 이상 침투했다면
            {
                float correctedY = tileTop - vPlayerColliderScale.y / 2.f;
                vPlayerPos.y = correctedY;
                SetPos(vPlayerPos);
            }

            m_pRigidBody->SetGround(true);

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

        // 위쪽으로 빠르게 이동 중이거나 (점프), 수평으로 이동해서 벗어났을 때
        if (vVelocity.y < -30.f || abs(vVelocity.x) > 50.f)
        {
            m_pRigidBody->SetGround(false);
        }

        SetWindowText(CCore::GetInst()->GetMainHwnd(), L"타일에서 벗어남");
    }
}