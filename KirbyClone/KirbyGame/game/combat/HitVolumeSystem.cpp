//
// Responsibility: Implementation of spawn/update/collision/event emission for HitVolumes.
// Non-Goals:      External synchronization; file I/O; editor tooling.
// Call-Context:   Main thread only.
//
#include <algorithm>
#include <cmath>

#include "engine/platform/win32/ColorUtil.h"     // debug color helper (cpp 한정 Win32)
#include "engine/render/IDebugDraw.h"
#include "engine/physics/AABB.h"           // Overlap*, Expand, etc.
#include "game/combat/HitVolumeGeom.h"
#include "game/debug/HitVolumeDebugDraw.h"
#include "game/combat/HitVolumeSystem.h"
#include "game/combat/HitVolumeFactory.h"        // archetypes
#include "game/combat/Ability.h"

namespace game {

    // ---------- local helpers (cpp-only) ----------
    namespace {
        inline float Lerp ( float a , float b , float t ) { return a + ( b - a ) * t; }
        inline float DegToRad ( float d ) { return d * 3.1415926535f / 180.f; }
        inline float clampf ( float v , float lo , float hi ) { return v < lo ? lo : ( v > hi ? hi : v ); }
    } // namespace (helpers)

    // ---------- HitVolumeSystem ----------
    int HitVolumeSystem::Spawn ( const SpawnDesc& s ) {
        const HitVolume::Cfg* def = HitVolumeFactory::Find ( s.archetype );
        if ( !def ) return -1;

        // Copy preset, then apply optional overrides (position policy lives at spawn site)
        HitVolume::Cfg cfg = *def;
        if ( s.overrideOffset )       cfg.localOffset = s.localOffset;
        if ( s.overrideFollowFacing ) cfg.followFacing = s.followFacing;

        auto hv = std::make_unique<HitVolume> ( s.ownerId , s.ownerFacing , s.worldAnchor , cfg );

        Slot slot;
        slot.id = m_nextId++;
        slot.hv = std::move ( hv );
        m_vols.emplace_back ( std::move ( slot ) );
        return m_vols.back ( ).id;
    }

    void HitVolumeSystem::Step ( double fixedDt , const std::vector<Target>& targets ) {
        // 1) Update volumes / follow owners
        for ( auto& s : m_vols ) {
            auto& hv = *s.hv;

            // follow owner (anchor + facing)
            if ( m_locateOwner ) {
                engine::Vec2 pos; int fac = hv.Facing ( );
                if ( m_locateOwner ( hv.Owner ( ) , pos , fac ) ) {
                    const auto& cfg = hv.GetCfg ( );
                    engine::Vec2 anchor = pos;
                    if ( cfg.followFacing ) anchor.x += cfg.localOffset.x * ( fac >= 0 ? 1.f : -1.f );
                    else                  anchor.x += cfg.localOffset.x;
                    anchor.y += cfg.localOffset.y;
                    hv.SetAnchor ( anchor );
                    hv.SetFacing ( fac );
                }
            }

            // NOTE: HitVolume::Update doesn't deref input; pass a dummy reference.
            hv.Advance ( ( float ) fixedDt );;
        }

        // 2) Overlap tests vs targets
        for ( auto& s : m_vols ) {
            auto& hv = *s.hv;
            if ( !hv.Alive ( ) ) continue;

            engine::IntRect volBox{}; engine::Vec2 A{} , B{}; float R{};
            HitVolumeGeom::BuildShape ( hv , volBox , A , B , R );

            const auto& cfg = hv.GetCfg ( );
            for ( const auto& t : targets ) {
                if ( !t.alive ) continue;
                if ( cfg.excludeOwner && t.id == hv.Owner ( ) ) continue;

                bool hit = false;
                switch ( cfg.shape ) {
                case HitShape::Box:
                    hit = engine::physics::Overlap ( t.aabb , volBox );
                    break;

                case HitShape::Circle: {
                    hit = engine::physics::OverlapRectCircle ( t.aabb , hv.Anchor ( ) , cfg.r );
                    break;
                }

                case HitShape::Capsule: {
                    // current capsule
                    hit = engine::physics::OverlapRectCapsule ( t.aabb , A , B , R );
                    // previous capsule (sweep gap)
                    if ( !hit && s.hasPrev ) hit = engine::physics::OverlapRectCapsule ( t.aabb , s.prevA , s.prevB , s.prevR );
                    // bridge previous end → current end (handles fast turning)
                    if ( !hit && s.hasPrev ) hit = engine::physics::OverlapRectCapsule ( t.aabb , s.prevB , B , R );
                    break;
                }
                }

                if ( hit && hv.CanHitTarget ( t.id ) ) {
                    hv.MarkHitTarget ( t.id );

                    if ( cfg.payload.effect == HitEffect::Capture ) {
                        if ( t.inhalable ) {
                            HitEvent ev{};
                            ev.targetId = t.id;
                            ev.volumeId = s.id;
                            ev.ownerId = hv.Owner ( );
                            ev.payload = cfg.payload;
                            ev.gift = ( cfg.payload.gift != Ability::None ) ? cfg.payload.gift : t.abilityGift;
                            m_hits.push_back ( ev );
                        }
                    }
                    else {
                        HitEvent ev{ t.id, s.id, hv.Owner ( ), cfg.payload };
                        ev.gift = Ability::None;
                        m_hits.push_back ( ev );
                    }
                }
            }

            s.prevA = A; s.prevB = B; s.prevR = R; s.hasPrev = true;
        }

        // 3) Cleanup / despawn events
        for ( auto& s : m_vols ) {
            if ( s.hv && !s.hv->Alive ( ) ) {
                m_despawns.push_back ( DespawnEvent{ s.id, true } );
            }
        }
        m_vols.erase ( std::remove_if ( m_vols.begin ( ) , m_vols.end ( ) ,
            [ ] ( const Slot& s ) { return !s.hv || !s.hv->Alive ( ); } ) , m_vols.end ( ) );
    }

    void HitVolumeSystem::DebugDraw ( engine::IDebugDraw& dbg , int ox , int oy ) const {
        for ( const auto& s : m_vols ) if ( s.hv ) HitVolumeDebugDraw::Draw ( *s.hv , &dbg , ox , oy );
    }
} // namespace game
