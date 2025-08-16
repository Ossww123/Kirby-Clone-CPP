#include "pch.h"
#include "CPlayerCollisionSystem.h"
#include "CPlayerInhaleSystem.h"
#include "CPlayerHealthSystem.h"
#include "CPlayer.h"
#include "CCollider.h"
#include "CObject.h"
#include "CTile.h"
#include "CMonster.h"
#include "CRigidBody.h"

CPlayerCollisionSystem::CPlayerCollisionSystem(CPlayer* _pOwner)
    : m_pOwner(_pOwner)
    , m_bNeedGroundCheck(false)
    , m_pExitingTile(nullptr)
{
}

CPlayerCollisionSystem::~CPlayerCollisionSystem()
{
}

void CPlayerCollisionSystem::Init()
{
    // 필요시 초기화 로직 추가
}

void CPlayerCollisionSystem::UpdateGroundState()
{
    if (!m_bNeedGroundCheck) return;

    CRigidBody* pRigidBody = m_pOwner ? m_pOwner->GetRigidBody() : nullptr;
    if (!pRigidBody) return;

    // Exit 중인 타일을 제외하고 다른 solid 타일과 충돌 중인지 확인
    if (!IsCollidingWithOtherSolidTiles(m_pExitingTile))
    {
        pRigidBody->SetGround(false);
    }

    // 플래그 및 Exit 타일 초기화
    m_bNeedGroundCheck = false;
    m_pExitingTile = nullptr;
}

// === 충돌 처리 메인 인터페이스 ===

void CPlayerCollisionSystem::HandleCollisionEnter(CCollider* _pOther)
{
    if (!_pOther || !m_pOwner) return;

    CObject* pOtherObj = _pOther->GetOwner();
    if (!pOtherObj) return;

    if (IsInvincibleState()) return;

    OBJECT_TYPE eType = pOtherObj->GetType();

    if (IsTileType(eType))
    {
        CTile* pTile = dynamic_cast<CTile*>(pOtherObj);
        if (pTile) HandleTileCollisionEnter(pTile);
    }
    else if (IsMonsterType(eType))
    {
        CMonster* pMonster = dynamic_cast<CMonster*>(pOtherObj);
        if (pMonster) HandleMonsterCollisionEnter(pMonster);
    }
    else if (IsItemType(eType))
    {
        HandleItemCollisionEnter(pOtherObj);
    }
    else if (IsSpecialObjectType(eType))
    {
        HandleSpecialObjectCollisionEnter(pOtherObj);
    }
}

void CPlayerCollisionSystem::HandleCollision(CCollider* _pOther)
{
    if (!_pOther || !m_pOwner) return;

    CObject* pOtherObj = _pOther->GetOwner();
    if (!pOtherObj) return;

    OBJECT_TYPE eType = pOtherObj->GetType();

    if (IsTileType(eType))
    {
        CTile* pTile = dynamic_cast<CTile*>(pOtherObj);
        if (pTile) HandleTileCollision(pTile);
    }
}

void CPlayerCollisionSystem::HandleCollisionExit(CCollider* _pOther)
{
    if (!_pOther || !m_pOwner) return;

    CObject* pOtherObj = _pOther->GetOwner();
    if (!pOtherObj) return;

    OBJECT_TYPE eType = pOtherObj->GetType();

    if (IsTileType(eType))
    {
        CTile* pTile = dynamic_cast<CTile*>(pOtherObj);
        if (pTile) HandleTileCollisionExit(pTile);
    }
}

// === 타입별 충돌 처리 함수들 ===

void CPlayerCollisionSystem::HandleTileCollisionEnter(CTile* _pTile)
{
    if (!_pTile || !_pTile->IsSolid()) return;
    // 타일 진입 시 특별한 처리가 필요하면 여기에 추가
}

void CPlayerCollisionSystem::HandleTileCollision(CTile* _pTile)
{
    if (!_pTile || !_pTile->IsSolid()) return;

    if (ShouldSetGroundState(_pTile))
    {
        CorrectPlayerPosition(_pTile);
        SetGroundState(_pTile);
        ResetVerticalVelocity();
    }
}

void CPlayerCollisionSystem::HandleTileCollisionExit(CTile* _pTile)
{
    if (!_pTile || !_pTile->IsSolid()) return;

    m_pExitingTile = _pTile;
    m_bNeedGroundCheck = true;
}

void CPlayerCollisionSystem::HandleMonsterCollisionEnter(CMonster* _pMonster)
{
    if (!_pMonster) return;

    // 흡입 중인지 확인
    if (m_pOwner->IsInhaling())
    {
        Vec2 vDiff = m_pOwner->GetPos() - _pMonster->GetPos();
        if (vDiff.Length() < 60.f) // 흡입 범위 내
        {
            if (m_pOwner->GetInhaleSystem())
            {
                m_pOwner->GetInhaleSystem()->SwallowTarget(_pMonster);
            }
            return;
        }
    }

    // 입에 물고 있거나 흡입 중이 아니면 데미지 처리
    if (!m_pOwner->HasMouthful() && !m_pOwner->IsInhaling())
    {
        Vec2 vKnockbackDir = m_pOwner->GetPos() - _pMonster->GetPos();
        vKnockbackDir.Normalize();
        m_pOwner->TakeDamage(1, vKnockbackDir);
    }
}

void CPlayerCollisionSystem::HandleItemCollisionEnter(CObject* _pItem)
{
    if (!_pItem) return;
    // TODO: 아이템별 처리 로직 추가
}

void CPlayerCollisionSystem::HandleSpecialObjectCollisionEnter(CObject* _pSpecialObject)
{
    if (!_pSpecialObject) return;
    // TODO: 특수 오브젝트 처리 로직 추가
}

// === 타일 충돌 세부 처리 ===

bool CPlayerCollisionSystem::ShouldSetGroundState(CTile* _pTile) const
{
    if (!_pTile || !m_pOwner) return false;

    float tileTop = GetTileTopPosition(_pTile);
    float playerBottom = GetPlayerBottomPosition();

    return (abs(playerBottom - tileTop) < TILE_COLLISION_THRESHOLD &&
        IsPlayerAboveTile(_pTile));
}

void CPlayerCollisionSystem::CorrectPlayerPosition(CTile* _pTile)
{
    if (!_pTile || !m_pOwner) return;

    float tileTop = GetTileTopPosition(_pTile);
    float playerBottom = GetPlayerBottomPosition();

    if (playerBottom > tileTop + POSITION_CORRECTION_THRESHOLD)
    {
        Vec2 vPlayerColliderScale = GetPlayerColliderScale();
        float correctedY = tileTop - vPlayerColliderScale.y / 2.f;

        Vec2 vPlayerPos = m_pOwner->GetPos();
        vPlayerPos.y = correctedY;
        m_pOwner->SetPos(vPlayerPos);
    }
}

void CPlayerCollisionSystem::SetGroundState(CTile* _pTile)
{
    if (!_pTile || !m_pOwner) return;

    CRigidBody* pRigidBody = m_pOwner->GetRigidBody();
    if (!pRigidBody) return;

    if (!pRigidBody->IsGround())
    {
        pRigidBody->SetGround(true);
    }
}

void CPlayerCollisionSystem::ResetVerticalVelocity()
{
    if (!m_pOwner) return;

    CRigidBody* pRigidBody = m_pOwner->GetRigidBody();
    if (!pRigidBody) return;

    Vec2 vVelocity = pRigidBody->GetVelocity();
    if (vVelocity.y > 0.f)
    {
        pRigidBody->SetVelocityY(0.f);
    }
}

// === 헬퍼 함수들 ===

bool CPlayerCollisionSystem::IsPlayerAboveTile(CTile* _pTile) const
{
    if (!_pTile || !m_pOwner) return false;

    Vec2 vPlayerPos = m_pOwner->GetPos();
    Vec2 vTilePos = _pTile->GetPos();

    return vPlayerPos.y < vTilePos.y;
}

float CPlayerCollisionSystem::GetTileTopPosition(CTile* _pTile) const
{
    if (!_pTile) return 0.f;

    Vec2 vTilePos = _pTile->GetPos();
    Vec2 vTileColliderScale = GetTileColliderScale(_pTile);

    return vTilePos.y - vTileColliderScale.y / 2.f;
}

float CPlayerCollisionSystem::GetPlayerBottomPosition() const
{
    if (!m_pOwner) return 0.f;

    Vec2 vPlayerPos = m_pOwner->GetPos();
    Vec2 vPlayerColliderScale = GetPlayerColliderScale();

    return vPlayerPos.y + vPlayerColliderScale.y / 2.f;
}

Vec2 CPlayerCollisionSystem::GetPlayerColliderScale() const
{
    if (!m_pOwner) return Vec2(0.f, 0.f);

    CCollider* pCollider = m_pOwner->GetCollider();
    return pCollider ? pCollider->GetScale() : Vec2(0.f, 0.f);
}

Vec2 CPlayerCollisionSystem::GetTileColliderScale(CTile* _pTile) const
{
    if (!_pTile) return Vec2(0.f, 0.f);

    CCollider* pCollider = _pTile->GetCollider();
    return pCollider ? pCollider->GetScale() : Vec2(0.f, 0.f);
}

// === 유틸리티 함수들 ===

bool CPlayerCollisionSystem::IsInvincibleState() const
{
    if (!m_pOwner) return false;

    if (m_pOwner->GetHealthSystem())
    {
        return m_pOwner->GetHealthSystem()->IsInvincible();
    }

    return false;
}

bool CPlayerCollisionSystem::IsCollidingWithOtherSolidTiles(CTile* _excludeTile) const
{
    if (!m_pOwner || !m_pOwner->GetCollider()) return false;

    CCollider* pPlayerCollider = m_pOwner->GetCollider();
    const vector<CCollider*>& collidingColliders = pPlayerCollider->GetCollidingColliders();

    for (CCollider* pOtherCollider : collidingColliders)
    {
        if (!pOtherCollider || !pOtherCollider->GetOwner()) continue;

        CObject* pOtherObj = pOtherCollider->GetOwner();
        if (IsTileType(pOtherObj->GetType()))
        {
            CTile* pTile = dynamic_cast<CTile*>(pOtherObj);
            if (pTile && pTile->IsSolid() && pTile != _excludeTile)
            {
                return true;
            }
        }
    }

    return false;
}

bool CPlayerCollisionSystem::IsMonsterType(OBJECT_TYPE _eType)
{
    return _eType >= OBJECT_TYPE::MONSTER_WADDLE_DEE &&
        _eType <= OBJECT_TYPE::MONSTER_SPARKY;
}

bool CPlayerCollisionSystem::IsTileType(OBJECT_TYPE _eType)
{
    return _eType >= OBJECT_TYPE::TILE_GROUND &&
        _eType <= OBJECT_TYPE::TILE_INVISIBLE;
}

bool CPlayerCollisionSystem::IsItemType(OBJECT_TYPE _eType)
{
    return _eType >= OBJECT_TYPE::ITEM_STAR &&
        _eType <= OBJECT_TYPE::ITEM_ABILITY_STAR;
}

bool CPlayerCollisionSystem::IsSpecialObjectType(OBJECT_TYPE _eType)
{
    return _eType >= OBJECT_TYPE::OBJECT_DOOR &&
        _eType <= OBJECT_TYPE::OBJECT_MIRROR;
}