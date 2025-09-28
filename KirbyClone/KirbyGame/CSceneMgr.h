#pragma once

class CScene;
class CStageScene;   // 
struct StageDesc;    // 데이터 씬에 전달할 스테이지 설명자

class CSceneMgr
{
    SINGLE(CSceneMgr);

public:
    // === 핵심 생명주기 함수들 ===
    void init();
    void update();
    void render(HDC _dc);

    // 씬 타입 전환 (Start, Tool 등)
    void ChangeScene(SCENE_TYPE _eNext);

    // 데이터로 스테이지 로드 + 전환
    void ChangeStage(const StageDesc& desc);

    // === Getter ===
    CScene* GetCurScene() const { return m_pCurScene; }
    SCENE_TYPE GetCurSceneType() const { return m_eCurSceneType; }

private:
    void HandleGlobalSceneTransition();

private:
    CScene* m_arrScene[(UINT)SCENE_TYPE::END]{}; // 씬 슬롯
    CScene* m_pCurScene{ nullptr };
    SCENE_TYPE  m_eCurSceneType{ SCENE_TYPE::START };
};
