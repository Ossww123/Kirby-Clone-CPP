#pragma once

class CObject;
class CAnimation;
class CTexture;

class CAnimator
{
public:
    CAnimator();
    ~CAnimator();

public:
    // === 기본 시스템 함수들 ===
    void Update();
    void Render(HDC _dc);

public:
    // === 애니메이션 생성 및 관리 ===
    void CreateAnimation(const wstring& _strName, CTexture* _pTex, Vec2 _vLT,
        Vec2 _vSliceSize, Vec2 _vStep, float _fDuration,
        int _iFrameCount, bool _bLoop = true);
    void AddCustomAnimation(const wstring& _strName, CAnimation* _pAnim);

private:
    // === 파일 관리 (미구현) ===
    void LoadAnimation(const wstring& _strRelativePath);
    void SaveAnimation(const wstring& _strRelativePath);

public:
    // === 애니메이션 재생 제어 ===
    void Play(const wstring& _strName, bool _bRepeat);

public:
    // === 렌더링 옵션들 ===
    void RenderScaled(HDC _dc, float _fScale);
    void RenderAtPosition(HDC _dc, Vec2 _vPos, float _fScale = 0.f); // 지정된 위치에 렌더링

private:
    // === 애니메이션 검색 ===
    CAnimation* FindAnimation(const wstring& _strName);

public:
    // === Getter 함수들 ===
    CAnimation* GetCurAnim() const { return m_pCurAnim; }
    
    // === 플립 관련 함수들 ===
    void SetFlipX(bool _bFlip) { m_bFlipX = _bFlip; }
    bool IsFlipX() const { return m_bFlipX; }

private:
    // === 멤버 변수들 ===
    CObject* m_pOwner;                          // 소유자 오브젝트
    map<wstring, CAnimation*> m_mapAnim;        // 애니메이션 맵
    CAnimation* m_pCurAnim;                     // 현재 재생중인 애니메이션
    bool m_bRepeat;                             // 반복 재생 상태
    bool m_bFlipX;                              // X축 플립 여부

    friend class CObject;
};