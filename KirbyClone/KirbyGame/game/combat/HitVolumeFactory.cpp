#include "game/HitVolumeFactory.h"

namespace game {

    std::unordered_map<std::string , HitVolume::Cfg>& HitVolumeFactory::Registry ( ) {
        static std::unordered_map<std::string , HitVolume::Cfg> R;
        return R;
    }

    void HitVolumeFactory::Register ( const std::string& id , const HitVolume::Cfg& cfg ) {
        if ( id.empty ( ) ) return;
        Registry ( )[ id ] = cfg;
    }

    const HitVolume::Cfg* HitVolumeFactory::Find ( const std::string& id ) {
        auto& R = Registry ( );
        auto it = R.find ( id );
        return ( it == R.end ( ) ) ? nullptr : &it->second;
    }

    void HitVolumeFactory::RegisterDefaults ( ) {
        // Spark aura: circle centered at owner, ticks every 120ms
        {
            HitVolume::Cfg c{};
            c.behavior = HitBehavior::AreaPulse;
            c.shape = HitShape::Circle;
            c.r = 46.f;
            c.ttl = 0.90f;
            c.armTime = 0.04f;
            c.tickIntervalMs = 120;
            c.followFacing = true;
            c.localOffset = { 0.f, 0.f };
            c.payload.effect = HitEffect::Damage;
            c.payload.damage = 1;
            c.payload.knockback = { 80.f, -60.f };
            c.perTargetOnce = false;
            c.excludeOwner = true;
            Register ( "SparkAura" , c );
        }

        // Beam sweep: capsule sweeping from -40deg to +55deg over ~0.28s
        {
            HitVolume::Cfg c{};
            c.behavior = HitBehavior::MeleeArc;
            c.shape = HitShape::Capsule;
            c.len = 300.f;
            c.thick = 100.f;
            c.startDeg = -40.f;
            c.endDeg = +55.f;
            c.sweepDuration = 0.28f;
            c.ttl = 0.30f;  // little margin beyond sweep
            c.armTime = 0.04f;
            c.followFacing = true;
            c.localOffset = { 10.f, -6.f }; // from player's hand, tweak later
            c.payload.effect = HitEffect::Damage;
            c.payload.damage = 2;
            c.payload.knockback = { 260.f, -90.f };
            c.perTargetOnce = true; // single hit per swing
            c.excludeOwner = true;
            Register ( "BeamSweep" , c );
        }

        // Inhale field: attached box at Kirby's mouth; capture-only; short-lived (재스폰 전제)
        {
            HitVolume::Cfg c{};
            c.behavior = HitBehavior::Attached;
            c.shape = HitShape::Box;
            c.w = 48.f; c.h = 20.f;
            c.ttl = 0.08f;      // 흡입 중 프레임마다 스폰해도 누적 안 됨
            c.armTime = 0.f;
            c.followFacing = true;
            c.localOffset = { 18.f, -2.f }; // 입 위치(튜닝 지점)
            c.payload.effect = HitEffect::Capture;
            c.payload.gift = Ability::None;  // 선물은 타깃 메타에서 결정
            c.perTargetOnce = true;          // 한 번만 캡처
            c.excludeOwner = true;          // 자가 히트 방지
            Register ( "InhaleField" , c );
        }
    }

} // namespace game
