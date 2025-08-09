#pragma once

class CObject;
class CAnimation;
class CTexture;

class CAnimator
{
private:
    CObject* m_pOwner;       // 소유자 오브젝트
    map<wstring, CAnimation*>   m_mapAnim;      // 애니메이션 맵
    CAnimation* m_pCurAnim;     // 현재 재생중인 애니메이션
    bool                        m_bRepeat;      // 반복 재생 여부

public:
    void CreateAnimation(const wstring& _strName, CTexture* _pTex, Vec2 _vLT, Vec2 _vSliceSize, Vec2 _vStep, float _fDuration, int _iFrameCount, bool _bLoop = true);
    void LoadAnimation(const wstring& _strRelativePath);
    void SaveAnimation(const wstring& _strRelativePath);

    void Play(const wstring& _strName, bool _bRepeat);
    void Update();
    void Render(HDC _dc);

    CAnimation* FindAnimation(const wstring& _strName);
    CAnimation* GetCurAnim() { return m_pCurAnim; }

    void AddCustomAnimation(const wstring& _strName, CAnimation* _pAnim);
    void RenderScaled(HDC _dc, float _fScale);

public:
    CAnimator();
    ~CAnimator();

    friend class CObject;
};