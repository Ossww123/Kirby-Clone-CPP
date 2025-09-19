#pragma once
#include "CObjectFactory.h"

class CEditorCore;
class CBackground;

class CEditorObjectManager
{
public:
    // === 생명주기 함수 ===
    CEditorObjectManager();
    ~CEditorObjectManager();

public:
    // === 핵심 생명주기 함수 ===
    void Initialize(CEditorCore* _pCore);
    void Update();

public:
    // === 오브젝트 배치/삭제 관리 ===
    void PlaceObject(Vec2 _vPos);
    void DeleteObjectAtPos(Vec2 _vPos);
    CObject* FindObjectAtPos(Vec2 _vPos);

public:
    // === 오브젝트 카테고리 관리 ===
    void ChangeObjectCategory(const wstring& _strCategory);
    void NextObjectInCategory();
    void PrevObjectInCategory();
    void SetCurrentSubType(int index);

public:
    // === 배경 시스템 관리 ===
    void ChangeBackground(BACKGROUND_TYPE _eBgType);
    void NextBackground();
    void PrevBackground();
    void SetCurrentBackgroundType(BACKGROUND_TYPE _eCurrentBgType) { m_eCurrentBgType = _eCurrentBgType; }

private:
    // === 배경 시스템 내부 함수 ===
    void InitializeBackgroundSystem();

public:
    // === 타일 시각적 타입 관리 ===
    void NextTileVisual();
    void PrevTileVisual();

private:
    // === 타일 시스템 내부 함수 ===
    void InitializeTileVisualSystem();

public:
    // === 레벨 관리 함수들 ===
    void ClearAllObjects();           // 모든 오브젝트 삭제
    void ResetToDefault();            // 기본 설정으로 리셋

public:
    // === 플레이어 스폰 관리 ===
    void SetPlayerSpawnPos(Vec2 _vPos) { m_vPlayerSpawnPos = _vPos; }
    void SetShowPlayerSpawn(bool _bShow) { m_bShowPlayerSpawn = _bShow; }

public:
    // === Getter 함수들 ===
    OBJECT_TYPE GetCurrentObjectType() const { return m_eCurrentObjectType; }
    TILE_VISUAL_TYPE GetCurrentTileVisual() const { return m_eCurrentTileVisual; }
    BACKGROUND_TYPE GetCurrentBackgroundType() const { return m_eCurrentBgType; }
    CBackground* GetCurrentBackground() const { return m_pCurrentBackground; }

    const vector<OBJECT_TYPE>& GetCurrentCategory() const { return m_vecCurrentCategory; }
    int GetCurrentSubType() const { return m_iCurrentSubType; }
    const vector<BACKGROUND_TYPE>& GetBackgroundTypes() const { return m_vecBackgroundTypes; }
    const vector<TILE_VISUAL_TYPE>& GetTileVisualTypes() const { return m_vecTileVisualTypes; }

    int GetTileVisualIndex() const { return m_iTileVisualIndex; }
    Vec2 GetPlayerSpawnPos() const { return m_vPlayerSpawnPos; }
    bool IsShowPlayerSpawn() const { return m_bShowPlayerSpawn; }

    int GetTotalObjectCount() const;        // 전체 오브젝트 수 반환
    const wchar_t* GetCurrentObjectName() const;
    const wchar_t* GetBackgroundName(BACKGROUND_TYPE _eType) const;
    const wchar_t* GetTileVisualName(TILE_VISUAL_TYPE _eType) const;

private:
    // === 멤버 변수들 ===

    // === 에디터 코어 참조 ===
    CEditorCore* m_pEditorCore;     // 에디터 코어 참조

    // === 오브젝트 팩토리 관련 ===
    OBJECT_TYPE         m_eCurrentObjectType;  // 현재 선택된 오브젝트 타입
    int                 m_iCurrentSubType;     // 현재 서브 타입 인덱스
    vector<OBJECT_TYPE> m_vecCurrentCategory;  // 현재 카테고리의 오브젝트 목록

    // === 배경 시스템 관련 ===
    CBackground* m_pCurrentBackground;   // 현재 배경 객체
    BACKGROUND_TYPE             m_eCurrentBgType;       // 현재 배경 타입
    vector<BACKGROUND_TYPE>     m_vecBackgroundTypes;   // 사용 가능한 배경 타입들

    // === 타일 시각적 타입 관련 ===
    TILE_VISUAL_TYPE            m_eCurrentTileVisual;   // 현재 타일 시각적 타입
    vector<TILE_VISUAL_TYPE>    m_vecTileVisualTypes;   // 사용 가능한 타일 시각적 타입들
    int                         m_iTileVisualIndex;     // 타일 시각적 타입 인덱스

    // === 플레이어 스폰 관련 ===
    Vec2                m_vPlayerSpawnPos;      // 플레이어 스폰 위치
    bool                m_bShowPlayerSpawn;     // 플레이어 스폰 표시 여부
    bool                m_bPlayerSpawnMode;     // 플레이어 스폰 모드 여부
};