#pragma once

class CTexture;

class CAnimation
{
public:
    CAnimation();
    ~CAnimation();

public:
    // === 기본 시스템 함수들 ===
    void Update();
    void Render(HDC _dc, Vec2 _vPos);
    void Reset();

public:
    // === 애니메이션 생성 및 관리 ===
    void Create(CTexture* _pTex, Vec2 _vLT, Vec2 _vSliceSize, Vec2 _vStep,
        float _fDuration, int _iFrameCount, bool _bLoop = true);
    void AddFrame(Vec2 _vLT, Vec2 _vSliceSize, float _fDuration);
    void ClearFrames();

public:
    // === 렌더링 옵션 ===
    void RenderScaled(HDC _dc, Vec2 _vPos, float _fScale, bool _bFlipX = false);

private:
    // === 유효성 검사 ===
    bool IsValidCreateParams(CTexture* _pTex, int _iFrameCount, float _fDuration) const;
    bool IsValidRenderState() const;

private:
    // === 내부 처리 ===
    void CreateFrameSequence(Vec2 _vLT, Vec2 _vSliceSize, Vec2 _vStep,
        float _fDuration, int _iFrameCount);
    void RenderFrame(HDC _dc, const Vec2& _vRenderPos, const tAnimFrame& _frame,
        const Vec2& _vDestSize, bool _bFlipX = false);

public:
    // === Setter 함수들 ===
    void SetName(const wstring& _strName) { m_strName = _strName; }
    void SetTexture(CTexture* _pTex) { m_pTex = _pTex; }
    void SetLoop(bool _bLoop) { m_bLoop = _bLoop; }

public:
    // === Getter 함수들 ===
    const wstring& GetName() const { return m_strName; }
    bool IsFinish() const { return m_bFinish; }
    tAnimFrame& GetFrame(int _iIdx) { return m_vecFrame[_iIdx]; }
    int GetMaxFrame() const { return (int)m_vecFrame.size(); }
    int GetCurFrame() const { return m_iCurFrame; }
    CTexture* GetTexture() const { return m_pTex; }

private:
    // === 멤버 변수들 ===
    wstring m_strName;                  // 애니메이션 이름
    CTexture* m_pTex;                   // 스프라이트 시트 텍스처
    vector<tAnimFrame> m_vecFrame;      // 애니메이션 프레임들

    int m_iCurFrame;                    // 현재 프레임 인덱스
    float m_fAccTime;                   // 누적 시간
    bool m_bFinish;                     // 애니메이션 완료 상태
    bool m_bLoop;                       // 반복 재생 상태
};