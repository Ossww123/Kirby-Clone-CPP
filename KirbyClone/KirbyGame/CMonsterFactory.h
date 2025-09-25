#pragma once
#include <memory>
#include <functional>
#include <unordered_map>
#include <vector>
#include <string>

#include "CMonster.h"         // CMonster, MONSTER_STATE 등
#include "CObject.h"          // Vec2, CObject

// 전방 선언 (CBasicMonster의 설정 구조체)
struct BasicMonsterConfig;

// 생성 타입(필요시 계속 추가)
enum class MONSTER_KIND : unsigned char
{
    WADDLE_DEE,
    BRONTO_BURT,
    WADDLE_DOO,
    HOT_HEAD,  
    SPARKY,



    WHISPY_WOODS,
    // … add more
};


class CMonsterFactory
{
public:
    // === 주요 생성 API ===
    // 1) enum 기반
    static CMonster* Create(MONSTER_KIND kind,
        const Vec2& pos,
        CObject* pTarget = nullptr,
        const BasicMonsterConfig* cfg = nullptr);

    // 2) 문자열 이름 기반 (대소문자/언더스코어 무시)
    static CMonster* Create(const std::wstring& name,
        const Vec2& pos,
        CObject* pTarget = nullptr,
        const BasicMonsterConfig* cfg = nullptr);

    // 3) unique_ptr 버전이 필요하면 이걸 사용 (엔진이 스마트포인터를 받는다면)
    static std::unique_ptr<CMonster> CreateUnique(MONSTER_KIND kind,
        const Vec2& pos,
        CObject* pTarget = nullptr,
        const BasicMonsterConfig* cfg = nullptr);

    static std::unique_ptr<CMonster> CreateUnique(const std::wstring& name,
        const Vec2& pos,
        CObject* pTarget = nullptr,
        const BasicMonsterConfig* cfg = nullptr);

    // === 커스텀 등록 (모드/에디터 확장용) ===
    using CreateFn = std::function<CMonster* ()>;
    static void Register(const std::wstring& name, CreateFn fn); // 이름 기반 등록
    static std::vector<std::wstring> RegisteredNames();

private:
    static void EnsureDefaults();              // 기본 몬스터 등록
    static std::wstring Normalize(const std::wstring& s);

private:
    // 이름 레지스트리 (정규화된 소문자 키)
    static std::unordered_map<std::wstring, CreateFn>& Registry();
};
