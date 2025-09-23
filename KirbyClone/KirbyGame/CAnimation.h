#pragma once

class CTexture;

class CAnimation
{
public:
    CAnimation();
    ~CAnimation();

    // 런루프
    void  Update(); // CTimeMgr 사용
    void  Reset();

    // 렌더
    void  Render(HDC _dc, Vec2 _vWorldPos) { RenderScaled(_dc, _vWorldPos, 1.f, false); }
    void  RenderScaled(HDC _dc, const Vec2& _vWorldPos, float _fScale, bool _flipX);

    // 데이터/컨트롤
    void  SetName(const std::wstring& n) { m_strName = n; }
    const std::wstring& GetName() const { return m_strName; }

    void  SetLoop(bool b) { m_bLoop = b; }
    bool  IsLoop() const { return m_bLoop; }

    void  SetSheet(CTexture* tex) { m_pSheet = tex; }
    CTexture* GetSheet() const { return m_pSheet; }

    // 프레임 구성
    void  AddFrame(Vec2 vLT, Vec2 vSlice, float dur, Vec2 vOffset = Vec2{ 0,0 });
    void  ClearFrames();

    int   GetCurFrameIndex() const { return m_iCurFrame; }
    int   GetFrameCount()    const { return (int)m_vecFrame.size(); }

private:
    void  RenderFrame(HDC _dc, const tAnimFrame& fr, const Vec2& vWorldPos, float fScale, bool flipX);
    void  EnsureFlipSurfaces(const POINT& sizePx);
    void  ReleaseFlipSurfaces();

private:
    std::wstring          m_strName;
    std::vector<tAnimFrame> m_vecFrame;
    int                   m_iCurFrame{ 0 };
    float                 m_fAccTime{ 0.f };
    bool                  m_bLoop{ true };
    bool                  m_bFinish{ false };

    CTexture* m_pSheet{ nullptr }; // 애니메이션 단위 시트 1개

    // 좌우반전용 캐시
    HDC                   m_hFlipDC1{ nullptr };
    HDC                   m_hFlipDC2{ nullptr };
    HBITMAP               m_hFlipBmp1{ nullptr };
    HBITMAP               m_hFlipBmp2{ nullptr };
    POINT                 m_cachedSize{ 0,0 };
};
