#include "pch.h"
#include "CDeathEffect.h"
#include "CTimeMgr.h"
#include "CAnimator.h"
#include "CAnimationDataMgr.h"

CDeathEffect::CDeathEffect()
    : m_fTimer(0.f)
    , m_fLifeTime(1.0f)  // 1초간 재생
    , m_bAnimationLoaded(false)
{
    SetType(OBJECT_TYPE::EFFECT);
    
    // 애니메이터 추가
    CreateAnimator();
}

CDeathEffect::~CDeathEffect()
{
}

void CDeathEffect::Init()
{
    LoadAnimation();
    
    // IDLE 애니메이션 재생 (한 번만 재생)
    CAnimator* pAnimator = GetAnimator();
    if (pAnimator)
    {
        pAnimator->Play(L"IDLE", false);
    }
}

void CDeathEffect::Update()
{
    // 시간 업데이트
    m_fTimer += CTimeMgr::GetInst()->GetfDT();
    
    // 생존 시간이 지나면 삭제
    if (m_fTimer >= m_fLifeTime)
    {
        SetDead();
        return;
    }
    
    // 애니메이터 업데이트
    if (nullptr != GetAnimator())
        GetAnimator()->Update();
}

void CDeathEffect::Render(HDC _dc)
{
    if (nullptr != GetAnimator())
        GetAnimator()->Render(_dc);
}

void CDeathEffect::LoadAnimation()
{
    if (m_bAnimationLoaded)
        return;
        
    CAnimator* pAnimator = GetAnimator();
    if (!pAnimator)
        return;

    // 데스 이펙트 애니메이션 파일 로드
    wstring animationFilePath = L"death_effect_animations.json";
    CAnimationDataMgr::GetInst()->LoadAnimationsIntoAnimator(pAnimator, animationFilePath);
    
    m_bAnimationLoaded = true;
}