#pragma once

class CObject;
class CProjectile;

class CProjectileFactory
{
private:
    // 팩토리는 정적 클래스로 사용
    CProjectileFactory() = delete;
    ~CProjectileFactory() = delete;

public:
    // === 메인 팩토리 함수 ===
    static CProjectile* Create(PROJECTILE_TYPE _eType, Vec2 _vPos, Vec2 _vDirection, GROUP_TYPE _eOwner = GROUP_TYPE::PLAYER);

    // === 커비 투사체 전용 생성 함수들 ===
    static CProjectile* CreateAirPuff(Vec2 _vPos, Vec2 _vDirection, GROUP_TYPE _eOwner = GROUP_TYPE::PLAYER);
    static CProjectile* CreateSlideKick(Vec2 _vPos, Vec2 _vDirection, GROUP_TYPE _eOwner = GROUP_TYPE::PLAYER);
    static CProjectile* CreateStar(Vec2 _vPos, Vec2 _vDirection, GROUP_TYPE _eOwner = GROUP_TYPE::PLAYER);
    static CProjectile* CreateStarEnhanced(Vec2 _vPos, Vec2 _vDirection, GROUP_TYPE _eOwner = GROUP_TYPE::PLAYER);
    static CProjectile* CreateFire(Vec2 _vPos, Vec2 _vDirection, GROUP_TYPE _eOwner = GROUP_TYPE::PLAYER);
    static CProjectile* CreateBeam(Vec2 _vPos, Vec2 _vDirection, GROUP_TYPE _eOwner = GROUP_TYPE::PLAYER);
    static CProjectile* CreateElectricField(Vec2 _vPos, Vec2 _vDirection, GROUP_TYPE _eOwner = GROUP_TYPE::PLAYER);
    
    // === 특수 투사체 생성 함수들 ===
    static CProjectile* CreateRotatingBeam(Vec2 _vCenter, float _fRadius, float _fStartAngle, float _fEndAngle, float _fDuration, CObject* _pOwner);

    // === 몬스터 투사체 전용 생성 함수들 ===
    static CProjectile* CreateBossAirPuff(Vec2 _vPos, Vec2 _vDirection, GROUP_TYPE _eOwner = GROUP_TYPE::MONSTER);
    static CProjectile* CreateMonsterFireball(Vec2 _vPos, Vec2 _vDirection, GROUP_TYPE _eOwner = GROUP_TYPE::MONSTER);
    static CProjectile* CreateMonsterElectric(Vec2 _vPos, Vec2 _vDirection, GROUP_TYPE _eOwner = GROUP_TYPE::MONSTER);
    static CProjectile* CreateMonsterBeam(Vec2 _vPos, Vec2 _vDirection, GROUP_TYPE _eOwner = GROUP_TYPE::MONSTER);

    // === 호환성 유지 함수들 (deprecated) ===
    static CProjectile* CreateIce(Vec2 _vPos, Vec2 _vDirection, GROUP_TYPE _eOwner = GROUP_TYPE::PLAYER);
    static CProjectile* CreateElectric(Vec2 _vPos, Vec2 _vDirection, GROUP_TYPE _eOwner = GROUP_TYPE::PLAYER);
    static CProjectile* CreateCopyEssence(Vec2 _vPos, Vec2 _vDirection, GROUP_TYPE _eOwner = GROUP_TYPE::PLAYER);

public:
    // === 투사체 정보 유틸리티 함수들 ===
    static const wchar_t* GetProjectileTypeName(PROJECTILE_TYPE _eType);
    static float GetDefaultSpeed(PROJECTILE_TYPE _eType);
    static float GetDefaultDamage(PROJECTILE_TYPE _eType);
    static float GetDefaultLifeTime(PROJECTILE_TYPE _eType);
    static Vec2 GetDefaultScale(PROJECTILE_TYPE _eType);

public:
    // === 씬 연동 헬퍼 함수들 ===
    static GROUP_TYPE GetProjectileGroup(GROUP_TYPE _eOwner);
    static void AddToScene ( CProjectile* _pProjectile , GROUP_TYPE _eOwner );

private:
    // === 내부 설정 함수들 ===
    static void SetupProjectileByType(CProjectile* _pProjectile, PROJECTILE_TYPE _eType);
    static void ApplyOwnerSettings(CProjectile* _pProjectile, GROUP_TYPE _eOwner);
};