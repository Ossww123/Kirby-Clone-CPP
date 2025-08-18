#pragma once

class CObject;
class CWaddleDee;
class CWaddleDoo;
class CBrontoBurt;
class CGordo;
class CHotHead;
class CSparky;
class CWhispyWoods;

class CObjectFactory
{
private:
    // 팩토리는 정적 클래스로 사용
    CObjectFactory ( ) = delete;
    ~CObjectFactory ( ) = delete;

public:
    // 통합 팩토리 함수
    static CObject* CreateObject ( OBJECT_TYPE _eType , Vec2 _vPos = Vec2 ( 0.f , 0.f ) );

    // 카테고리별 생성 함수들
    static CObject* CreatePlayer ( Vec2 _vPos );
    static CObject* CreateMonster ( OBJECT_TYPE _eMonsterType , Vec2 _vPos );
    static CObject* CreateItem ( OBJECT_TYPE _eItemType , Vec2 _vPos );
    static CObject* CreateTile ( OBJECT_TYPE _eTileType , Vec2 _vPos );
    static CObject* CreateSpecialObject ( OBJECT_TYPE _eObjectType , Vec2 _vPos );

    // === 몬스터별 세부 생성 함수들 ===
    static CWaddleDee* CreateWaddleDee ( Vec2 _vPos );
    static CWaddleDoo* CreateWaddleDoo ( Vec2 _vPos );
    static CBrontoBurt* CreateBrontoBurt ( Vec2 _vPos );
    static CGordo* CreateGordo ( Vec2 _vPos );
    static CHotHead* CreateHotHead ( Vec2 _vPos );
    static CSparky* CreateSparky ( Vec2 _vPos );
    static CWhispyWoods* CreateWhispyWoods ( Vec2 _vPos );


    // 유틸리티 함수들
    static const wchar_t* GetObjectTypeName ( OBJECT_TYPE _eType );
    static GROUP_TYPE GetObjectGroup ( OBJECT_TYPE _eType );
    static Vec2 GetDefaultScale ( OBJECT_TYPE _eType );
    static bool IsValidObjectType ( OBJECT_TYPE _eType );

    // 에디터용 함수들
    static vector<OBJECT_TYPE> GetObjectTypesByCategory ( const wstring& _strCategory );
    static vector<wstring> GetAvailableCategories ( );

    // 충돌체 시스템 타입 변환 함수들
    static COLLISION_TYPE ConvertObjectTypeToCollisionType ( OBJECT_TYPE _eObjectType );
    static OBJECT_TYPE ConvertCollisionTypeToObjectType ( COLLISION_TYPE _eCollisionType );

    // 충돌체 타입 정보 유틸리티
    static const wchar_t* GetCollisionTypeName ( COLLISION_TYPE _eType );
    static COLORREF GetCollisionTypeColor ( COLLISION_TYPE _eType );
    static vector<COLLISION_TYPE> GetAvailableCollisionTypes ( );

private:
    // 내부 설정 함수들
    static void SetupMonsterAI ( CObject* _pMonster , OBJECT_TYPE _eType );
    static void SetupItemProperties ( CObject* _pItem , OBJECT_TYPE _eType );
    static void SetupTileProperties ( CObject* _pTile , OBJECT_TYPE _eType );
};