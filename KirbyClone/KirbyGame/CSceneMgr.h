#pragma once

class CScene;

class CSceneMgr
{
    SINGLE(CSceneMgr);

public:
    // === 핵심 생명주기 함수들 ===
    void init();
    void update();
    void render(HDC _dc);

    // 변경: SceneChangeSystem에서 호출할 공개 API
    void ChangeScene(SCENE_TYPE _eNext);        // 실제 씬 전환 로직

    // === Getter ===
    CScene* GetCurScene() const { return m_pCurScene; }
    SCENE_TYPE GetCurSceneType() const { return m_eCurSceneType; }

private:
    void HandleGlobalSceneTransition();                 // 전역 키 입력 처리

private:
    CScene* m_arrScene[(UINT)SCENE_TYPE::END]{}; // 모든 씬 배열
    CScene* m_pCurScene{ nullptr };               // 현재 활성화된 씬
    SCENE_TYPE m_eCurSceneType{ SCENE_TYPE::START }; // 현재 씬 타입
};
