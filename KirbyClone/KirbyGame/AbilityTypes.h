#pragma once
#include <cstdint>

// === Copy Ability 식별자 ===
enum class AbilityGift : std::uint8_t {
    None, Fire, Beam, Spark, Cutter, Sword, Ice, Bomb
};

// === 흡입/삼킴 파이프라인에 전달되는 토큰 ===
struct AbilitySourceToken {
    AbilityGift gift{ AbilityGift::None };
    int         potency{ 1 };
};

// === 흡입 가능한 대상 공통 인터페이스 ===
class IInhalable {
public:
    virtual ~IInhalable() = default;
    virtual bool IsInhalable() const = 0;
    virtual AbilitySourceToken GetAbilityToken() const = 0;
    virtual void OnInhaledStart() {}   // 빨려가기 시작
    virtual void OnSwallowed() {}   // 완전 삼킴
    virtual void OnSpatOut() {}   // 별로 뱉힘
};
