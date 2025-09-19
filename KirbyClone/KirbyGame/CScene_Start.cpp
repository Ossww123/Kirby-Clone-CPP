#include "pch.h"
#include "CScene_Start.h"

#include "CCore.h"
#include "CCamera.h"
#include "CResMgr.h"
#include "CTexture.h"
#include "CKeyMgr.h"
#include "CFadeEffect.h"
#include "CSceneMgr.h"
#include "CEventMgr.h"
#include "CAnimator.h"
#include "CAnimationDataMgr.h"
#include "CAnimation.h"
#include "CSoundMgr.h"

CScene_Start::CScene_Start()
    : m_pBackgroundTexture(nullptr)
    , m_pLogoAnimator(nullptr)
    , m_bEnterPressed(false)
    , m_bTransitioning(false)
{
}

CScene_Start::~CScene_Start()
{
    if (m_pLogoAnimator)
    {
        delete m_pLogoAnimator;
        m_pLogoAnimator = nullptr;
    }
}

void CScene_Start::Update()
{
    // 전환 중이 아닐 때만 입력 처리
    if (!m_bTransitioning && !CFadeEffect::GetInst()->IsActive())
    {
        // 엔터 키 입력 체크
        if (CKeyMgr::GetInst()->IsKeyTap(KEY::ENTER))
        {
            m_bEnterPressed = true;
            m_bTransitioning = true;
            
            // 흰색 페이드 아웃 시작 (1.0초)
            CFadeEffect::GetInst()->StartFadeOut(FADE_COLOR::WHITE, 1.0f);
        }
    }
    
    // 페이드 아웃이 완료되면 씬 전환
    if (m_bTransitioning && CFadeEffect::GetInst()->IsComplete())
    {
        // Stage1으로 씬 전환 이벤트 발송
        tEvent sceneChangeEvent = {};
        sceneChangeEvent.eType = EVENT_TYPE::SCENE_CHANGE;
        sceneChangeEvent.lParam = (DWORD_PTR)SCENE_TYPE::STAGE_01;
        CEventMgr::GetInst()->AddEvent(sceneChangeEvent);
    }
    
    // 로고 애니메이터 업데이트
    if (m_pLogoAnimator)
    {
        m_pLogoAnimator->Update();
    }
    
    // 페이드 효과 업데이트
    CFadeEffect::GetInst()->Update();
}

void CScene_Start::Render(HDC _dc)
{
    // 배경 렌더링 (화면 중앙에 배치)
    if (m_pBackgroundTexture)
    {
        Vec2 vResolution = CCore::GetInst()->GetResolution();
        Vec2 vBgSize = Vec2(240.f, 160.f) * CCore::GetPixelScale(); // 4배 스케일
        Vec2 vBgPos = Vec2(
            (vResolution.x - vBgSize.x) / 2.f,
            (vResolution.y - vBgSize.y) / 2.f
        );
        
        StretchBlt(_dc,
            (int)vBgPos.x, (int)vBgPos.y,
            (int)vBgSize.x, (int)vBgSize.y,
            m_pBackgroundTexture->GetDC(),
            0, 0,
            (int)m_pBackgroundTexture->GetWidth(),
            (int)m_pBackgroundTexture->GetHeight(),
            SRCCOPY
        );
    }
    
    // 로고 애니메이터 렌더링 (화면 중앙)
    if (m_pLogoAnimator)
    {
        Vec2 vResolution = CCore::GetInst()->GetResolution();
        Vec2 vLogoPos = Vec2(vResolution.x / 2.f + 24.f, vResolution.x / 4.f );
        
        m_pLogoAnimator->RenderAtPosition(_dc, vLogoPos);
    }
    
    // 페이드 효과 렌더링 (항상 마지막)
    CFadeEffect::GetInst()->Render(_dc);
}

void CScene_Start::Enter()
{
    // 씬 일시정지 해제
    SetPaused(false);
    
    // 배경 텍스처 로드
    m_pBackgroundTexture = CResMgr::GetInst()->LoadTexture(L"TitleBackground", L"texture/UI/title_background.bmp");
    
    // 로고 애니메이터 생성 및 애니메이션 로드
    m_pLogoAnimator = new CAnimator();
    
    // JSON 파일에서 애니메이션 데이터를 애니메이터에 로드
    CAnimationDataMgr::GetInst()->LoadAnimationsIntoAnimator(m_pLogoAnimator, L"title_logo_animations.json");
    
    // 애니메이션 재생 (JSON에 정의된 애니메이션 이름 사용)
    m_pLogoAnimator->Play(L"IDLE", true); // 반복 재생
    
    // 상태 초기화
    m_bEnterPressed = false;
    m_bTransitioning = false;
    
    // 타이틀 BGM 로드 및 재생
    CSoundMgr::GetInst()->LoadSound(L"main_title", L"sound/main_title.mp3", SOUND_TYPE::BGM);
    CSoundMgr::GetInst()->PlayBGM(L"main_title", true);
    
    // 커비 효과음들 로드
    CSoundMgr::GetInst()->LoadSound(L"kirby_run", L"sound/kirby_run.wav", SOUND_TYPE::SFX);
    CSoundMgr::GetInst()->LoadSound(L"kirby_jump", L"sound/kirby_jump.wav", SOUND_TYPE::SFX);
    CSoundMgr::GetInst()->LoadSound(L"kirby_slide", L"sound/kirby_slide.wav", SOUND_TYPE::SFX);
    CSoundMgr::GetInst()->LoadSound(L"kirby_bounce", L"sound/kirby_bounce.wav", SOUND_TYPE::SFX);
    CSoundMgr::GetInst()->LoadSound(L"kirby_beam", L"sound/kirby_beam.wav", SOUND_TYPE::SFX);
    CSoundMgr::GetInst()->LoadSound(L"kirby_fire", L"sound/kirby_fire.wav", SOUND_TYPE::SFX);
    CSoundMgr::GetInst()->LoadSound(L"kirby_spark", L"sound/kirby_spark.wav", SOUND_TYPE::SFX);
    CSoundMgr::GetInst()->LoadSound(L"monster_damage", L"sound/monster_damage.wav", SOUND_TYPE::SFX);
    CSoundMgr::GetInst()->LoadSound(L"kirbydance_short", L"sound/kirbydance_short.wav", SOUND_TYPE::SFX);
    CSoundMgr::GetInst()->LoadSound(L"kirby_hover", L"sound/kirby_hover.wav", SOUND_TYPE::SFX);
    CSoundMgr::GetInst()->LoadSound(L"kirby_inhale", L"sound/kirby_inhale.wav", SOUND_TYPE::SFX);
    CSoundMgr::GetInst()->LoadSound(L"kirby_swallow", L"sound/kirby_swallow.wav", SOUND_TYPE::SFX);
    CSoundMgr::GetInst()->LoadSound(L"exhale_air_puff", L"sound/exhale_air_puff.wav", SOUND_TYPE::SFX);
    CSoundMgr::GetInst ( )->LoadSound ( L"kirby_exhale_star" , L"sound/kirby_exhale_star.wav" , SOUND_TYPE::SFX );
    CSoundMgr::GetInst()->LoadSound(L"enter_door", L"sound/enter_door.wav", SOUND_TYPE::SFX);
    CSoundMgr::GetInst()->LoadSound(L"enemy_death", L"sound/enemy_death.wav", SOUND_TYPE::SFX);
    CSoundMgr::GetInst()->LoadSound(L"copy", L"sound/kirby_copy.wav", SOUND_TYPE::SFX);
    CSoundMgr::GetInst()->LoadSound(L"kirby_damage", L"sound/kirby_damage.wav", SOUND_TYPE::SFX);
    CSoundMgr::GetInst()->LoadSound(L"gameover", L"sound/gameover.wav", SOUND_TYPE::SFX);
    CSoundMgr::GetInst ( )->LoadSound ( L"boss_clear" , L"sound/boss_clear.wav" , SOUND_TYPE::SFX );
    
    // 몬스터 공격 효과음들 로드
    CSoundMgr::GetInst()->LoadSound(L"sparky", L"sound/sparky.wav", SOUND_TYPE::SFX);
    CSoundMgr::GetInst()->LoadSound(L"hothead", L"sound/hothead.wav", SOUND_TYPE::SFX);
    CSoundMgr::GetInst()->LoadSound(L"waddledoo", L"sound/waddledoo.wav", SOUND_TYPE::SFX);
    
    // 보스 관련 효과음들 로드
    CSoundMgr::GetInst()->LoadSound(L"boss_HP_fill", L"sound/boss_HP_fill.wav", SOUND_TYPE::SFX);
    
    // 흰색 페이드 인 시작 (씬 진입시)
    CFadeEffect::GetInst()->StartFadeIn(FADE_COLOR::WHITE, 1.0f);
}

void CScene_Start::Exit()
{
    // BGM 정지
    CSoundMgr::GetInst()->StopBGM();
    
    // 리소스 정리는 소멸자에서 처리
}