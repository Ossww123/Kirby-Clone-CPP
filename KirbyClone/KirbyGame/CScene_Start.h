#pragma once
#include "CScene.h"

class CTexture;
class CAnimator;

class CScene_Start : public CScene
{
public:
    virtual void Update();
    virtual void Render(HDC _dc);

    virtual void Enter();
    virtual void Exit();

public:
    CScene_Start();
    ~CScene_Start();

private:
    // 타이틀 화면 리소스
    CTexture* m_pBackgroundTexture;     // 배경 텍스처
    CAnimator* m_pLogoAnimator;         // 로고 애니메이터
    
    // 입력 처리
    bool m_bEnterPressed;               // 엔터 키 입력 상태
    bool m_bTransitioning;              // 전환 중 상태
};