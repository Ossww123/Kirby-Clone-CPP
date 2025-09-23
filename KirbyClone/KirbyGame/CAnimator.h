#pragma once

class CObject;
class CAnimation;
class CTexture;

class CAnimator
{
public:
    CAnimator();
    ~CAnimator();

    // 시스템
    void Update();
    void Render(HDC _dc);

    // 애니메이션 구성
    void CreateAnimation(const std::wstring& name, CTexture* sheet, Vec2 vLT,
        Vec2 vSlice, Vec2 vStep, float dur, int count, bool loop = true);
    void AddCustomAnimation(const std::wstring& name, CAnimation* anim);

    // 재생
    void Play(const std::wstring& name, bool repeat);

    // 렌더 옵션
    void RenderScaled(HDC _dc, float scale);
    void RenderAtPosition(HDC _dc, Vec2 worldPos, float scale = 0.f);

    // 조회
    CAnimation* GetCurAnim() const { return m_pCurAnim; }

    // 플립(기존 코드 호환)
    void SetFlipX(bool b) { m_bFlipX = b; }
    bool IsFlipX() const { return m_bFlipX; }

private:
    CAnimation* FindAnimation(const std::wstring& name);

private:
    CObject* m_pOwner{ nullptr };
    std::map<std::wstring, CAnimation*> m_mapAnim;
    CAnimation* m_pCurAnim{ nullptr };
    bool                             m_bRepeat{ true };
    bool                             m_bFlipX{ false }; // owner.Transform().flipX와 OR

    friend class CObject;
};
