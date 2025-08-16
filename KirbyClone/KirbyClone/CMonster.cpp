#include "pch.h"
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
#include "CTile.h"
#include "CAnimationDataMgr.h"

CMonster::CMonster()
    : CObject(OBJECT_TYPE::MONSTER_WADDLE_DEE)  // 기본값, 자식에서 변경
    , m_eCurState(MONSTER_STATE::IDLE)
    , m_ePrevState(MONSTER_STATE::END)
    , m_fStateTimer(0.f)
    , m_fSpeed(DEFAULT_SPEED)
    , m_iDir(1)
    , m_fIdleTime(DEFAULT_IDLE_TIME)
    , m_bHitWall(false)
    , m_fGroundCheckDist(32.f)
    , m_fWallCheckDist(32.f)
    , m_pEnemyTex(nullptr)
{
    // 기본 컴포넌트 생성
    CreateCollider();
    GetCollider()->SetScale(Vec2(56.f, 56.f));

    CreateAnimator();
    CreateRigidBody();

    // 리지드바디 기본 설정
    GetRigidBody()->SetMass(0.8f);
    GetRigidBody()->SetMaxVelocity(200.f);
    GetRigidBody()->SetFriction(8.f);
    GetRigidBody()->SetUseGravity(true);

    // 공통 텍스처 로드
    LoadEnemySpriteSheet();

    // 자식 클래스에서 CreateAnimations() 호출됨
    // 초기 상태는 자식 클래스에서 설정
}

CMonster::~CMonster()
{
    // 상위 클래스 CObject에서 컴포넌트 해제 처리
    // m_pEnemyTex는 리소스 매니저에서 관리하므로 delete 하지 않음
}

void CMonster::Update()
{
    // 상태 업데이트
    UpdateState();

    // 이동 업데이트
    UpdateMove();

    // 컴포넌트 업데이트
    if (nullptr != GetRigidBody())
        GetRigidBody()->Update();

    if (nullptr != GetAnimator())
        GetAnimator()->Update();

    // 상태 타이머 업데이트
    m_fStateTimer += CTimeMgr::GetInst()->GetfDT();
}

void CMonster::Render(HDC _dc)
{
    // 플레이어와 동일한 렌더링 방식
    CAnimator* pAnimator = GetAnimator();
    if (pAnimator)
    {
        pAnimator->Render(_dc);
    }
    else
    {
        // 애니메이터가 없으면 기본 오브젝트 렌더링
        CObject::Render(_dc);
    }

    // 충돌체 렌더링
    if (GetCollider())
    {
        GetCollider()->RenderScaled(_dc, 1.0f);  // TAB키로 토글
    }
}

// === 충돌 처리 ===

void CMonster::OnCollisionEnter(CCollider* _pOther)
{
    CObject* pOtherObj = _pOther->GetOwner();
    if (!pOtherObj)
        return;

    OBJECT_TYPE eType = pOtherObj->GetType();

    // 타일과의 충돌 처리
    if (eType >= OBJECT_TYPE::TILE_GROUND && eType <= OBJECT_TYPE::TILE_INVISIBLE)
    {
        HandleTileCollision(pOtherObj);
    }
}

void CMonster::OnCollision(CCollider* _pOther)
{
    CObject* pOtherObj = _pOther->GetOwner();
    if (!pOtherObj)
        return;

    OBJECT_TYPE eType = pOtherObj->GetType();

    // 타일과의 지속적인 충돌 처리
    if (eType >= OBJECT_TYPE::TILE_GROUND && eType <= OBJECT_TYPE::TILE_INVISIBLE)
    {
        HandleTileCollision(pOtherObj);
    }
}

void CMonster::OnCollisionExit(CCollider* _pOther)
{
    CObject* pOtherObj = _pOther->GetOwner();
    if (!pOtherObj)
        return;

    OBJECT_TYPE eType = pOtherObj->GetType();

    // 타일에서 벗어날 때 처리
    if (eType >= OBJECT_TYPE::TILE_GROUND && eType <= OBJECT_TYPE::TILE_INVISIBLE)
    {
        // Ground 상태 해제 (점프나 낙하 중일 때만)
        if (GetRigidBody())
        {
            Vec2 vVelocity = GetRigidBody()->GetVelocity();
            if (vVelocity.y < -50.f)  // 위로 이동 중
            {
                GetRigidBody()->SetGround(false);
            }
        }
    }
}

void CMonster::HandleTileCollision(CObject* _pTile)
{
    CTile* pTile = dynamic_cast<CTile*>(_pTile);
    if (!pTile)  // IsSolid() 체크 제거
        return;

    if (!GetRigidBody())
        return;

    Vec2 vMyPos = GetPos();
    Vec2 vTilePos = pTile->GetPos();
    Vec2 vMyScale = GetCollider() ? GetCollider()->GetScale() : Vec2(32.f, 32.f);
    Vec2 vTileScale = pTile->GetCollider() ? pTile->GetCollider()->GetScale() : Vec2(64.f, 64.f);

    // 타일 위에 서 있는지 확인
    float tileTop = vTilePos.y - vTileScale.y / 2.f;
    float myBottom = vMyPos.y + vMyScale.y / 2.f;

    // 몬스터가 타일 위에 있고, 아래로 떨어지는 중이거나 정지 상태면 Ground 설정
    if (abs(myBottom - tileTop) < 8.f && vMyPos.y < vTilePos.y)
    {
        Vec2 vVelocity = GetRigidBody()->GetVelocity();

        // 아래로 떨어지는 중이면 위치 보정 및 Ground 설정
        if (vVelocity.y >= 0.f)
        {
            // 위치 보정
            float correctedY = tileTop - vMyScale.y / 2.f;
            SetPos(Vec2(vMyPos.x, correctedY));

            // Ground 설정 및 Y 속도 제거
            GetRigidBody()->SetGround(true);
            GetRigidBody()->SetVelocityY(0.f);
        }
    }
}

void CMonster::LoadAnimationsFromFile(const wstring& _strFileName)
{
    CAnimator* pAnimator = GetAnimator();
    if (!pAnimator)
    {
        return;
    }

    // 애니메이션 파일 로드
    CAnimationDataMgr::GetInst()->LoadAnimationsIntoAnimator(pAnimator, _strFileName);

    // 자식 클래스에서 애니메이션 매핑 설정
    SetupAnimationMapping();

    // 로드 확인
    pAnimator->Play(L"IDLE", true);
}

void CMonster::ChangeState(MONSTER_STATE _eState)
{
    m_ePrevState = m_eCurState;
    m_eCurState = _eState;
    m_fStateTimer = 0.f;

    // 상태에 맞는 애니메이션 재생
    CAnimator* pAnimator = GetAnimator();
    if (pAnimator)
    {
        // 매핑에서 애니메이션 이름 찾기
        auto iter = m_mapStateToAnimation.find(_eState);
        if (iter != m_mapStateToAnimation.end())
        {
            const wstring& animName = iter->second;
            bool bLoop = (_eState != MONSTER_STATE::DAMAGE); // DAMAGE는 반복 안함
            pAnimator->Play(animName, bLoop);
        }
        else
        {
            // 매핑에 없으면 기본 애니메이션 사용
            switch (_eState)
            {
            case MONSTER_STATE::IDLE:
                pAnimator->Play(L"IDLE", true);
                break;
            case MONSTER_STATE::WALK:
                pAnimator->Play(L"WALK", true);
                break;
            case MONSTER_STATE::TURN:
                pAnimator->Play(L"WALK", true);
                break;
            case MONSTER_STATE::DAMAGE:
                pAnimator->Play(L"DAMAGE", false);
                break;
            default:
                pAnimator->Play(L"IDLE", true);
                break;
            }
        }
    }
}

void CMonster::TakeDamage()
{
    ChangeState(MONSTER_STATE::DAMAGE);
}

void CMonster::TurnAround()
{
    m_iDir *= -1;
}

void CMonster::MoveHorizontal(float speed)
{
    if (nullptr != GetRigidBody())
    {
        GetRigidBody()->SetVelocityX(speed * m_iDir);
    }
}

void CMonster::HandleWallCollision()
{
    // 벽 충돌 시 방향 전환
    if (CheckWallAhead())
    {
        TurnAround();
        m_bHitWall = true;
    }
    else
    {
        m_bHitWall = false;
    }
}

void CMonster::LoadEnemySpriteSheet()
{
    if (nullptr == m_pEnemyTex)
    {
        m_pEnemyTex = CResMgr::GetInst()->LoadTexture(L"EnemiesSprite", L"texture\\enemy\\enemies.bmp");
    }
}

void CMonster::UpdateState()
{
    switch (m_eCurState)
    {
    case MONSTER_STATE::IDLE:
        UpdateIdle();
        break;
    case MONSTER_STATE::WALK:
        UpdateWalk();
        break;
    case MONSTER_STATE::FLY:
        UpdateFly();
        break;
    case MONSTER_STATE::TURN:
        UpdateTurn();
        break;
    case MONSTER_STATE::DAMAGE:
        UpdateDamage();
        break;
    case MONSTER_STATE::ATTACK_READY:
        UpdateAttackReady();
        break;
    case MONSTER_STATE::ATTACK:
        UpdateAttack();
        break;
    }
}

void CMonster::UpdateMove()
{
    // 자식 클래스에서 Move() 호출로 실제 이동 처리
    // 이 함수는 Move() 호출 후 추가 처리가 필요할 때 사용
}

void CMonster::UpdateIdle()
{
    // 일정 시간 정지 후 다음 행동 결정
    if (m_fStateTimer >= m_fIdleTime)
    {
        // 기본적으로 걷기 상태로 전환 (자식에서 override 가능)
        ChangeState(MONSTER_STATE::WALK);
    }
}

void CMonster::UpdateWalk()
{
    // 자식 클래스의 Move() 함수 호출
    Move();
}

void CMonster::UpdateFly()
{
    // 기본 비행 로직 (자식에서 override 권장)
    Move();

    // 일정 시간 후 방향 전환
    if (m_fStateTimer >= 3.0f)
    {
        TurnAround();
        m_fStateTimer = 0.f;
    }
}

void CMonster::UpdateTurn()
{
    // 방향 전환 시간
    if (m_fStateTimer >= TURN_DURATION)
    {
        TurnAround();
        ChangeState(MONSTER_STATE::WALK);  // 기본적으로 걷기로 복귀
    }
    else
    {
        // 방향 전환 중에는 멈춤
        if (nullptr != GetRigidBody())
        {
            GetRigidBody()->SetVelocityX(0.f);
        }
    }
}

void CMonster::UpdateDamage()
{
    // 데미지 상태 지속 시간
    if (m_fStateTimer >= DAMAGE_DURATION)
    {
        ChangeState(MONSTER_STATE::WALK);
    }

    // 데미지 중에는 이동 정지
    if (nullptr != GetRigidBody())
    {
        GetRigidBody()->SetVelocityX(0.f);
    }
}

void CMonster::UpdateAttackReady()
{
    // 기본 공격 준비 시간 (자식에서 override 권장)
    if (m_fStateTimer >= 1.0f)
    {
        ChangeState(MONSTER_STATE::ATTACK);
    }

    // 공격 준비 중에는 이동 정지
    if (nullptr != GetRigidBody())
    {
        GetRigidBody()->SetVelocityX(0.f);
    }
}

void CMonster::UpdateAttack()
{
    // 기본 공격 지속 시간 (자식에서 override 권장)
    if (m_fStateTimer >= 1.0f)
    {
        ChangeState(MONSTER_STATE::WALK);
    }

    // 공격 중에는 이동 정지
    if (nullptr != GetRigidBody())
    {
        GetRigidBody()->SetVelocityX(0.f);
    }
}

bool CMonster::CheckWallAhead()
{
    // 실제 타일과의 충돌 검사 구현
    CScene* pCurScene = CSceneMgr::GetInst()->GetCurScene();
    if (!pCurScene)
        return false;

    const vector<CObject*>& vecTiles = pCurScene->GetGroupObject(GROUP_TYPE::TILE);

    Vec2 vMyPos = GetPos();
    Vec2 vCheckPos = Vec2(vMyPos.x + (m_fWallCheckDist * m_iDir), vMyPos.y);

    // 몬스터의 충돌체 크기
    Vec2 vMyScale = GetCollider() ? GetCollider()->GetScale() : Vec2(32.f, 32.f);

    for (CObject* pTile : vecTiles)
    {
        if (!pTile || pTile->IsDead())
            continue;

        CTile* pTileObj = dynamic_cast<CTile*>(pTile);
        if (!pTileObj || !pTileObj->IsSolid())
            continue;

        Vec2 vTilePos = pTile->GetPos();
        Vec2 vTileScale = pTile->GetCollider() ? pTile->GetCollider()->GetScale() : Vec2(64.f, 64.f);

        // AABB 충돌 검사
        if (abs(vCheckPos.x - vTilePos.x) < (vMyScale.x + vTileScale.x) / 2.f &&
            abs(vMyPos.y - vTilePos.y) < (vMyScale.y + vTileScale.y) / 2.f)
        {
            return true;
        }
    }

    // 화면 경계 체크
    if (vCheckPos.x < 32.f || vCheckPos.x > 928.f)
    {
        return true;
    }

    return false;
}

bool CMonster::CheckGroundAhead()
{
    // 실제 타일과의 바닥 검사 구현
    CScene* pCurScene = CSceneMgr::GetInst()->GetCurScene();
    if (!pCurScene)
        return false;

    const vector<CObject*>& vecTiles = pCurScene->GetGroupObject(GROUP_TYPE::TILE);

    Vec2 vMyPos = GetPos();
    Vec2 vCheckPos = Vec2(vMyPos.x + (m_fWallCheckDist * m_iDir), vMyPos.y + m_fGroundCheckDist);

    Vec2 vMyScale = GetCollider() ? GetCollider()->GetScale() : Vec2(32.f, 32.f);

    for (CObject* pTile : vecTiles)
    {
        if (!pTile || pTile->IsDead())
            continue;

        CTile* pTileObj = dynamic_cast<CTile*>(pTile);
        if (!pTileObj || !pTileObj->IsSolid())
            continue;

        Vec2 vTilePos = pTile->GetPos();
        Vec2 vTileScale = pTile->GetCollider() ? pTile->GetCollider()->GetScale() : Vec2(64.f, 64.f);

        // 발 아래쪽 위치에 타일이 있는지 검사
        if (abs(vCheckPos.x - vTilePos.x) < (vMyScale.x + vTileScale.x) / 2.f &&
            abs(vCheckPos.y - vTilePos.y) < (vMyScale.y + vTileScale.y) / 2.f)
        {
            return true;
        }
    }

    return false;
}