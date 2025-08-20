#include "pch.h"
#include "CProjectileFactory.h"
#include "CProjectile.h"
#include "CEventMgr.h"

CProjectile* CProjectileFactory::Create(PROJECTILE_TYPE _eType, Vec2 _vPos, Vec2 _vDirection, GROUP_TYPE _eOwner)
{
    CProjectile* pProjectile = new CProjectile(_eType);
    
    if (pProjectile)
    {
        // 기본 속성 설정
        pProjectile->SetPos(_vPos);
        pProjectile->SetDirection(_vDirection);
        pProjectile->SetOwnerType(_eOwner);
        
        // 타입별 추가 설정
        SetupProjectileByType(pProjectile, _eType);
        
        // 소유자별 설정 적용
        ApplyOwnerSettings(pProjectile, _eOwner);
    }
    
    return pProjectile;
}

// === 커비 투사체 생성 함수들 ===
CProjectile* CProjectileFactory::CreateAirPuff(Vec2 _vPos, Vec2 _vDirection, GROUP_TYPE _eOwner)
{
    return Create(PROJECTILE_TYPE::KIRBY_AIR_PUFF, _vPos, _vDirection, _eOwner);
}

CProjectile* CProjectileFactory::CreateSlideKick(Vec2 _vPos, Vec2 _vDirection, GROUP_TYPE _eOwner)
{
    return Create(PROJECTILE_TYPE::KIRBY_SLIDE_KICK, _vPos, _vDirection, _eOwner);
}

CProjectile* CProjectileFactory::CreateStar(Vec2 _vPos, Vec2 _vDirection, GROUP_TYPE _eOwner)
{
    return Create(PROJECTILE_TYPE::KIRBY_STAR, _vPos, _vDirection, _eOwner);
}

CProjectile* CProjectileFactory::CreateFire(Vec2 _vPos, Vec2 _vDirection, GROUP_TYPE _eOwner)
{
    return Create(PROJECTILE_TYPE::KIRBY_FIRE, _vPos, _vDirection, _eOwner);
}

CProjectile* CProjectileFactory::CreateIce(Vec2 _vPos, Vec2 _vDirection, GROUP_TYPE _eOwner)
{
    // ICE는 현재 미구현 상태이므로 기본 AIR_PUFF로 대체
    return Create(PROJECTILE_TYPE::KIRBY_AIR_PUFF, _vPos, _vDirection, _eOwner);
}

CProjectile* CProjectileFactory::CreateElectric(Vec2 _vPos, Vec2 _vDirection, GROUP_TYPE _eOwner)
{
    return Create(PROJECTILE_TYPE::KIRBY_ELECTRIC_FIELD, _vPos, _vDirection, _eOwner);
}

// === 새로운 커비 투사체 생성 함수들 ===
CProjectile* CProjectileFactory::CreateStarEnhanced(Vec2 _vPos, Vec2 _vDirection, GROUP_TYPE _eOwner)
{
    return Create(PROJECTILE_TYPE::KIRBY_STAR_ENHANCED, _vPos, _vDirection, _eOwner);
}

CProjectile* CProjectileFactory::CreateBeam(Vec2 _vPos, Vec2 _vDirection, GROUP_TYPE _eOwner)
{
    return Create(PROJECTILE_TYPE::KIRBY_BEAM, _vPos, _vDirection, _eOwner);
}

CProjectile* CProjectileFactory::CreateElectricField(Vec2 _vPos, Vec2 _vDirection, GROUP_TYPE _eOwner)
{
    return Create(PROJECTILE_TYPE::KIRBY_ELECTRIC_FIELD, _vPos, _vDirection, _eOwner);
}

// === 몬스터 투사체 생성 함수들 ===
CProjectile* CProjectileFactory::CreateBossAirPuff(Vec2 _vPos, Vec2 _vDirection, GROUP_TYPE _eOwner)
{
    return Create(PROJECTILE_TYPE::BOSS_AIR_PUFF, _vPos, _vDirection, _eOwner);
}

CProjectile* CProjectileFactory::CreateMonsterFireball(Vec2 _vPos, Vec2 _vDirection, GROUP_TYPE _eOwner)
{
    return Create(PROJECTILE_TYPE::MONSTER_FIREBALL, _vPos, _vDirection, _eOwner);
}

CProjectile* CProjectileFactory::CreateMonsterElectric(Vec2 _vPos, Vec2 _vDirection, GROUP_TYPE _eOwner)
{
    return Create(PROJECTILE_TYPE::MONSTER_ELECTRIC, _vPos, _vDirection, _eOwner);
}

CProjectile* CProjectileFactory::CreateMonsterBeam(Vec2 _vPos, Vec2 _vDirection, GROUP_TYPE _eOwner)
{
    return Create(PROJECTILE_TYPE::MONSTER_BEAM, _vPos, _vDirection, _eOwner);
}

// === 호환성 유지 함수들 ===
CProjectile* CProjectileFactory::CreateCopyEssence(Vec2 _vPos, Vec2 _vDirection, GROUP_TYPE _eOwner)
{
    // 삼킨 적 1마리 뱉기는 KIRBY_STAR로 처리
    return Create(PROJECTILE_TYPE::KIRBY_STAR, _vPos, _vDirection, _eOwner);
}

const wchar_t* CProjectileFactory::GetProjectileTypeName(PROJECTILE_TYPE _eType)
{
    switch (_eType)
    {
    // === 커비 투사체들 ===
    case PROJECTILE_TYPE::KIRBY_AIR_PUFF:       return L"Kirby Air Puff";
    case PROJECTILE_TYPE::KIRBY_SLIDE_KICK:     return L"Kirby Slide Kick";
    case PROJECTILE_TYPE::KIRBY_STAR:           return L"Kirby Star";
    case PROJECTILE_TYPE::KIRBY_STAR_ENHANCED:  return L"Kirby Enhanced Star";
    case PROJECTILE_TYPE::KIRBY_FIRE:           return L"Kirby Fire";
    case PROJECTILE_TYPE::KIRBY_BEAM:           return L"Kirby Beam";
    case PROJECTILE_TYPE::KIRBY_ELECTRIC_FIELD: return L"Kirby Electric Field";
    
    // === 몬스터 투사체들 ===
    case PROJECTILE_TYPE::BOSS_AIR_PUFF:        return L"Boss Air Puff";
    case PROJECTILE_TYPE::MONSTER_FIREBALL:     return L"Monster Fireball";
    case PROJECTILE_TYPE::MONSTER_ELECTRIC:     return L"Monster Electric";
    case PROJECTILE_TYPE::MONSTER_BEAM:         return L"Monster Beam";
    
    default:                                    return L"Unknown";
    }
}

float CProjectileFactory::GetDefaultSpeed(PROJECTILE_TYPE _eType)
{
    switch (_eType)
    {
    // === 커비 투사체들 ===
    case PROJECTILE_TYPE::KIRBY_AIR_PUFF:       return 750.f;
    case PROJECTILE_TYPE::KIRBY_SLIDE_KICK:     return 320.f;  // 슬라이드 속도와 동일
    case PROJECTILE_TYPE::KIRBY_STAR:           return 500.f;
    case PROJECTILE_TYPE::KIRBY_STAR_ENHANCED:  return 550.f;
    case PROJECTILE_TYPE::KIRBY_FIRE:           return 350.f;
    case PROJECTILE_TYPE::KIRBY_BEAM:           return 700.f;
    case PROJECTILE_TYPE::KIRBY_ELECTRIC_FIELD: return 200.f;
    
    // === 몬스터 투사체들 ===
    case PROJECTILE_TYPE::BOSS_AIR_PUFF:        return 300.f;
    case PROJECTILE_TYPE::MONSTER_FIREBALL:     return 250.f;
    case PROJECTILE_TYPE::MONSTER_ELECTRIC:     return 400.f;
    case PROJECTILE_TYPE::MONSTER_BEAM:         return 600.f;
    
    default:                                    return 400.f;
    }
}

float CProjectileFactory::GetDefaultDamage(PROJECTILE_TYPE _eType)
{
    switch (_eType)
    {
    // === 커비 투사체들 ===
    case PROJECTILE_TYPE::KIRBY_AIR_PUFF:       return 1.f;
    case PROJECTILE_TYPE::KIRBY_SLIDE_KICK:     return 1.f;  // 기본 데미지
    case PROJECTILE_TYPE::KIRBY_STAR:           return 2.f;
    case PROJECTILE_TYPE::KIRBY_STAR_ENHANCED:  return 4.f;
    case PROJECTILE_TYPE::KIRBY_FIRE:           return 3.f;
    case PROJECTILE_TYPE::KIRBY_BEAM:           return 2.f;
    case PROJECTILE_TYPE::KIRBY_ELECTRIC_FIELD: return 2.5f;
    
    // === 몬스터 투사체들 ===
    case PROJECTILE_TYPE::BOSS_AIR_PUFF:        return 2.f;
    case PROJECTILE_TYPE::MONSTER_FIREBALL:     return 2.f;
    case PROJECTILE_TYPE::MONSTER_ELECTRIC:     return 1.5f;
    case PROJECTILE_TYPE::MONSTER_BEAM:         return 1.f;
    
    default:                                    return 1.f;
    }
}

float CProjectileFactory::GetDefaultLifeTime(PROJECTILE_TYPE _eType)
{
    switch (_eType)
    {
    // === 커비 투사체들 ===
    case PROJECTILE_TYPE::KIRBY_AIR_PUFF:       return 0.5f;
    case PROJECTILE_TYPE::KIRBY_SLIDE_KICK:     return 0.8f;  // 슬라이드 지속시간과 동일
    case PROJECTILE_TYPE::KIRBY_STAR:           return 3.f;
    case PROJECTILE_TYPE::KIRBY_STAR_ENHANCED:  return 4.f;
    case PROJECTILE_TYPE::KIRBY_FIRE:           return 2.5f;
    case PROJECTILE_TYPE::KIRBY_BEAM:           return 1.5f;
    case PROJECTILE_TYPE::KIRBY_ELECTRIC_FIELD: return 3.f;
    
    // === 몬스터 투사체들 ===
    case PROJECTILE_TYPE::BOSS_AIR_PUFF:        return 4.f;
    case PROJECTILE_TYPE::MONSTER_FIREBALL:     return 3.f;
    case PROJECTILE_TYPE::MONSTER_ELECTRIC:     return 2.f;
    case PROJECTILE_TYPE::MONSTER_BEAM:         return 1.5f;
    
    default:                                    return 3.f;
    }
}

Vec2 CProjectileFactory::GetDefaultScale(PROJECTILE_TYPE _eType)
{
    switch (_eType)
    {
    // === 커비 투사체들 ===
    case PROJECTILE_TYPE::KIRBY_AIR_PUFF:       return Vec2(64.f, 32.f);
    case PROJECTILE_TYPE::KIRBY_SLIDE_KICK:     return Vec2(32.f, 32.f);  // 커비 발끝 크기
    case PROJECTILE_TYPE::KIRBY_STAR:           return Vec2(24.f, 24.f);
    case PROJECTILE_TYPE::KIRBY_STAR_ENHANCED:  return Vec2(32.f, 32.f);
    case PROJECTILE_TYPE::KIRBY_FIRE:           return Vec2(28.f, 28.f);
    case PROJECTILE_TYPE::KIRBY_BEAM:           return Vec2(20.f, 8.f);
    case PROJECTILE_TYPE::KIRBY_ELECTRIC_FIELD: return Vec2(40.f, 40.f);
    
    // === 몬스터 투사체들 ===
    case PROJECTILE_TYPE::BOSS_AIR_PUFF:        return Vec2(48.f, 48.f);
    case PROJECTILE_TYPE::MONSTER_FIREBALL:     return Vec2(24.f, 24.f);
    case PROJECTILE_TYPE::MONSTER_ELECTRIC:     return Vec2(20.f, 20.f);
    case PROJECTILE_TYPE::MONSTER_BEAM:         return Vec2(16.f, 6.f);
    
    default:                                    return Vec2(32.f, 32.f);
    }
}

GROUP_TYPE CProjectileFactory::GetProjectileGroup(GROUP_TYPE _eOwner)
{
    switch (_eOwner)
    {
    case GROUP_TYPE::PLAYER:  return GROUP_TYPE::PROJ_PLAYER;
    case GROUP_TYPE::MONSTER: return GROUP_TYPE::PROJ_MONSTER;
    default:                  return GROUP_TYPE::PROJ_PLAYER;
    }
}

void CProjectileFactory::AddToScene(CProjectile* _pProjectile, GROUP_TYPE _eOwner)
{
    if (!_pProjectile) return;
    
    GROUP_TYPE projGroup = GetProjectileGroup(_eOwner);
    CREATE_OBJECT(_pProjectile, projGroup);
}

void CProjectileFactory::SetupProjectileByType(CProjectile* _pProjectile, PROJECTILE_TYPE _eType)
{
    if (!_pProjectile) return;
    
    // 기본 속성 적용 (CProjectile 생성자에서 이미 처리되지만 명시적으로)
    _pProjectile->SetSpeed(GetDefaultSpeed(_eType));
    _pProjectile->SetDamage(GetDefaultDamage(_eType));
    _pProjectile->SetLifeTime(GetDefaultLifeTime(_eType));
    
    // 스케일 설정은 CProjectile 생성자에서 처리됨
    
    // 향후 타입별 특수 설정 추가 가능
    switch (_eType)
    {
    // === 커비 투사체 특수 설정 ===
    case PROJECTILE_TYPE::KIRBY_FIRE:
        // 화염 특수 효과 설정 (향후 확장)
        break;
    case PROJECTILE_TYPE::KIRBY_ELECTRIC_FIELD:
        // 전기장 특수 효과 설정 (향후 확장)
        break;
    case PROJECTILE_TYPE::KIRBY_BEAM:
        // 빔 특수 효과 설정 (향후 확장)
        break;
    case PROJECTILE_TYPE::KIRBY_STAR_ENHANCED:
        // 강화 별 특수 효과 설정 (향후 확장)
        break;

    // === 몬스터 투사체 특수 설정 ===
    case PROJECTILE_TYPE::MONSTER_FIREBALL:
        // 몬스터 화염구 특수 효과 설정 (향후 확장)
        break;
    case PROJECTILE_TYPE::MONSTER_ELECTRIC:
        // 몬스터 전기구슬 특수 효과 설정 (향후 확장)
        break;
    case PROJECTILE_TYPE::MONSTER_BEAM:
        // 몬스터 빔 특수 효과 설정 (향후 확장)
        break;
    case PROJECTILE_TYPE::BOSS_AIR_PUFF:
        // 보스 공기포 특수 효과 설정 (향후 확장)
        break;

    // === 기본 투사체 ===
    case PROJECTILE_TYPE::KIRBY_AIR_PUFF:
    case PROJECTILE_TYPE::KIRBY_STAR:
    default:
        // 기본 설정 (특별한 처리 없음)
        break;
    }
}

void CProjectileFactory::ApplyOwnerSettings(CProjectile* _pProjectile, GROUP_TYPE _eOwner)
{
    if (!_pProjectile) return;
    
    // 소유자별 추가 설정
    switch (_eOwner)
    {
    case GROUP_TYPE::PLAYER:
        // 플레이어 투사체 특수 설정
        break;
    case GROUP_TYPE::MONSTER:
        // 몬스터 투사체 특수 설정 (색상 변경, 속도 조정 등)
        break;
    default:
        break;
    }
}