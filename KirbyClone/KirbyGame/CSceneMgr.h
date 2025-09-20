#pragma once

class CScene;

class CSceneMgr
{
    SINGLE(CSceneMgr);

    // EventMgr에서 ChangeScene 함수 접근 허용
    friend class CEventMgr;

public:
    // === 핵심 생명주기 함수들 ===
    void init();
    void update();
    void render(HDC _dc);

private:
    // === 씬 전환 내부 구현 ===
    void ChangeScene(SCENE_TYPE _eNext);        // 실제 씬 전환 로직
    void ApplySceneResolution(SCENE_TYPE _eSceneType);  // 씬별 해상도 설정
    void HandleGlobalSceneTransition();         // 전역 키 입력 처리

public:
    // === Getter 함수들 ===
    CScene* GetCurScene() const  { return m_pCurScene; }
    SCENE_TYPE GetCurSceneType() const { return m_eCurSceneType; }

private:
    // === 멤버 변수들 ===
    CScene* m_arrScene[(UINT)SCENE_TYPE::END];  // 모든 씬 배열
    CScene* m_pCurScene;                        // 현재 활성화된 씬
    SCENE_TYPE m_eCurSceneType;                 // 현재 씬 타입
};