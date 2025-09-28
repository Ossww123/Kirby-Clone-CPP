#include "gamePCH.h"
#include "CSceneMgr.h"
#include "CCollisionMgr.h"
#include "CKeyMgr.h"
#include "CEventMgr.h"

#include "CCore.h"
#include "CScene.h"
#include "CScene_Start.h"
// #include "CScene_Stage01.h"   // 제거
// #include "CScene_Stage02.h"   // 제거
#include "CStageScene.h"        // 추가
#include "CKirby.h"             // 단축키 테스트용 스폰 좌표에 쓸 수도 있음

CSceneMgr::CSceneMgr()
    : m_pCurScene(nullptr)
    , m_arrScene{}
    , m_eCurSceneType(SCENE_TYPE::START)
{
}

CSceneMgr::~CSceneMgr()
{
    for (UINT i = 0; i < (UINT)SCENE_TYPE::END; ++i)
    {
        if (m_arrScene[i]) { delete m_arrScene[i]; m_arrScene[i] = nullptr; }
    }
}

void CSceneMgr::init()
{
    // ── 씬 슬롯 구성 ───────────────────────────────────────────
    m_arrScene[(UINT)SCENE_TYPE::START] = new CScene_Start;
    // m_arrScene[(UINT)SCENE_TYPE::TOOL] = new CScene_Tool;

    // 중요: “스테이지”는 단 하나의 데이터 주도 씬만 둔다.
    // 이미 enum에 STAGE_01, STAGE_02 등이 있더라도, 실제 인스턴스는 딱 하나만 생성.
    // 안전하게 STAGE_01 슬롯을 '공용 스테이지'로 사용한다.
    m_arrScene[(UINT)SCENE_TYPE::STAGE_01] = (CScene*)new CStageScene;

    // 시작 씬 활성화
    m_eCurSceneType = SCENE_TYPE::START;
    m_pCurScene = m_arrScene[(UINT)m_eCurSceneType];
    if (m_pCurScene) m_pCurScene->Enter();
}

void CSceneMgr::update()
{
    if (m_pCurScene) m_pCurScene->Update();
    HandleGlobalSceneTransition();
}

void CSceneMgr::render(HDC _dc)
{
    if (m_pCurScene) m_pCurScene->Render(_dc);
}

void CSceneMgr::ChangeScene(SCENE_TYPE _eNext)
{
    if (m_pCurScene) m_pCurScene->Exit();

    m_eCurSceneType = _eNext;
    m_pCurScene = m_arrScene[(UINT)_eNext];

    CCollisionMgr::GetInst()->ClearCollisionPairs();

    if (m_pCurScene) m_pCurScene->Enter();
}

// 데이터 기반 스테이지 전환
void CSceneMgr::ChangeStage(const StageDesc& desc)
{
    // 공용 스테이지 씬 인스턴스 확보
    CStageScene* stage = nullptr;

    // 1) STAGE_01 슬롯을 '공용 스테이지'로 사용 (enum 변경 전 과도기 대책)
    stage = dynamic_cast<CStageScene*>(m_arrScene[(UINT)SCENE_TYPE::STAGE_01]);

    // 2) 없으면 생성(방어)
    if (!stage) {
        stage = new CStageScene;
        m_arrScene[(UINT)SCENE_TYPE::STAGE_01] = stage;
    }

    // 현재 씬이 스테이지가 아니면, 씬 전환 준비
    const bool switchingFromOtherScene = (m_pCurScene != stage);
    if (switchingFromOtherScene && m_pCurScene) m_pCurScene->Exit();

    // 충돌 상태 초기화(씬 갈아탈 때나 스테이지 재로드 때 모두 안전)
    CCollisionMgr::GetInst()->ClearCollisionPairs();

    // 스테이지 데이터 로딩 (타일/스폰 전부 세팅)
    stage->LoadStage(desc);

    // 포인터/타입 갱신
    m_eCurSceneType = SCENE_TYPE::STAGE_01; // 공용 슬롯 사용
    m_pCurScene = stage;

    // 씬 진입 (다른 씬에서 넘어온 경우에만 호출)
    if (switchingFromOtherScene) stage->Enter();
}

void CSceneMgr::HandleGlobalSceneTransition()
{
    // ── 예시 단축키: Ctrl+0 → START 씬 ────────────────────────
    if (KEY_TAP(KEY::ALPHA_0) && KEY_HOLD(KEY::CTRL))
    {
        ChangeScene(SCENE_TYPE::START);
        return;
    }

    // ── 예시 단축키: Ctrl+1 → stage01 데이터 로드 ──────────────
    if (KEY_TAP(KEY::ALPHA_1) && KEY_HOLD(KEY::CTRL))
    {
        StageDesc stage01{};
        stage01.name = L"stage01";
        stage01.tileSize = Vec2{ 16,16 };
        stage01.originLT = Vec2{ 0,0 };

        // 커비 스폰
        stage01.kirbySpawn = Vec2{ 64, 64 };

        // 몬스터(예시)
        stage01.monsters = {
            {L"WaddleDee", Vec2{ 200,  64}},
            {L"HotHead",   Vec2{ 500,  64}},
            {L"Sparky",    Vec2{ 750,  64}},
        };

        // (선택) 보스 트리거
        // stage01.bossTriggers = {
        //     { Vec2{1200, 256}, Vec2{32, 64}, Vec2{1280, 240} }
        // };

        CSceneMgr::GetInst()->ChangeStage(stage01);
        return;
    }

    // ── 예시 단축키: Ctrl+2 → stage02 데이터 로드 ──────────────
    if (KEY_TAP(KEY::ALPHA_2) && KEY_HOLD(KEY::CTRL))
    {
        StageDesc stage02{};
        stage02.name = L"stage02";
        stage02.tileSize = Vec2{ 16,16 };
        stage02.originLT = Vec2{ 0,0 };
        stage02.kirbySpawn = Vec2{ 96,64 };
        // TODO: tilesetJson/tilemapCsv 채우기
        // TODO: monsters/triggers 채우기

        ChangeStage(stage02);
        return;
    }
}
