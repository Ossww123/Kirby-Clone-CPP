#pragma once

class CTexture;

class CAnimation
{
private:
    wstring             m_strName;      // 애니메이션 이름
    CTexture*           m_pTex;         // 스프라이트 시트 텍스처
    vector<tAnimFrame>  m_vecFrame;     // 애니메이션 프레임들

    int                 m_iCurFrame;    // 현재 프레임 인덱스
    float               m_fAccTime;     // 누적 시간
    bool                m_bFinish;      // 애니메이션 완료 여부
    bool                m_bLoop;        // 반복 재생 여부

public:
    void SetName(const wstring& _strName) { m_strName = _strName; }
    void SetTexture(CTexture* _pTex) { m_pTex = _pTex; }
    void SetLoop(bool _bLoop) { m_bLoop = _bLoop; }

    const wstring& GetName() { return m_strName; }
    bool IsFinish() { return m_bFinish; }
    tAnimFrame& GetFrame(int _iIdx) { return m_vecFrame[_iIdx]; }
    int GetMaxFrame() { return (int)m_vecFrame.size(); }
    int GetCurFrame() { return m_iCurFrame; }
    CTexture* GetTexture() { return m_pTex; }

    void Create(CTexture* _pTex, Vec2 _vLT, Vec2 _vSliceSize, Vec2 _vStep, float _fDuration, int _iFrameCount, bool _bLoop = true);
    void Update();
    void Render(HDC _dc, Vec2 _vPos);
    void Reset();

    // 새로 추가된 함수들
    void AddFrame(Vec2 _vLT, Vec2 _vSliceSize, float _fDuration);  // 개별 프레임 추가
    void ClearFrames();                                            // 모든 프레임 제거

public:
    CAnimation();
    ~CAnimation();
};