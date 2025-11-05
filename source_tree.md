```
/engine
  /core
    GameApp.h
    GameApp.cpp
    RenderSystem.h
    RenderSystem.cpp
    Scene.h/.cpp — Lightweight object container: spawn/update/render/clear.
    Time.h/.cpp — Frame timing (variable dt + fixed-step) & FPS counter (QPC-backed)
    Input.h/.cpp — Win32 poll-based input (keyboard/mouse), edge flags, simple action/axis mapping.
    Object.h — Base object: Id + virtual Update/Render hooks.
  /render
    IRenderer.h
    D3D11Renderer.h
    D3D11Sprite.h
    D3D11SpriteBatch.h
    D3D11SpriteBatch.cpp
    Texture.h
    TextureLoader.h
    D3D11TextureLoader.cpp
    DWriteText.h
    D3D11DebugDraw.h
  /world
    Camera.h
    TileSet.h
    TileSet.cpp
    TileMap.h
    TileMap.cpp
    WorldSystem.h
    WorldSystem.cpp
  /physics
    Collision.h
    Collision.cpp
    PhysicsBody.h
    PhysicsBody.cpp
  /util
    Anim.h
    Math.h
    StringConv.h
    DebugDraw.h

/game
  /session
    PlaySession.h
    PlaySession.Core.cpp
    PlaySession.Render.cpp
    PlaySession.Stage.cpp
    PlaySession.Combat.cpp
    PlaySession.Door.cpp

  /data
    StageDesc.h
    StageDesc.cpp
    StageCSV.h
    StageCSV.cpp
    AnimCSV.h
    AnimCSV.cpp
    GameConfig.h

  /entities
    /player
      Player.h
      PlayerFSM.h
      PlayerFSM.cpp
    /monsters
      Monster.h
      MonsterTypes.h
      MonsterFactory.h
      WaddleDee.h
      WaddleDee.cpp
      WaddleDoo.h
      WaddleDoo.cpp
      HotHead.h
      HotHead.cpp
      Sparky.h
      Sparky.cpp
      Apple.h
      WhispyWoods.h

  /combat
    CombatTypes.h
    CombatTarget.h
    Damage.h
    Ability.h
    HitVolume.h
    HitVolume.cpp
    HitVolumeFactory.h
    HitVolumeFactory.cpp
    HitVolumeSystem.h
    HitVolumeSystem.cpp

  /projectile
    Projectile.h
    ProjectileFactory.h
    ProjectileFactory.cpp
    ProjectileSystem.h
    ProjectileSystem.cpp
```