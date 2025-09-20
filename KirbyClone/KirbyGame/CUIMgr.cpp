#include "gamePCH.h"
#include "CUIMgr.h"
#include "CTexture.h"
#include "CResMgr.h"
#include "CPlayer.h"
#include "CBoss.h"
#include "CPlayerDataMgr.h"
#include "CPlayerHealthSystem.h"
#include "CSceneMgr.h"
#include "CScene.h"
#include "CTimeMgr.h"
#include "CSoundMgr.h"

// === UI 스프라이트 좌표 상수 정의 ===
const Vec2 CUIMgr::KIRBY_LIFE_ICON_POS = Vec2(0.f, 0.f);     // 24x16
const Vec2 CUIMgr::NUMBER_0_POS = Vec2(32.f, 0.f);           // 8x16, 숫자별로 +8씩
const Vec2 CUIMgr::HEALTH_FULL_POS = Vec2(112.f, 0.f);       // 8x16
const Vec2 CUIMgr::HEALTH_EMPTY_POS = Vec2(120.f, 0.f);      // 8x16
const Vec2 CUIMgr::BOSS_HP_BAR_POS = Vec2(128.f, 0.f);       // 80x16
const Vec2 CUIMgr::BOSS_HP_FILL_POS = Vec2(216.f, 8.f);      // 64x4

// === 화면 렌더링 위치 ===
const Vec2 CUIMgr::KIRBY_LIFE_SCREEN_POS = Vec2(272.f, 588.f); // 커비 생명 UI
const Vec2 CUIMgr::LIFE_COUNT_OFFSET = Vec2(64.f, 0.f);        // 생명수 숫자
const Vec2 CUIMgr::HEALTH_OFFSET = Vec2(160.f, 0.f);           // 체력 UI (32px 띄움)
const Vec2 CUIMgr::BOSS_HP_OFFSET = Vec2(528.f, 0.f);          // 보스 HP (32px 띄움)

// === 스프라이트 크기 상수 ===
const Vec2 CUIMgr::KIRBY_LIFE_ICON_SIZE = Vec2(24.f, 16.f);   // 24x16
const Vec2 CUIMgr::NUMBER_SIZE = Vec2(8.f, 16.f);             // 8x16
const Vec2 CUIMgr::HEALTH_SIZE = Vec2(8.f, 16.f);             // 8x16
const Vec2 CUIMgr::BOSS_HP_BAR_SIZE = Vec2(80.f, 16.f);       // 80x16
const Vec2 CUIMgr::BOSS_HP_FILL_SIZE = Vec2(64.f, 5.f);       // 64x5

const int CUIMgr::UI_SCALE = 4;                               // UI 4배 스케일

CUIMgr::CUIMgr()
    : m_pUITexture(nullptr)
    , m_bShowBossHP(false)
    , m_fBossHPFillAnimation(0.f)
    , m_bBossHPFillActive(false)
{
}

CUIMgr::~CUIMgr()
{
}

void CUIMgr::Init()
{
    // UI.bmp 텍스처 로드
    m_pUITexture = CResMgr::GetInst()->LoadTexture(L"UI", L"texture\\UI\\UI.bmp");
    if (!m_pUITexture)
    {
        MessageBox(nullptr, L"UI.bmp 로드에 실패했습니다.", L"오류", MB_OK);
    }

    // 보스 HP 바 초기 상태
    m_bShowBossHP = false;
    m_fBossHPFillAnimation = 0.f;
    m_bBossHPFillActive = false;
}

void CUIMgr::Update()
{
    // 보스 HP 바 채우기 애니메이션 업데이트
    if (m_bBossHPFillActive)
    {
        float fFillSpeed = 1.5f; // 1.5초에 걸쳐서 채움
        m_fBossHPFillAnimation += CTimeMgr::GetInst()->GetfDT() / fFillSpeed;
        
        if (m_fBossHPFillAnimation >= 1.f)
        {
            m_fBossHPFillAnimation = 1.f;
            m_bBossHPFillActive = false;
        }
    }
}

void CUIMgr::StartBossHPFillAnimation()
{
    m_fBossHPFillAnimation = 0.f;
    m_bBossHPFillActive = true;
    m_bShowBossHP = true;
    
    // HP바 채우기 효과음 재생
    CSoundMgr::GetInst()->PlaySFX(L"boss_HP_fill");
}

void CUIMgr::RenderGameUI(HDC _dc)
{
    if (!m_pUITexture)
        return;

    // 플레이어 데이터 수집
    CPlayer* pPlayer = FindPlayer();
    CBoss* pBoss = FindBoss();

    if (pPlayer)
    {
        // 커비 생명 수 UI 렌더링
        int iLives = CPlayerDataMgr::GetInst()->GetLives();
        RenderKirbyLifeUI(_dc, iLives);

        // 커비 체력 UI 렌더링
        CPlayerHealthSystem* pHealthSystem = pPlayer->GetHealthSystem();
        if (pHealthSystem)
        {
            RenderKirbyHealthUI(_dc, pHealthSystem->GetCurrentHP(), pHealthSystem->GetMaxHP());
        }
    }

    // 보스 체력 UI 렌더링 (보스가 있을 때만)
    if (pBoss)
    {
        RenderBossHealthUI(_dc, pBoss);
    }
}

void CUIMgr::RenderKirbyLifeUI(HDC _dc, int _iLives)
{
    Vec2 vPos = KIRBY_LIFE_SCREEN_POS;

    // 커비 초상화 + 곱하기 표시
    RenderSprite(_dc, KIRBY_LIFE_ICON_POS, KIRBY_LIFE_ICON_SIZE, 
                 vPos, KIRBY_LIFE_ICON_SIZE * UI_SCALE);

    // 생명 수를 2자리 숫자로 렌더링 (01, 02, 03...)
    Vec2 vNumberPos = vPos + LIFE_COUNT_OFFSET;
    
    // 10의 자리 숫자 (0~9)
    int tensDigit = _iLives / 10;
    RenderNumber(_dc, tensDigit, vNumberPos);
    
    // 1의 자리 숫자 (0~9)
    int onesDigit = _iLives % 10;
    Vec2 vOnesPos = vNumberPos + Vec2(NUMBER_SIZE.x * UI_SCALE, 0.f);
    RenderNumber(_dc, onesDigit, vOnesPos);
}

void CUIMgr::RenderKirbyHealthUI(HDC _dc, int _iCurrentHP, int _iMaxHP)
{
    Vec2 vPos = KIRBY_LIFE_SCREEN_POS + HEALTH_OFFSET;

    // 체력칸 6개 렌더링 (오른쪽부터 빈 체력칸으로 변경)
    for (int i = 0; i < _iMaxHP; ++i)
    {
        Vec2 vHealthPos = vPos + Vec2(i * HEALTH_SIZE.x * UI_SCALE, 0.f);
        // 인덱스를 역순으로 계산하여 오른쪽부터 빈칸으로
        bool bIsFull = (i + 1) <= _iCurrentHP;
        RenderHealthBar(_dc, 1, 1, vHealthPos, bIsFull);
    }
}

void CUIMgr::RenderBossHealthUI(HDC _dc, CBoss* _pBoss)
{
    // 보스 HP 바가 표시 상태가 아니면 렌더링하지 않음
    if (!m_bShowBossHP || !_pBoss || _pBoss->IsDefeated())
        return;

    Vec2 vPos = KIRBY_LIFE_SCREEN_POS + BOSS_HP_OFFSET;

    // 보스 체력바 배경 렌더링
    RenderSprite(_dc, BOSS_HP_BAR_POS, BOSS_HP_BAR_SIZE,
                 vPos, BOSS_HP_BAR_SIZE * UI_SCALE);

    // 보스 체력바 내부 렌더링 (애니메이션과 실제 HP 비율 모두 고려)
    float fHPRatio = _pBoss->GetHPRatio();
    float fDisplayRatio;
    
    if (m_bBossHPFillActive)
    {
        // 첫 연출 중: 애니메이션 진행도 * HP 비율 (HP바가 서서히 차오름)
        fDisplayRatio = fHPRatio * m_fBossHPFillAnimation;
    }
    else
    {
        // 연출 완료 후: 실제 HP 비율만 반영 (실시간 체력 변화)
        fDisplayRatio = fHPRatio;
    }
    
    if (fDisplayRatio > 0.f)
    {
        // 소스 이미지 크기 조절 (왼쪽 기준으로 오른쪽부터 줄어들도록)
        Vec2 vSrcFillSize = BOSS_HP_FILL_SIZE;
        vSrcFillSize.x *= fDisplayRatio; // HP 비율만큼 소스 이미지 가로 크기 조절

        // 목표 크기도 비율에 맞게 조절
        Vec2 vDestFillSize = vSrcFillSize * UI_SCALE;
        
        // 렌더링 위치 조정: 중심점 기준이므로 왼쪽 고정을 위해 위치를 조정
        Vec2 vFillPos = vPos + Vec2(0.f * UI_SCALE, 2.5f * UI_SCALE);
        
        // 원래 HP바 크기와 현재 HP바 크기의 차이만큼 왼쪽으로 이동
        float fSizeDiff = (BOSS_HP_FILL_SIZE.x - vSrcFillSize.x) * UI_SCALE / 2.f;
        vFillPos.x -= fSizeDiff; // 왼쪽으로 이동하여 왼쪽 끝을 고정

        RenderSprite(_dc, BOSS_HP_FILL_POS, vSrcFillSize,
                     vFillPos, vDestFillSize);
    }
}

void CUIMgr::RenderNumber(HDC _dc, int _iNumber, Vec2 _vPos)
{
    // 0~9 숫자만 지원
    if (_iNumber < 0 || _iNumber > 9)
        _iNumber = 0;

    Vec2 vSrcPos = NUMBER_0_POS + Vec2(_iNumber * NUMBER_SIZE.x, 0.f);
    RenderSprite(_dc, vSrcPos, NUMBER_SIZE, _vPos, NUMBER_SIZE * UI_SCALE);
}

void CUIMgr::RenderHealthBar(HDC _dc, int _iCurrent, int _iMax, Vec2 _vPos, bool _bIsFull)
{
    Vec2 vSrcPos = _bIsFull ? HEALTH_FULL_POS : HEALTH_EMPTY_POS;
    RenderSprite(_dc, vSrcPos, HEALTH_SIZE, _vPos, HEALTH_SIZE * UI_SCALE);
}

void CUIMgr::RenderSprite(HDC _dc, Vec2 _vSrcPos, Vec2 _vSrcSize, Vec2 _vDestPos, Vec2 _vDestSize)
{
    if (!m_pUITexture)
        return;

    m_pUITexture->RenderSpriteWithColorKey(_dc, _vDestPos, _vSrcPos, _vSrcSize, _vDestSize, RGB(255, 0, 255));
}

CPlayer* CUIMgr::FindPlayer()
{
    CScene* pCurScene = CSceneMgr::GetInst()->GetCurScene();
    if (!pCurScene)
        return nullptr;

    const vector<CObject*>& vecPlayer = pCurScene->GetGroupObject(GROUP_TYPE::PLAYER);
    if (vecPlayer.empty())
        return nullptr;

    return dynamic_cast<CPlayer*>(vecPlayer[0]);
}

CBoss* CUIMgr::FindBoss()
{
    CScene* pCurScene = CSceneMgr::GetInst()->GetCurScene();
    if (!pCurScene)
        return nullptr;

    const vector<CObject*>& vecMonster = pCurScene->GetGroupObject(GROUP_TYPE::MONSTER);
    for (CObject* pObj : vecMonster)
    {
        CBoss* pBoss = dynamic_cast<CBoss*>(pObj);
        if (pBoss)
            return pBoss;
    }

    return nullptr;
}