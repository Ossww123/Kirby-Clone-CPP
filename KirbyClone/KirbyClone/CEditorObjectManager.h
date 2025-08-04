#pragma once
#include "CObjectFactory.h"

class CEditorCore;
class CBackground;

class CEditorObjectManager
{
private:
    CEditorCore* m_pEditorCore;

    // 오브젝트 팩토리 관련
    OBJECT_TYPE         m_eCurrentObjectType;
    int                 m_iCurrentSubType;
    vector<OBJECT_TYPE> m_vecCurrentCategory;

    // 배경 시스템 관련
    CBackground* m_pCurrentBackground;
    BACKGROUND_TYPE     m_eCurrentBgType;
    vector<BACKGROUND_TYPE> m_vecBackgroundTypes;

    // 타일 시각적 타입 관련
    TILE_VISUAL_TYPE    m_eCurrentTileVisual;
    vector<TILE_VISUAL_TYPE> m_vecTileVisualTypes;
    int                 m_iTileVisualIndex;

    // 플레이어 스폰 관련
    Vec2                m_vPlayerSpawnPos;
    bool                m_bShowPlayerSpawn;
    bool                m_bPlayerSpawnMode;

public:
    void Initialize(CEditorCore* _pCore);
    void Update();  // 배경 업데이트 등

    // 오브젝트 생성/삭제
    void PlaceObject(Vec2 _vPos);
    void DeleteObjectAtPosition(Vec2 _vPos);
    CObject* FindObjectAtPosition(Vec2 _vPos);

    // 오브젝트 카테고리 관리
    void ChangeObjectCategory(const wstring& _strCategory);
    void NextObjectInCategory();
    void PrevObjectInCategory();
    const wchar_t* GetCurrentObjectName();
    void SetCurrentSubType(int index);

    // 배경 시스템 관리
    void InitializeBackgroundSystem();
    void ChangeBackground(BACKGROUND_TYPE _eBgType);
    void NextBackground();
    void PrevBackground();
    const wchar_t* GetBackgroundName(BACKGROUND_TYPE _eType);

    // 타일 시각적 타입 관리
    void InitializeTileVisualSystem();
    void NextTileVisual();
    void PrevTileVisual();
    const wchar_t* GetTileVisualName(TILE_VISUAL_TYPE _eType);

    // 레벨 관리 함수들
    void ClearAllObjects();           // 모든 오브젝트 삭제
    void ResetToDefault();            // 기본 설정으로 리셋
    int GetTotalObjectCount();        // 전체 오브젝트 수 반환

    // 플레이어 스폰 관련
    void SetPlayerSpawnPosition(Vec2 _vPos) { m_vPlayerSpawnPos = _vPos; }
    Vec2 GetPlayerSpawnPosition() { return m_vPlayerSpawnPos; }
    void SetShowPlayerSpawn(bool _bShow) { m_bShowPlayerSpawn = _bShow; }
    bool IsShowPlayerSpawn() { return m_bShowPlayerSpawn; }

    // 접근자들
    OBJECT_TYPE GetCurrentObjectType() { return m_eCurrentObjectType; }
    TILE_VISUAL_TYPE GetCurrentTileVisual() { return m_eCurrentTileVisual; }
    BACKGROUND_TYPE GetCurrentBackgroundType() { return m_eCurrentBgType; }
    CBackground* GetCurrentBackground() { return m_pCurrentBackground; }
    const vector<OBJECT_TYPE>& GetCurrentCategory() { return m_vecCurrentCategory; }
    int GetCurrentSubType() { return m_iCurrentSubType; }
    const vector<BACKGROUND_TYPE>& GetBackgroundTypes() { return m_vecBackgroundTypes; }
    const vector<TILE_VISUAL_TYPE>& GetTileVisualTypes() { return m_vecTileVisualTypes; }
    int GetTileVisualIndex() { return m_iTileVisualIndex; }
    Vec2 GetPlayerSpawnPos() { return m_vPlayerSpawnPos; }

public:
    CEditorObjectManager();
    ~CEditorObjectManager();
};