#pragma once

class CObject;
class CWaddleDee;
class CWaddleDoo;
class CBrontoBurt;
class CGordo;
class CHotHead;
class CSparky;
class CWhispyWoods;
class CApple;

class CMonsterFactory
{
private:
    // 팩토리는 정적 클래스로 사용
    CMonsterFactory() = delete;
    ~CMonsterFactory() = delete;

public:
    // 몬스터 생성 함수
    static CObject* CreateMonster(OBJECT_TYPE _eMonsterType, Vec2 _vPos);

    // === 몬스터별 세부 생성 함수들 ===
    static CWaddleDee* CreateWaddleDee(Vec2 _vPos);
    static CWaddleDoo* CreateWaddleDoo(Vec2 _vPos);
    static CBrontoBurt* CreateBrontoBurt(Vec2 _vPos);
    static CGordo* CreateGordo(Vec2 _vPos);
    static CHotHead* CreateHotHead(Vec2 _vPos);
    static CSparky* CreateSparky(Vec2 _vPos);
    static CWhispyWoods* CreateWhispyWoods(Vec2 _vPos);
    static CApple* CreateApple(Vec2 _vPos);

    // 몬스터 관련 유틸리티 함수들
    static const wchar_t* GetMonsterTypeName(OBJECT_TYPE _eType);
    static GROUP_TYPE GetMonsterGroup(OBJECT_TYPE _eType);
    static Vec2 GetMonsterDefaultScale(OBJECT_TYPE _eType);
    static bool IsMonsterType(OBJECT_TYPE _eType);

private:
    // 내부 설정 함수
    static void SetupMonsterAI(CObject* _pMonster, OBJECT_TYPE _eType);
};

