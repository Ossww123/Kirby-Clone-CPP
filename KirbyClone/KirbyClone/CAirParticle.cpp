#include "pch.h"
#include "CAirParticle.h"
#include "CTimeMgr.h"
#include "CCamera.h"
#include "CResMgr.h"
#include "CTexture.h"

CAirParticle::CAirParticle()
    : m_vTargetPos(Vec2(0.f, 0.f))
    , m_vInitialPos(Vec2(0.f, 0.f))
    , m_fMoveSpeed(150.f)
    , m_fTimer(0.f)
    , m_fLifeTime(2.f)
    , m_fBlinkTimer(0.f)
    , m_fBlinkInterval(0.1f)
    , m_bVisible(true)
    , m_vScale(Vec2(64.f, 64.f))
    , m_Color(RGB(200, 220, 255))
    , m_pTexture(nullptr)
    , m_vSpritePos(Vec2(0.f, 0.f))
{
    SetType(OBJECT_TYPE::EFFECT);
}

CAirParticle::~CAirParticle()
{
}

void CAirParticle::Init()
{
    m_vInitialPos = GetPos();
    
    // kirby.bmp 텍스처 로드
    m_pTexture = CResMgr::GetInst()->LoadTexture(L"KirbyTex", L"texture\\kirby\\kirby.bmp");
    
    // 5개의 파티클 스프라이트 위치 중 랜덤 선택
    Vec2 spritePositions[5] = {
        Vec2(48.f, 336.f),
        Vec2(80.f, 336.f), 
        Vec2(144.f, 336.f),
        Vec2(176.f, 336.f),
        Vec2(208.f, 336.f)
    };
    
    int randomIndex = rand() % 5;
    m_vSpritePos = spritePositions[randomIndex];
}

void CAirParticle::Update()
{
    float fDT = CTimeMgr::GetInst()->GetfDT();
    m_fTimer += fDT;

    // 수명이 다하면 삭제 요청
    if (m_fTimer >= m_fLifeTime)
    {
        SetDead();
        DeleteObject(this);
        return;
    }

    UpdateMovement();
    UpdateBlink();
}

void CAirParticle::UpdateMovement()
{
    float fDT = CTimeMgr::GetInst()->GetfDT();
    Vec2 vCurrentPos = GetPos();
    
    // 목표 위치까지의 벡터 계산
    Vec2 vDirection = m_vTargetPos - vCurrentPos;
    float fDistance = sqrtf(vDirection.x * vDirection.x + vDirection.y * vDirection.y);
    
    // 목표에 도달했으면 삭제
    if (fDistance < 5.f)
    {
        SetDead();
        DeleteObject(this);
        return;
    }
    
    // 방향 정규화
    if (fDistance > 0.f)
    {
        vDirection.x /= fDistance;
        vDirection.y /= fDistance;
    }
    
    // 거리에 따라 속도 증가 (빨아들이는 느낌)
    float fSpeedMultiplier = 1.f + (2.f - fDistance / 100.f);
    if (fSpeedMultiplier < 1.f) fSpeedMultiplier = 1.f;
    
    // 위치 업데이트
    Vec2 vNewPos = vCurrentPos + vDirection * m_fMoveSpeed * fSpeedMultiplier * fDT;
    SetPos(vNewPos);
}

void CAirParticle::UpdateBlink()
{
    float fDT = CTimeMgr::GetInst()->GetfDT();
    m_fBlinkTimer += fDT;
    
    if (m_fBlinkTimer >= m_fBlinkInterval)
    {
        m_bVisible = !m_bVisible;
        m_fBlinkTimer = 0.f;
    }
}

void CAirParticle::Render(HDC _dc)
{
    if (!m_bVisible || !m_pTexture) return;
    
    // 카메라 변환된 위치 계산
    Vec2 vRenderPos = CCamera::GetInst()->GetRenderPos(GetPos());
    
    // 스프라이트 렌더링 (16x16을 64x64로 4배 스케일링)
    TransparentBlt(_dc,
        (int)(vRenderPos.x - m_vScale.x * 0.5f), // 목표 x
        (int)(vRenderPos.y - m_vScale.y * 0.5f), // 목표 y
        (int)m_vScale.x,                         // 목표 폭 (64 = 16*4)
        (int)m_vScale.y,                         // 목표 높이 (64 = 16*4)
        m_pTexture->GetDC(),                     // 소스 DC
        (int)m_vSpritePos.x,                     // 소스 x
        (int)m_vSpritePos.y,                     // 소스 y
        16, 16,                                  // 소스 폭, 높이 (원본 16x16)
        RGB(255, 0, 255));                       // 투명색 (마젠타)
}

bool CAirParticle::HasReachedTarget() const
{
    Vec2 vCurrentPos = GetPos();
    Vec2 vDirection = m_vTargetPos - vCurrentPos;
    float fDistance = sqrtf(vDirection.x * vDirection.x + vDirection.y * vDirection.y);
    return fDistance < 5.f;
}