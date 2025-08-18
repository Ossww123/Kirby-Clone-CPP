#include "pch.h"
#include "CProjectile.h"
#include "CTimeMgr.h"
#include "CCollider.h"
#include "CCore.h"
#include "CEventMgr.h"

CProjectile::CProjectile()
    : m_eProjectileType(PROJECTILE_TYPE::KIRBY_AIR_PUFF)
    , m_vDirection(Vec2(1.f, 0.f))
    , m_fSpeed(300.f)
    , m_fDamage(1.f)
    , m_fAccTime(0.f)
    , m_fMaxLifeTime(3.f)
    , m_eOwnerType(GROUP_TYPE::DEFAULT)
{
    SetType(OBJECT_TYPE::PLAYER);
    InitializeByType();
}

CProjectile::CProjectile(PROJECTILE_TYPE _eType)
    : m_eProjectileType(_eType)
    , m_vDirection(Vec2(1.f, 0.f))
    , m_fSpeed(300.f)
    , m_fDamage(1.f)
    , m_fAccTime(0.f)
    , m_fMaxLifeTime(3.f)
    , m_eOwnerType(GROUP_TYPE::DEFAULT)
{
    SetType(OBJECT_TYPE::PLAYER);
    InitializeByType();
}

CProjectile::~CProjectile()
{
}

void CProjectile::Update()
{
    UpdateMovement();
    UpdateLifeTime();
    CheckBounds();
}

void CProjectile::Render(HDC _dc)
{
    CObject::Render(_dc);
}

void CProjectile::OnCollisionEnter(CCollider* _pOther)
{
    CObject* pOtherObj = _pOther->GetOwner();
    
    // 다른 오브젝트의 그룹 타입은 씬 시스템에서 관리되므로
    // 오브젝트 타입으로 판단
    OBJECT_TYPE eOtherType = pOtherObj->GetType();

    // 같은 소유자와는 충돌하지 않음 (플레이어 투사체는 플레이어와 충돌 안함)
    if ((m_eOwnerType == GROUP_TYPE::PLAYER && eOtherType == OBJECT_TYPE::PLAYER) ||
        (m_eOwnerType == GROUP_TYPE::MONSTER && 
         (eOtherType >= OBJECT_TYPE::MONSTER_WADDLE_DEE && eOtherType <= OBJECT_TYPE::MONSTER_WHISPY_WOODS)))
        return;

    // 타일과 충돌 시 투사체 소멸
    if (eOtherType >= OBJECT_TYPE::TILE_GROUND && eOtherType <= OBJECT_TYPE::TILE_INVISIBLE)
    {
        // 투사체 히트 이벤트 발생
        tEvent event = {};
        event.eType = EVENT_TYPE::PROJECTILE_HIT;
        event.wParam = (DWORD_PTR)this;
        event.lParam = (DWORD_PTR)pOtherObj;
        CEventMgr::GetInst()->AddEvent(event);

        // 투사체 삭제
        SetDead();
        return;
    }

    // 몬스터와 충돌 시 (플레이어 투사체인 경우)
    if (m_eOwnerType == GROUP_TYPE::PLAYER && 
        (eOtherType >= OBJECT_TYPE::MONSTER_WADDLE_DEE && eOtherType <= OBJECT_TYPE::MONSTER_WHISPY_WOODS))
    {
        // 몬스터에게 데미지 이벤트 발생
        tEvent event = {};
        event.eType = EVENT_TYPE::MONSTER_DAMAGE;
        event.wParam = (DWORD_PTR)pOtherObj;
        event.lParam = (DWORD_PTR)&m_fDamage;
        CEventMgr::GetInst()->AddEvent(event);

        // 투사체 히트 이벤트 발생
        tEvent hitEvent = {};
        hitEvent.eType = EVENT_TYPE::PROJECTILE_HIT;
        hitEvent.wParam = (DWORD_PTR)this;
        hitEvent.lParam = (DWORD_PTR)pOtherObj;
        CEventMgr::GetInst()->AddEvent(hitEvent);

        // 투사체 삭제
        SetDead();
        return;
    }

    // 플레이어와 충돌 시 (몬스터 투사체인 경우)
    if (m_eOwnerType == GROUP_TYPE::MONSTER && eOtherType == OBJECT_TYPE::PLAYER)
    {
        // 플레이어에게 데미지 이벤트 발생
        tEvent event = {};
        event.eType = EVENT_TYPE::PLAYER_DAMAGE;
        event.wParam = (DWORD_PTR)pOtherObj;
        event.lParam = (DWORD_PTR)&m_fDamage;
        CEventMgr::GetInst()->AddEvent(event);

        // 투사체 히트 이벤트 발생
        tEvent hitEvent = {};
        hitEvent.eType = EVENT_TYPE::PROJECTILE_HIT;
        hitEvent.wParam = (DWORD_PTR)this;
        hitEvent.lParam = (DWORD_PTR)pOtherObj;
        CEventMgr::GetInst()->AddEvent(hitEvent);

        // 투사체 삭제
        SetDead();
        return;
    }
}

void CProjectile::UpdateMovement()
{
    float fDT = CTimeMgr::GetInst()->GetDT();
    
    // 현재 위치에 방향 * 속도 * 델타타임을 더해서 이동
    Vec2 vPos = GetPos();
    vPos += m_vDirection * m_fSpeed * fDT;
    SetPos(vPos);
}

void CProjectile::UpdateLifeTime()
{
    float fDT = CTimeMgr::GetInst()->GetDT();
    m_fAccTime += fDT;

    // 생존 시간 초과 시 삭제
    if (m_fAccTime >= m_fMaxLifeTime)
    {
        SetDead();
    }
}

void CProjectile::CheckBounds()
{
    // 화면 밖으로 나가면 삭제 (옵션)
    Vec2 vResolution = CCore::GetInst()->GetResolution();
    Vec2 vPos = GetPos();
    Vec2 vScale = GetScale();

    // 화면 경계를 벗어나면 삭제
    if (vPos.x + vScale.x * 0.5f < 0 ||
        vPos.x - vScale.x * 0.5f > vResolution.x ||
        vPos.y + vScale.y * 0.5f < 0 ||
        vPos.y - vScale.y * 0.5f > vResolution.y)
    {
        SetDead();
    }
}

void CProjectile::InitializeByType()
{
    switch (m_eProjectileType)
    {
    // === 커비 투사체들 ===
    case PROJECTILE_TYPE::KIRBY_AIR_PUFF:
        SetScale(Vec2(32.f, 32.f));
        m_fSpeed = 400.f;
        m_fDamage = 1.f;
        m_fMaxLifeTime = 2.f;
        CreateCollider();
        GetCollider()->SetScale(Vec2(24.f, 24.f));
        break;

    case PROJECTILE_TYPE::KIRBY_STAR:
        SetScale(Vec2(24.f, 24.f));
        m_fSpeed = 500.f;
        m_fDamage = 2.f;
        m_fMaxLifeTime = 3.f;
        CreateCollider();
        GetCollider()->SetScale(Vec2(20.f, 20.f));
        break;

    case PROJECTILE_TYPE::KIRBY_STAR_ENHANCED:
        SetScale(Vec2(32.f, 32.f));
        m_fSpeed = 550.f;
        m_fDamage = 4.f;  // 강화된 데미지
        m_fMaxLifeTime = 4.f;
        CreateCollider();
        GetCollider()->SetScale(Vec2(28.f, 28.f));
        break;

    case PROJECTILE_TYPE::KIRBY_FIRE:
        SetScale(Vec2(28.f, 28.f));
        m_fSpeed = 350.f;
        m_fDamage = 3.f;
        m_fMaxLifeTime = 2.5f;
        CreateCollider();
        GetCollider()->SetScale(Vec2(24.f, 24.f));
        break;

    case PROJECTILE_TYPE::KIRBY_BEAM:
        SetScale(Vec2(20.f, 8.f));  // 빔 형태 (가로로 긴)
        m_fSpeed = 700.f;  // 빠른 속도
        m_fDamage = 2.f;
        m_fMaxLifeTime = 1.5f;
        CreateCollider();
        GetCollider()->SetScale(Vec2(18.f, 6.f));
        break;

    case PROJECTILE_TYPE::KIRBY_ELECTRIC_FIELD:
        SetScale(Vec2(40.f, 40.f));  // 큰 전기장
        m_fSpeed = 200.f;  // 느린 이동
        m_fDamage = 2.5f;
        m_fMaxLifeTime = 3.f;
        CreateCollider();
        GetCollider()->SetScale(Vec2(36.f, 36.f));
        break;

    // === 몬스터 투사체들 ===
    case PROJECTILE_TYPE::BOSS_AIR_PUFF:
        SetScale(Vec2(48.f, 48.f));  // 보스 투사체는 큼
        m_fSpeed = 300.f;
        m_fDamage = 2.f;
        m_fMaxLifeTime = 4.f;
        CreateCollider();
        GetCollider()->SetScale(Vec2(40.f, 40.f));
        break;

    default:
        break;
    }
}