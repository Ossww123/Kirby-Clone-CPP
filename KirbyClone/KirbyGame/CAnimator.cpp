#include "gamePCH.h"
#include "CAnimator.h"
#include "CAnimation.h"
#include "CTexture.h"
#include "CObject.h"
#include "CCore.h"

CAnimator::CAnimator() {}
CAnimator::~CAnimator()
{
    for (auto& kv : m_mapAnim) delete kv.second;
    m_mapAnim.clear();
    m_pCurAnim = nullptr;
}

void CAnimator::Update()
{
    if (m_pCurAnim) m_pCurAnim->Update();
}

void CAnimator::Render(HDC _dc)
{
    if (!m_pCurAnim || !m_pOwner) return;

    const float baseScale = CCore::PIXEL_SCALE;
    const float scale = baseScale;

    // 최종 flipX = 오브젝트.Transform.flipX OR Animator.m_bFlipX
    bool flipX = m_bFlipX;
    // CObject에 IsFlipX() 존재(Transform2D 반영) 확인됨
    flipX = flipX || m_pOwner->IsFlipX();

    m_pCurAnim->RenderScaled(_dc, m_pOwner->GetPos(), scale, flipX);
}

void CAnimator::RenderScaled(HDC _dc, float scale)
{
    if (!m_pCurAnim || !m_pOwner) return;
    if (scale <= 0.f) scale = CCore::PIXEL_SCALE;

    bool flipX = m_bFlipX || m_pOwner->IsFlipX();
    m_pCurAnim->RenderScaled(_dc, m_pOwner->GetPos(), scale, flipX);
}

void CAnimator::RenderAtPosition(HDC _dc, Vec2 worldPos, float scale)
{
    if (!m_pCurAnim) return;
    if (scale <= 0.f) scale = CCore::PIXEL_SCALE;

    bool flipX = m_bFlipX;
    if (m_pOwner) flipX = flipX || m_pOwner->IsFlipX();

    m_pCurAnim->RenderScaled(_dc, worldPos, scale, flipX);
}

void CAnimator::CreateAnimation(const std::wstring& name, CTexture* sheet,
    Vec2 vLT, Vec2 vSlice, Vec2 vStep,
    float dur, int count, bool loop)
{
    if (!sheet || count <= 0 || dur <= 0.f) return;

    CAnimation* anim = new CAnimation;
    anim->SetName(name);
    anim->SetSheet(sheet);
    anim->SetLoop(loop);

    for (int i = 0; i < count; ++i)
    {
        Vec2 lt = Vec2(vLT.x + vStep.x * i, vLT.y + vStep.y * i);
        anim->AddFrame(lt, vSlice, dur); // vOffset은 기본 {0,0}
    }
    m_mapAnim[name] = anim;
}

void CAnimator::AddCustomAnimation(const std::wstring& name, CAnimation* anim)
{
    if (!anim) return;
    anim->SetName(name);
    m_mapAnim[name] = anim;
}

void CAnimator::Play(const std::wstring& name, bool repeat)
{
    CAnimation* found = FindAnimation(name);
    if (!found) return;

    m_bRepeat = repeat;
    found->SetLoop(repeat);
    m_pCurAnim = found;
    m_pCurAnim->Reset();
}

CAnimation* CAnimator::FindAnimation(const std::wstring& name)
{
    auto it = m_mapAnim.find(name);
    return (it == m_mapAnim.end()) ? nullptr : it->second;
}
