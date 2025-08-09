#pragma once

class CObject;

class CObjectFactory
{
private:
    // 팩토리는 정적 클래스로 사용
    CObjectFactory() = delete;
    ~CObjectFactory() = delete;

public:
    // 메인 팩토리 함수
    static CObject* CreateObject(OBJECT_TYPE _eType, Vec2 _vPos = Vec2(0.f, 0.f));

    // 카테고리별 생성 함수들
    static CObject* CreatePlayer(Vec2 _vPos);
    static CObject* CreateMonster(OBJECT_TYPE _eMonsterType, Vec2 _vPos);
    static CObject* CreateItem(OBJECT_TYPE _eItemType, Vec2 _vPos);
    static CObject* CreateTile(OBJECT_TYPE _eTileType, Vec2 _vPos);
    static CObject* CreateSpecialObject(OBJECT_TYPE _eObjectType, Vec2 _vPos);

    // 유틸리티 함수들
    static const wchar_t* GetObjectTypeName(OBJECT_TYPE _eType);
    static GROUP_TYPE GetObjectGroup(OBJECT_TYPE _eType);
    static Vec2 GetDefaultScale(OBJECT_TYPE _eType);
    static bool IsValidObjectType(OBJECT_TYPE _eType);

    // 에디터용 함수들
    static vector<OBJECT_TYPE> GetObjectTypesByCategory(const wstring& _strCategory);
    static vector<wstring> GetAvailableCategories();

    // 새로 추가: 충돌체 시스템 관련 변환 함수들
    static COLLISION_TYPE ConvertObjectTypeToCollisionType(OBJECT_TYPE _eObjectType);
    static OBJECT_TYPE ConvertCollisionTypeToObjectType(COLLISION_TYPE _eCollisionType);

    // 새로 추가: 충돌체 타입 관련 유틸리티
    static const wchar_t* GetCollisionTypeName(COLLISION_TYPE _eType);
    static COLORREF GetCollisionTypeColor(COLLISION_TYPE _eType);
    static vector<COLLISION_TYPE> GetAvailableCollisionTypes();

private:
    // 내부 헬퍼 함수들
    static void SetupMonsterAI(CObject* _pMonster, OBJECT_TYPE _eType);
    static void SetupItemProperties(CObject* _pItem, OBJECT_TYPE _eType);
    static void SetupTileProperties(CObject* _pTile, OBJECT_TYPE _eType);
};