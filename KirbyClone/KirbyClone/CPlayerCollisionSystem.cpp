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
{
}

CPlayerCollisionSystem::~CPlayerCollisionSystem()
{
}

void CPlayerCollisionSystem::Init()
{
    // 필요시 초기화 로직 추가
}

// === 충돌 처리 메인 인터페이스 ===

void CPlayerCollisionSystem::HandleCollisionEnter(CCollider* _pOther)
{
    if (!_pOther || !m_pOwner) return;

    CObject* pOtherObj = _pOther->GetOwner();
    if (!pOtherObj) return;

    // 무적 상태 확인
    if (IsInvincibleState())
    {
        return;
    }

    // 오브젝트 타입별 처리
    OBJECT_TYPE eType = pOtherObj->GetType();

    if (IsTileType(eType))
    {
        CTile* pTile = dynamic_cast<CTile*>(pOtherObj);
        if (pTile)
            HandleTileCollisionEnter(pTile);
    }
    else if (IsMonsterType(eType))
    {
        CMonster* pMonster = dynamic_cast<CMonster*>(pOtherObj);
        if (pMonster)
            HandleMonsterCollisionEnter(pMonster);
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

    // 지속적인 충돌 처리 (주로 타일)
    OBJECT_TYPE eType = pOtherObj->GetType();

    if (IsTileType(eType))
    {
        CTile* pTile = dynamic_cast<CTile*>(pOtherObj);
        if (pTile)
            HandleTileCollision(pTile);
    }
}

void CPlayerCollisionSystem::HandleCollisionExit(CCollider* _pOther)
{
    if (!_pOther || !m_pOwner) return;

    CObject* pOtherObj = _pOther->GetOwner();
    if (!pOtherObj) return;

    // 타일에서 벗어날 때 Ground 상태 처리
    OBJECT_TYPE eType = pOtherObj->GetType();

    if (IsTileType(eType))
    {
        CTile* pTile = dynamic_cast<CTile*>(pOtherObj);
        if (pTile)
            HandleTileCollisionExit(pTile);
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

    // 플레이어가 타일 위에 올바르게 서 있는지 확인
    if (ShouldSetGroundState(_pTile))
    {
        CorrectPlayerPosition(_pTile);
        UpdateGroundState(_pTile);
        ResetVerticalVelocity();
    }
}

void CPlayerCollisionSystem::HandleTileCollisionExit(CTile* _pTile)
{
    if (!_pTile || !_pTile->IsSolid()) return;

    CRigidBody* pRigidBody = m_pOwner->GetRigidBody();
    if (!pRigidBody) return;

    Vec2 vVelocity = pRigidBody->GetVelocity();

    // 위쪽으로 벗어나는 이동(점프) 중일 때만 Ground 해제
    if (vVelocity.y < JUMP_VELOCITY_THRESHOLD)
    {
        pRigidBody->SetGround(false);
    }
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
            // 몬스터 흡수 처리 (직접 시스템 접근)
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
        // 넉백 방향 계산 (몬스터에서 플레이어 방향)
        Vec2 vMonsterPos = _pMonster->GetPos();
        Vec2 vPlayerPos = m_pOwner->GetPos();
        Vec2 vKnockbackDir = vPlayerPos - vMonsterPos;
        vKnockbackDir.Normalize();

        // 데미지 적용
        m_pOwner->TakeDamage(1, vKnockbackDir);
    }
}

void CPlayerCollisionSystem::HandleItemCollisionEnter(CObject* _pItem)
{
    if (!_pItem) return;

    // TODO: 아이템별 처리 로직 추가
    // 예: 체력 회복, 파워업 등
}

void CPlayerCollisionSystem::HandleSpecialObjectCollisionEnter(CObject* _pSpecialObject)
{
    if (!_pSpecialObject) return;

    // TODO: 특수 오브젝트 처리 로직 추가
    // 예: 문, 스위치, 이동 플랫폼 등
}

// === 타일 충돌 세부 처리 ===

bool CPlayerCollisionSystem::ShouldSetGroundState(CTile* _pTile) const
{
    if (!_pTile || !m_pOwner) return false;

    float tileTop = GetTileTopPosition(_pTile);
    float playerBottom = GetPlayerBottomPosition();

    // 플레이어 바닥과 타일 윗면이 거의 맞닿아 있고, 플레이어가 위에 있는지 확인
    return (abs(playerBottom - tileTop) < TILE_COLLISION_THRESHOLD &&
        IsPlayerAboveTile(_pTile));
}

void CPlayerCollisionSystem::CorrectPlayerPosition(CTile* _pTile)
{
    if (!_pTile || !m_pOwner) return;

    float tileTop = GetTileTopPosition(_pTile);
    float playerBottom = GetPlayerBottomPosition();

    // 미세한 위치 보정이 필요한 경우
    if (playerBottom > tileTop + POSITION_CORRECTION_THRESHOLD)
    {
        Vec2 vPlayerColliderScale = GetPlayerColliderScale();
        float correctedY = tileTop - vPlayerColliderScale.y / 2.f;

        Vec2 vPlayerPos = m_pOwner->GetPos();
        vPlayerPos.y = correctedY;
        m_pOwner->SetPos(vPlayerPos);
    }
}

void CPlayerCollisionSystem::UpdateGroundState(CTile* _pTile)
{
    if (!_pTile || !m_pOwner) return;

    CRigidBody* pRigidBody = m_pOwner->GetRigidBody();
    if (!pRigidBody) return;

    // Ground 상태가 아닐 때만 설정 (중복 설정 방지)
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

    // 아래로 떨어지는 속도가 있다면 제거
    if (vVelocity.y > 0.f)
    {
        pRigidBody->SetVelocityY(0.f);
    }
}

// === 충돌 계산 헬퍼 함수들 ===

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

// === 무적 상태 체크 ===

bool CPlayerCollisionSystem::IsInvincibleState() const
{
    if (!m_pOwner) return false;

    // 체력 시스템을 통한 무적 상태 확인
    if (m_pOwner->GetHealthSystem())
    {
        return m_pOwner->GetHealthSystem()->IsInvincible();
    }

    return false;
}

// === 오브젝트 타입 유틸리티 함수들 ===

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