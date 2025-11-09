#pragma once
//
// Responsibility: Create monsters by type; register per-type makers.
// Non-Goals:      Rendering policy, Windows types, gameplay tuning logic.
// Call-Context:   Main thread; used by PlaySession during stage load/spawn.
//

#include <memory>
#include <unordered_map>
#include <functional>

#include "engine/util/Types.h"                 // engine::IntRect
#include "engine/physics/Collision.h"          // fwd acceptable via header include

namespace engine { namespace physics { class CollisionSystem; } }

namespace game {
    enum class MonsterType : int;
    struct SpawnSpec;   // only referenced by const&; definition in .cpp

    class Monster;

    class MonsterFactory {
    public:
        using Maker = std::function<std::unique_ptr<Monster> (
            const engine::IntRect& ,
            const engine::physics::CollisionSystem* ,
            const SpawnSpec& )>;

        static void Register ( MonsterType t , Maker m );
        static std::unique_ptr<Monster> Create ( MonsterType t ,
                                               const engine::IntRect& worldBounds ,
                                               const engine::physics::CollisionSystem* col ,
                                               const SpawnSpec& spec );

        // Registers all built-in monsters (hothead, sparky, etc.)
        static void RegisterDefaults ( );

    private:
        static std::unordered_map<MonsterType , Maker>& Makers ( );
    };
} // namespace game
