#include "pch.h"
#include "CPlayerCollisionSystem.h"
#include "CPlayerInhaleSystem.h"
#include "CPlayerHealthSystem.h"
#include "CPlayer.h"
#include "CCollider.h"
#include "CObject.h"
#include "CTile.h"
#include "CMonster.h"
#include "CProjectile.h"
#include "CRigidBody.h"
#include "CTimeMgr.h"
#include "CScene.h"
#include "CSceneMgr.h"
#include "CEventMgr.h"

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

// CPlayerCollisionSystem.cpp
void CPlayerCollisionSystem::UpdateGroundState()
{
    if (!m_pOwner) return;

    CRigidBody* pRigidBody = m_pOwner->GetRigidBody();
    if (!pRigidBody) return;

    bool isCurrentlyGrounded = pRigidBody->IsGround();
    bool isActuallyOnGround = IsPlayerOnGround();

    if (isCurrentlyGrounded && !isActuallyOnGround)
    {
        // Ground 상태만 수정 (상태 변경은 TransitionTable이 자동 처리)
        pRigidBody->SetGround(false);

        // 상태 변경은 다음 프레임 StateMachine::Update()에서 자동으로 처리됨
    }
}

bool CPlayerCollisionSystem::IsPlayerOnGround() const
{
    if (!m_pOwner) return false;

    // 발 아래 타일이 있는지 체크
    return CheckGroundBelow();
}

bool CPlayerCollisionSystem::CheckGroundBelow() const
{
    if (!m_pOwner) return false;

    CTile* pSupportingTile = FindSupportingTile();
    return (pSupportingTile != nullptr);
}

CTile* CPlayerCollisionSystem::FindSupportingTile() const
{
    if (!m_pOwner) return nullptr;

    Vec2 vPlayerPos = m_pOwner->GetPos();
    Vec2 vPlayerScale = GetPlayerColliderScale();

    // 플레이어 발 아래 영역 정의
    float playerLeft = vPlayerPos.x - vPlayerScale.x / 2.f;
    float playerRight = vPlayerPos.x + vPlayerScale.x / 2.f;
    float playerBottom = vPlayerPos.y + vPlayerScale.y / 2.f;

    // 발 아래 약간의 여유 공간 (픽셀 단위)
    float groundCheckDistance = 8.f;
    float checkY = playerBottom + groundCheckDistance;

    // 현재 씬의 모든 타일 검사
    CScene* pCurScene = CSceneMgr::GetInst()->GetCurScene();
    if (!pCurScene) return nullptr;

    const vector<CObject*>& vecTiles = pCurScene->GetGroupObject(GROUP_TYPE::TILE);

    for (CObject* pObj : vecTiles)
    {
        if (!pObj || pObj->IsDead()) continue;

        CTile* pTile = dynamic_cast<CTile*>(pObj);
        if (!pTile || !pTile->IsSolid()) continue;

        Vec2 vTilePos = pTile->GetPos();
        Vec2 vTileScale = GetTileColliderScale(pTile);

        // 타일 영역 계산
        float tileLeft = vTilePos.x - vTileScale.x / 2.f;
        float tileRight = vTilePos.x + vTileScale.x / 2.f;
        float tileTop = vTilePos.y - vTileScale.y / 2.f;
        float tileBottom = vTilePos.y + vTileScale.y / 2.f;

        // 플레이어가 타일 위에 있고, 발 아래 영역이 타일과 겹치는지 체크
        bool horizontalOverlap = (playerRight > tileLeft) && (playerLeft < tileRight);
        bool isAboveTile = (playerBottom <= tileTop + TILE_COLLISION_THRESHOLD);
        bool isWithinCheckRange = (checkY >= tileTop) && (playerBottom <= tileTop + groundCheckDistance);

        if (horizontalOverlap && isAboveTile && isWithinCheckRange)
        {
            return pTile;
        }
    }

    return nullptr;
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
    else if (dynamic_cast<CProjectile*>(pOtherObj))
    {
        // 투사체(PROJ_MONSTER) 충돌 처리
        CProjectile* pProjectile = dynamic_cast<CProjectile*>(pOtherObj);
        if (pProjectile)
            HandleProjectileCollisionEnter(pProjectile);
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
    else if (eType == OBJECT_TYPE::MONSTER_WHISPY_WOODS)
    {
        // WhispyWoods를 벽처럼 처리
        HandleWhispyWoodsWallCollision(pOtherObj);
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
    if (!_pTile) return;


    // 보스 트리거 타일 체크
    if (_pTile->GetVisualType() == TILE_VISUAL_TYPE::BOSS_TRIGGER)
    {
        // 트리거가 비활성화된 경우 무시
        if (!_pTile->IsTriggerActive())
        {
            return;
        }
        
        // 보스 트리거 이벤트 발생
        tEvent evn = {};
        evn.eType = EVENT_TYPE::BOSS_BATTLE_START;
        evn.lParam = (DWORD_PTR)_pTile; // 트리거 타일 참조 전달 (카메라 고정 위치 정보 포함)
        evn.wParam = 0; // 추가 데이터
        CEventMgr::GetInst()->AddEvent(evn);
        return;
    }

    // 기존 Solid 타일 처리
    if (!_pTile->IsSolid()) return;

    // 타일 진입 시 특별한 처리가 필요하면 여기에 추가
}

void CPlayerCollisionSystem::HandleTileCollision(CTile* _pTile)
{
    CTile* pTile = dynamic_cast<CTile*>(_pTile);
    if (!pTile || !pTile->IsSolid())
        return;

    CRigidBody* pRigidBody = m_pOwner->GetRigidBody();
    if (!pRigidBody)
        return;

    Vec2 vMyPos = m_pOwner->GetPos();
    Vec2 vTilePos = pTile->GetPos();
    Vec2 vMyScale = GetPlayerColliderScale();
    Vec2 vTileScale = GetTileColliderScale(pTile);

    // 현재 속도 기반으로 충돌 방향 추정
    Vec2 vVelocity = pRigidBody->GetVelocity();
    float deltaTime = CTimeMgr::GetInst()->GetfDT();

    // 이전 프레임 위치 추정
    Vec2 vPrevPos = vMyPos - vVelocity * deltaTime;

    // 충돌 박스 경계 계산
    float myLeft = vMyPos.x - vMyScale.x / 2.f;
    float myRight = vMyPos.x + vMyScale.x / 2.f;
    float myTop = vMyPos.y - vMyScale.y / 2.f;
    float myBottom = vMyPos.y + vMyScale.y / 2.f;

    float tileLeft = vTilePos.x - vTileScale.x / 2.f;
    float tileRight = vTilePos.x + vTileScale.x / 2.f;
    float tileTop = vTilePos.y - vTileScale.y / 2.f;
    float tileBottom = vTilePos.y + vTileScale.y / 2.f;

    // 겹침 계산
    float overlapX = min(myRight, tileRight) - max(myLeft, tileLeft);
    float overlapY = min(myBottom, tileBottom) - max(myTop, tileTop);

    if (overlapX <= 0 || overlapY <= 0)
        return;

    // 속도 방향을 고려한 충돌 처리
    bool collisionFromLeft = vVelocity.x > 0 && vPrevPos.x < tileLeft;
    bool collisionFromRight = vVelocity.x < 0 && vPrevPos.x > tileRight;
    bool collisionFromTop = vVelocity.y > 0 && vPrevPos.y < tileTop;
    bool collisionFromBottom = vVelocity.y < 0 && vPrevPos.y > tileBottom;

    // 우선순위: 수직 충돌을 먼저 처리 (착지와 천장 충돌)
    if (collisionFromTop && overlapY <= overlapX)
    {
        // 위에서 아래로 떨어져서 착지
        float correctedY = tileTop - vMyScale.y / 2.f;
        m_pOwner->SetPos(Vec2(vMyPos.x, correctedY));
        pRigidBody->SetGround(true);
        pRigidBody->SetVelocityY(0.f);
    }
    else if (collisionFromBottom && overlapY <= overlapX)
    {
        // 아래에서 위로 올라와서 천장 충돌
        float correctedY = tileBottom + vMyScale.y / 2.f;
        m_pOwner->SetPos(Vec2(vMyPos.x, correctedY));
        pRigidBody->SetVelocityY(0.f);
    }
    else if (collisionFromLeft)
    {
        // 왼쪽에서 오른쪽으로 이동해서 벽 충돌
        float correctedX = tileLeft - vMyScale.x / 2.f;
        m_pOwner->SetPos(Vec2(correctedX, vMyPos.y));
        pRigidBody->SetVelocityX(0.f);
    }
    else if (collisionFromRight)
    {
        // 오른쪽에서 왼쪽으로 이동해서 벽 충돌
        float correctedX = tileRight + vMyScale.x / 2.f;
        m_pOwner->SetPos(Vec2(correctedX, vMyPos.y));
        pRigidBody->SetVelocityX(0.f);
    }
    else
    {
        // 방향을 정확히 알 수 없는 경우 - 최소 겹침 방향으로 처리
        if (overlapX < overlapY)
        {
            // 수평 분리
            if (vMyPos.x < vTilePos.x)
            {
                float correctedX = tileLeft - vMyScale.x / 2.f;
                m_pOwner->SetPos(Vec2(correctedX, vMyPos.y));
                if (vVelocity.x > 0) pRigidBody->SetVelocityX(0.f);
            }
            else
            {
                float correctedX = tileRight + vMyScale.x / 2.f;
                m_pOwner->SetPos(Vec2(correctedX, vMyPos.y));
                if (vVelocity.x < 0) pRigidBody->SetVelocityX(0.f);
            }
        }
        else
        {
            // 수직 분리
            if (vMyPos.y < vTilePos.y)
            {
                float correctedY = tileTop - vMyScale.y / 2.f;
                m_pOwner->SetPos(Vec2(vMyPos.x, correctedY));
                pRigidBody->SetGround(true);
                if (vVelocity.y > 0) pRigidBody->SetVelocityY(0.f);
            }
            else
            {
                float correctedY = tileBottom + vMyScale.y / 2.f;
                m_pOwner->SetPos(Vec2(vMyPos.x, correctedY));
                if (vVelocity.y < 0) pRigidBody->SetVelocityY(0.f);
            }
        }
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
        // 넉백 방향 계산
        Vec2 vMonsterPos = _pMonster->GetPos();
        Vec2 vPlayerPos = m_pOwner->GetPos();
        Vec2 vKnockbackDir = vPlayerPos - vMonsterPos;
        vKnockbackDir.Normalize();

        // === 1. 플레이어에게 피격 요청 (즉시) ===
        m_pOwner->RequestDamage(vKnockbackDir);

        // === 2. 실제 데미지 처리는 이벤트로 등록 ===
        tEvent playerDamageEvent;
        playerDamageEvent.eType = EVENT_TYPE::PLAYER_DAMAGE;
        playerDamageEvent.wParam = (DWORD_PTR)m_pOwner;
        playerDamageEvent.lParam = (DWORD_PTR)new Vec2(vKnockbackDir);
        CEventMgr::GetInst()->AddEvent(playerDamageEvent);

        // === 3. 몬스터 데미지 이벤트 등록 ===
        tEvent monsterDamageEvent;
        monsterDamageEvent.eType = EVENT_TYPE::MONSTER_DAMAGE;
        monsterDamageEvent.wParam = (DWORD_PTR)_pMonster;
        monsterDamageEvent.lParam = (DWORD_PTR)m_pOwner;  // 플레이어 객체 전달 (위치 정보 포함)
        CEventMgr::GetInst()->AddEvent(monsterDamageEvent);

        // === 상태 전환은 전환 테이블에서 자동으로 감지! ===
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

void CPlayerCollisionSystem::HandleProjectileCollisionEnter(CProjectile* _pProjectile)
{
    if (!_pProjectile || !m_pOwner) return;

    // 몬스터 투사체와의 충돌 시 플레이어가 데미지를 받음
    float fDamage = _pProjectile->GetDamage();
    
    // 투사체 방향에 따른 넉백 방향 계산 (왼쪽 또는 오른쪽)
    Vec2 knockbackDir = Vec2(_pProjectile->GetDirection().x * 200.f, 0.f);
    
    // 플레이어 데미지 처리 (넉백 포함)
    m_pOwner->TakeDamage((int)fDamage, knockbackDir);
    
    // 투사체 삭제
    _pProjectile->SetDead();
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
        _eType <= OBJECT_TYPE::TILE_TRIGGER;
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

void CPlayerCollisionSystem::HandleWhispyWoodsWallCollision(CObject* _pWhispyWoods)
{
    if (!_pWhispyWoods || !m_pOwner) return;

    // 플레이어와 WhispyWoods의 충돌체 정보 가져오기
    CCollider* pPlayerCollider = m_pOwner->GetCollider();
    CCollider* pWhispyCollider = _pWhispyWoods->GetCollider();
    
    if (!pPlayerCollider || !pWhispyCollider) return;

    // 플레이어와 WhispyWoods의 위치와 크기
    Vec2 vPlayerPos = m_pOwner->GetPos();
    Vec2 vPlayerScale = pPlayerCollider->GetScale();
    
    Vec2 vWhispyPos = _pWhispyWoods->GetPos();
    Vec2 vWhispyScale = pWhispyCollider->GetScale();

    // 충돌 영역 계산
    float fPlayerLeft = vPlayerPos.x - vPlayerScale.x * 0.5f;
    float fPlayerRight = vPlayerPos.x + vPlayerScale.x * 0.5f;
    float fPlayerTop = vPlayerPos.y - vPlayerScale.y * 0.5f;
    float fPlayerBottom = vPlayerPos.y + vPlayerScale.y * 0.5f;

    float fWhispyLeft = vWhispyPos.x - vWhispyScale.x * 0.5f;
    float fWhispyRight = vWhispyPos.x + vWhispyScale.x * 0.5f;
    float fWhispyTop = vWhispyPos.y - vWhispyScale.y * 0.5f;
    float fWhispyBottom = vWhispyPos.y + vWhispyScale.y * 0.5f;

    // 겹침 계산
    float fOverlapX = min(fPlayerRight, fWhispyRight) - max(fPlayerLeft, fWhispyLeft);
    float fOverlapY = min(fPlayerBottom, fWhispyBottom) - max(fPlayerTop, fWhispyTop);

    if (fOverlapX > 0 && fOverlapY > 0)
    {
        Vec2 vSeparation(0.f, 0.f);

        // 더 작은 겹침 방향으로 플레이어를 밀어냄
        if (fOverlapX < fOverlapY)
        {
            // 수평 방향으로 분리
            if (vPlayerPos.x < vWhispyPos.x)
            {
                vSeparation.x = -fOverlapX; // 왼쪽으로 밀어냄
            }
            else
            {
                vSeparation.x = fOverlapX;  // 오른쪽으로 밀어냄
            }
        }
        else
        {
            // 수직 방향으로 분리
            if (vPlayerPos.y < vWhispyPos.y)
            {
                vSeparation.y = -fOverlapY; // 위로 밀어냄
            }
            else
            {
                vSeparation.y = fOverlapY;  // 아래로 밀어냄
            }
        }

        // 플레이어 위치 조정
        Vec2 vNewPos = vPlayerPos + vSeparation;
        m_pOwner->SetPos(vNewPos);

        // 플레이어가 WhispyWoods 쪽으로 이동하려는 속도를 제거
        CRigidBody* pRigidBody = m_pOwner->GetRigidBody();
        if (pRigidBody)
        {
            Vec2 vVelocity = pRigidBody->GetVelocity();
            
            if (fOverlapX < fOverlapY) // 수평 분리
            {
                if ((vSeparation.x < 0 && vVelocity.x > 0) || // 왼쪽으로 밀렸는데 오른쪽으로 가려 함
                    (vSeparation.x > 0 && vVelocity.x < 0))   // 오른쪽으로 밀렸는데 왼쪽으로 가려 함
                {
                    vVelocity.x = 0.f;
                    pRigidBody->SetVelocity(vVelocity);
                }
            }
            else // 수직 분리
            {
                if ((vSeparation.y < 0 && vVelocity.y > 0) || // 위로 밀렸는데 아래로 가려 함
                    (vSeparation.y > 0 && vVelocity.y < 0))   // 아래로 밀렸는데 위로 가려 함
                {
                    vVelocity.y = 0.f;
                    pRigidBody->SetVelocity(vVelocity);
                }
            }
        }
    }
}