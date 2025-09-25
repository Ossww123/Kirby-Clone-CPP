#include "gamePCH.h"
#include "CMonsterFactory.h"

#include "CBasicMonster.h"
#include "CWaddleDee.h"
#include "CBrontoBurt.h"
#include "CWaddleDoo.h"
#include "CHotHead.h"
#include "CSparky.h"
#include "CWhispyWoods.h"

#include <algorithm>
#include <cwctype>

// ----- 내부 헬퍼 -----
static bool& s_inited()
{
    static bool inited = false;
    return inited;
}

std::unordered_map<std::wstring, CMonsterFactory::CreateFn>& CMonsterFactory::Registry()
{
    static std::unordered_map<std::wstring, CreateFn> reg;
    return reg;
}

std::wstring CMonsterFactory::Normalize(const std::wstring& s)
{
    std::wstring out; out.reserve(s.size());
    for (wchar_t ch : s)
    {
        if (ch == L'_' || ch == L' ' || ch == L'-') continue; // 구분자 제거
        out.push_back(std::towlower(ch));
    }
    return out;
}

void CMonsterFactory::EnsureDefaults()
{
    if (s_inited()) return;
    s_inited() = true;

    // 기본 등록: 여러 별칭을 같은 생성자에 맵핑
    auto& reg = Registry();

    auto regAll = [&](std::initializer_list<const wchar_t*> names, CreateFn fn)
        {
            for (auto* n : names) reg[Normalize(n)] = fn;
        };

    regAll({ L"WaddleDee", L"Waddle_Dee", L"waddledee" }, []() { return new CWaddleDee(); });
    regAll({ L"BrontoBurt", L"Bronto_Burt", L"brontoburt" }, []() { return new CBrontoBurt(); });
    regAll({ L"WaddleDoo", L"Waddle_Doo", L"waddledoo" }, []() { return new CWaddleDoo(); });
    regAll({ L"HotHead",   L"Hot_Head",   L"hothead" }, []() { return new CHotHead(); });
    regAll({ L"Sparky",    L"sparky" }, []() { return new CSparky(); });
    regAll({ L"WhispyWoods", L"Whispy_Woods", L"whispywoods" }, []() { return new CWhispyWoods(); });
}

// ----- 공용 API -----

void CMonsterFactory::Register(const std::wstring& name, CreateFn fn)
{
    EnsureDefaults();
    Registry()[Normalize(name)] = std::move(fn);
}

std::vector<std::wstring> CMonsterFactory::RegisteredNames()
{
    EnsureDefaults();
    std::vector<std::wstring> v;
    v.reserve(Registry().size());
    for (auto& kv : Registry()) v.push_back(kv.first);
    return v;
}

// enum 기반
CMonster* CMonsterFactory::Create(MONSTER_KIND kind,
    const Vec2& pos,
    CObject* pTarget,
    const BasicMonsterConfig* cfg)
{
    EnsureDefaults();

    CMonster* obj = nullptr;
    switch (kind)
    {
    case MONSTER_KIND::WADDLE_DEE:  obj = new CWaddleDee();  break;
    case MONSTER_KIND::BRONTO_BURT: obj = new CBrontoBurt(); break;
    case MONSTER_KIND::WADDLE_DOO:  obj = new CWaddleDoo(); break;
    case MONSTER_KIND::HOT_HEAD:    obj = new CHotHead();   break;
    case MONSTER_KIND::SPARKY:      obj = new CSparky();    break;
    case MONSTER_KIND::WHISPY_WOODS: obj = new CWhispyWoods(); break;
    default:                        obj = nullptr;           break;
    }

    if (!obj) return nullptr;

    // 공통 초기화
    obj->SetPos(pos);

    // 타깃/설정 적용 (CBasicMonster에만 해당)
    if (pTarget)
    {
        if (auto* basic = dynamic_cast<CBasicMonster*>(obj))
            basic->SetTarget(pTarget);
    }
    if (cfg)
    {
        if (auto* basic = dynamic_cast<CBasicMonster*>(obj))
            basic->SetConfig(*cfg);
    }

    return obj;
}

// 문자열 기반
CMonster* CMonsterFactory::Create(const std::wstring& name,
    const Vec2& pos,
    CObject* pTarget,
    const BasicMonsterConfig* cfg)
{
    EnsureDefaults();

    const auto key = Normalize(name);
    auto it = Registry().find(key);
    if (it == Registry().end()) return nullptr;

    CMonster* obj = it->second();
    if (!obj) return nullptr;

    obj->SetPos(pos);

    if (pTarget)
    {
        if (auto* basic = dynamic_cast<CBasicMonster*>(obj))
            basic->SetTarget(pTarget);
    }
    if (cfg)
    {
        if (auto* basic = dynamic_cast<CBasicMonster*>(obj))
            basic->SetConfig(*cfg);
    }

    return obj;
}

// unique_ptr 버전들
std::unique_ptr<CMonster> CMonsterFactory::CreateUnique(MONSTER_KIND kind,
    const Vec2& pos,
    CObject* pTarget,
    const BasicMonsterConfig* cfg)
{
    return std::unique_ptr<CMonster>(Create(kind, pos, pTarget, cfg));
}

std::unique_ptr<CMonster> CMonsterFactory::CreateUnique(const std::wstring& name,
    const Vec2& pos,
    CObject* pTarget,
    const BasicMonsterConfig* cfg)
{
    return std::unique_ptr<CMonster>(Create(name, pos, pTarget, cfg));
}
