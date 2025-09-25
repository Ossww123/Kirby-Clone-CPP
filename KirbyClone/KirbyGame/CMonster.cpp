#include "gamePCH.h"
#include "CMonster.h"

#include "CTimeMgr.h"
#include "CCollider.h"
#include "CAnimator.h"
#include "CRigidBody.h"
#include "CResMgr.h"
#include "CTexture.h"
#include "CCamera.h"
#include "CSceneMgr.h"
#include "CScene.h"
#include "CAnimationDataMgr.h"
#include "CSoundMgr.h"

// 공용 타일 충돌 모듈(앞서 설계)
#include "TileCollision.h"

CMonster::CMonster()
    : CObject(OBJECT_TYPE::MONSTER_WADDLE_DEE) // 기본값, 파생에서 바꾸면 됨
    , m_eCurState(MONSTER_STATE::IDLE)
    , m_ePrevState(MONSTER_STATE::END)
    , m_fStateTimer(0.f)
    , m_iDir(-1)
    , m_fIdleTime(DEFAULT_IDLE_TIME)
    , m_pEnemyTex(nullptr)
{
    SetGroup(GROUP_TYPE::MONSTER);

    CreateCollider();
    if (auto* col = GetCollider())
        col->SetScale(Vec2(56.f, 56.f));

    CreateAnimator();
    CreateRigidBody();

    if (auto* rb = GetRigidBody())
    {
        rb->SetMass(0.8f);
        rb->SetMaxVelocity(200.f);
        rb->SetFriction(0.f);
        rb->SetUseGravity(true);
    }

    SetFlipX(m_iDir > 0);
    LoadEnemySpriteSheet();

    m_stats.maxHp = 1;
    m_stats.hp = 1;
    m_stats.moveSpeed = 80.f;
    m_stats.contactDamage = 1;
}

CMonster::~CMonster()
{
    // 리소스는 매니저 소유
}

void CMonster::Update()
{
    const float dt = CTimeMgr::GetInst()->GetfDT();

    UpdateTimers(dt);
    UpdateState();

    if (auto* rb = GetRigidBody()) rb->Update();
    if (auto* an = GetAnimator())  an->Update();
}

void CMonster::OnCollisionEnter(CCollider* _pOther)
{
    CObject* other = _pOther ? _pOther->GetOwner() : nullptr;
    if (!other) return;

    if (other->GetGroup() == GROUP_TYPE::TILE)
    {
        TileContactInfo contact;
        if (TileCollision::ResolveAgainstTile(*this, *other, TileCollisionOpts{}, contact))
        {
            if (contact.wall)   TurnAround();
            if (contact.ground) { /* 착지 이펙트 등 필요 시 */ }
        }
    }
}

void CMonster::OnCollision(CCollider* _pOther)
{
    CObject* other = _pOther ? _pOther->GetOwner() : nullptr;
    if (!other) return;

    if (other->GetGroup() == GROUP_TYPE::TILE)
    {
        TileContactInfo contact;
        if (TileCollision::ResolveAgainstTile(*this, *other, TileCollisionOpts{}, contact))
        {
            if (contact.wall) TurnAround();
        }
    }
}

void CMonster::OnCollisionExit(CCollider* _pOther)
{
    CObject* other = _pOther ? _pOther->GetOwner() : nullptr;
    if (!other) return;

    if (other->GetGroup() == GROUP_TYPE::TILE)
    {
        if (auto* rb = GetRigidBody())
        {
            Vec2 v = rb->GetVelocity();
            if (v.y < -50.f) rb->SetGround(false);
        }
    }
}

// ===== 애니메이션 로딩/매핑 =====

void CMonster::LoadAnimationsFromFile(const std::wstring& _strFileName)
{
    if (auto* an = GetAnimator())
    {
        CAnimationDataMgr::GetInst()->LoadAnimationsIntoAnimator(an, _strFileName);
        SetupAnimationMapping();
        an->Play(L"IDLE", true);
    }
}

void CMonster::MapAnim(MONSTER_STATE s, const std::wstring& name)
{
    m_mapStateToAnimation[s] = name;
}

void CMonster::MapAnims(std::initializer_list<std::pair<MONSTER_STATE, const wchar_t*>> list)
{
    for (const auto& p : list)
        m_mapStateToAnimation[p.first] = std::wstring(p.second);
}

void CMonster::ClearAnimMap()
{
    m_mapStateToAnimation.clear();
}

void CMonster::ChangeState(MONSTER_STATE _eState)
{
    m_ePrevState = m_eCurState;
    m_eCurState = _eState;
    m_fStateTimer = 0.f;

    if (auto* an = GetAnimator())
    {
        auto it = m_mapStateToAnimation.find(_eState);
        if (it != m_mapStateToAnimation.end())
        {
            const std::wstring& name = it->second;
            const bool bLoop = (_eState != MONSTER_STATE::DAMAGE);
            an->Play(name, bLoop);
        }
        else
        {
            // 폴백
            switch (_eState)
            {
            case MONSTER_STATE::IDLE:           an->Play(L"IDLE", true);  break;
            case MONSTER_STATE::WALK:           an->Play(L"WALK", true);  break;
            case MONSTER_STATE::TURN:           an->Play(L"WALK", true);  break;
            case MONSTER_STATE::FLY:            an->Play(L"FLY", true);  break;
            case MONSTER_STATE::ATTACK_READY:   an->Play(L"ATTACK_READY", false); break;
            case MONSTER_STATE::ATTACK:         an->Play(L"ATTACK", false); break;
            case MONSTER_STATE::DAMAGE:         an->Play(L"DAMAGE", false); break;
            case MONSTER_STATE::BEING_INHALED:  an->Play(L"DAMAGE", true);  break;
            default:                            an->Play(L"IDLE", true);  break;
            }
        }
    }
}

// ===== 전투/피격 =====

void CMonster::TakeDamage(int dmg, Vec2 /*knockback*/)
{
    if (IsInvincible() || !IsAlive()) return;

    m_stats.hp = max(0, m_stats.hp - max(1, dmg));
    m_fInvTime = IFAME_DURATION;
    m_fHurtStun = HURT_STUN_TIME;

    if (m_stats.hp <= 0)
    {
        // 원한다면 DEATH 상태를 도입해서 연출 후 SetDead() 하도록 변경 가능
        SetDead();
        ChangeState(MONSTER_STATE::DAMAGE); // 마지막 히트 리액션 재생(1회)
        return;
    }

    ChangeState(MONSTER_STATE::DAMAGE);
}

// ===== 이동/리소스 =====

void CMonster::TurnAround()
{
    m_iDir *= -1;
    SetFlipX(m_iDir > 0);
}

void CMonster::MoveHorizontal(float speed)
{
    if (auto* rb = GetRigidBody())
        rb->SetVelocityX(speed * m_iDir);
}

void CMonster::LoadEnemySpriteSheet()
{
    if (!m_pEnemyTex)
        m_pEnemyTex = CResMgr::GetInst()->LoadTexture(L"EnemiesSprite", L"texture\\enemy\\enemies.bmp");
}

// ===== 상태 머신 틱 =====

void CMonster::UpdateState()
{
    switch (m_eCurState)
    {
    case MONSTER_STATE::IDLE:           UpdateIdle();          break;
    case MONSTER_STATE::WALK:           UpdateWalk();          break;
    case MONSTER_STATE::FLY:            UpdateFly();           break;
    case MONSTER_STATE::TURN:           UpdateTurn();          break;
    case MONSTER_STATE::DAMAGE:         UpdateDamage();        break;
    case MONSTER_STATE::BEING_INHALED:  UpdateBeingInhaled();  break;
    case MONSTER_STATE::ATTACK_READY:   UpdateAttackReady();   break;
    case MONSTER_STATE::ATTACK:         UpdateAttack();        break;
    default: break;
    }
}

void CMonster::UpdateIdle()
{
    if (m_fStateTimer >= m_fIdleTime)
        ChangeState(MONSTER_STATE::WALK);
}

void CMonster::UpdateWalk()
{
    if (m_fHurtStun <= 0.f)
        Move(); // 파생이 속도 의도 결정(보통 MoveHorizontal(Stats().moveSpeed))
}

void CMonster::UpdateFly()
{
    if (m_fHurtStun <= 0.f)
        Move();

    if (m_fStateTimer >= 3.f)
    {
        TurnAround();
        m_fStateTimer = 0.f;
    }
}

void CMonster::UpdateTurn()
{
    if (auto* rb = GetRigidBody()) rb->SetVelocityX(0.f);
    if (m_fStateTimer >= TURN_DURATION)
    {
        TurnAround();
        ChangeState(MONSTER_STATE::WALK);
    }
}

void CMonster::UpdateDamage()
{
    if (auto* rb = GetRigidBody()) rb->SetVelocityX(0.f);

    if (m_fStateTimer >= DAMAGE_DURATION)
    {
        if (IsAlive())
            ChangeState(MONSTER_STATE::WALK);
        // 죽었으면 씬/스포너에서 정리
    }
}

void CMonster::UpdateBeingInhaled()
{
    if (auto* rb = GetRigidBody()) rb->SetVelocityX(0.f);
}

void CMonster::UpdateAttackReady()
{
    if (auto* rb = GetRigidBody()) rb->SetVelocityX(0.f);
    // 기본 1초 준비 → 필요하면 파생에서 상태 전환 직접 호출
    if (m_fStateTimer >= 1.f)
        ChangeState(MONSTER_STATE::ATTACK);
}

void CMonster::UpdateAttack()
{
    if (auto* rb = GetRigidBody()) rb->SetVelocityX(0.f);
    if (m_fStateTimer >= 1.f)
        ChangeState(MONSTER_STATE::WALK);
}

void CMonster::UpdateTimers(float dt)
{
    m_fStateTimer += dt;
    if (m_fInvTime > 0.f) m_fInvTime = max(0.f, m_fInvTime - dt);
    if (m_fHurtStun > 0.f) m_fHurtStun = max(0.f, m_fHurtStun - dt);
}
