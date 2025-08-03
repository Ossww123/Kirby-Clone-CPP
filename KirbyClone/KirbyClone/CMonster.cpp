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

CMonster::CMonster()
    : CObject(OBJECT_TYPE::MONSTER_WADDLE_DEE)
    , m_pAnimator(nullptr)
    , m_pRigidBody(nullptr)
    , m_eCurState(MONSTER_STATE::IDLE)
    , m_ePrevState(MONSTER_STATE::END)
    , m_fSpeed(80.f)
    , m_iDir(1)
    , m_fStateTimer(0.f)
    , m_fIdleTime(1.f)
    , m_bHitWall(false)
    , m_fGroundCheckDist(32.f)
    , m_fWallCheckDist(32.f)
{
    // 충돌체 생성 - 56x56으로 설정
    CreateCollider();
    GetCollider()->SetScale(Vec2(56.f, 56.f));

    // 애니메이터 생성
    CreateAnimator();
    m_pAnimator = GetAnimator();

    // 리지드바디 생성
    CreateRigidBody();
    m_pRigidBody = GetRigidBody();

    // 리지드바디 설정 (몬스터는 가벼움)
    m_pRigidBody->SetMass(0.8f);
    m_pRigidBody->SetMaxVelocity(200.f);
    m_pRigidBody->SetFriction(8.f);
    m_pRigidBody->SetUseGravity(true);

    // 애니메이션 생성
    CreateAnimation();

    // 초기 상태 설정
    ChangeState(MONSTER_STATE::IDLE);
}

CMonster::~CMonster()
{
    m_pAnimator = nullptr;
    m_pRigidBody = nullptr;
}

void CMonster::CreateAnimation()
{
    // 웨이들디 스프라이트 로드
    CTexture* pTex = CResMgr::GetInst()->LoadTexture(L"WaddleDeeSprite", L"texture\\enemy\\waddle_dee.bmp");

    // IDLE 애니메이션 (첫 번째 프레임만 사용)
    m_pAnimator->CreateAnimation(L"IDLE", pTex, Vec2(24, 24), Vec2(80, 76), Vec2(96, 0), 0.5f, 1, true);

    // WALK 애니메이션 (걷기 애니메이션)
    m_pAnimator->CreateAnimation(L"WALK", pTex, Vec2(24, 24), Vec2(80, 76), Vec2(96, 0), 0.15f, 8, true);

    // 초기 애니메이션 재생
    m_pAnimator->Play(L"IDLE", true);
}

void CMonster::Update()
{
    UpdateState();
    UpdateMove();

    // 리지드바디 업데이트
    if (nullptr != m_pRigidBody)
        m_pRigidBody->Update();

    // 애니메이터 업데이트
    if (nullptr != m_pAnimator)
        m_pAnimator->Update();

    // 상태 타이머 업데이트
    m_fStateTimer += CTimeMgr::GetInst()->GetfDT();
}

void CMonster::UpdateState()
{
    MONSTER_STATE eNewState = m_eCurState;

    switch (m_eCurState)
    {
    case MONSTER_STATE::IDLE:
        UpdateIdle();
        break;

    case MONSTER_STATE::WALK:
        UpdateWalk();
        break;

    case MONSTER_STATE::TURN:
        UpdateTurn();
        break;
    }
}

void CMonster::UpdateIdle()
{
    // 일정 시간 정지 후 걷기 시작
    if (m_fStateTimer >= m_fIdleTime)
    {
        ChangeState(MONSTER_STATE::WALK);
    }
}

void CMonster::UpdateWalk()
{
    // 전방에 벽이 있거나 바닥이 없으면 방향 전환
    if (CheckWallAhead() || !CheckGroundAhead())
    {
        ChangeState(MONSTER_STATE::TURN);
        return;
    }

    // 계속 걷기
    m_pRigidBody->SetVelocityX(m_fSpeed * m_iDir);
}

void CMonster::UpdateTurn()
{
    // 방향 전환 시간 (0.2초)
    if (m_fStateTimer >= 0.2f)
    {
        TurnAround();
        ChangeState(MONSTER_STATE::WALK);
    }
    else
    {
        // 방향 전환 중에는 멈춤
        m_pRigidBody->SetVelocityX(0.f);
    }
}

void CMonster::UpdateMove()
{
    // 바닥에 있을 때만 이동 로직 적용
    if (m_pRigidBody && m_pRigidBody->IsGround())
    {
        // 상태별 이동은 UpdateState에서 처리됨
    }
}

void CMonster::ChangeState(MONSTER_STATE _eState)
{
    m_ePrevState = m_eCurState;
    m_eCurState = _eState;
    m_fStateTimer = 0.f;

    // 상태에 맞는 애니메이션 재생
    switch (m_eCurState)
    {
    case MONSTER_STATE::IDLE:
        m_pAnimator->Play(L"IDLE", true);
        break;
    case MONSTER_STATE::WALK:
        m_pAnimator->Play(L"WALK", true);
        break;
    case MONSTER_STATE::TURN:
        m_pAnimator->Play(L"IDLE", true);
        break;
    }
}

bool CMonster::CheckWallAhead()
{
    // 현재 위치에서 이동 방향으로 일정 거리만큼 떨어진 지점에서 타일 체크
    Vec2 vPos = GetPos();
    Vec2 vCheckPos = vPos;
    vCheckPos.x += m_fWallCheckDist * m_iDir;

    // 현재 씬의 타일들과 충돌 체크
    CScene* pCurScene = CSceneMgr::GetInst()->GetCurScene();
    const vector<CObject*>& vecTiles = pCurScene->GetGroupObject(GROUP_TYPE::TILE);

    for (size_t i = 0; i < vecTiles.size(); ++i)
    {
        CTile* pTile = dynamic_cast<CTile*>(vecTiles[i]);
        if (pTile && pTile->IsSolid())
        {
            Vec2 vTilePos = pTile->GetPos();
            Vec2 vTileScale = pTile->GetScale();

            // 간단한 AABB 충돌 체크
            if (vCheckPos.x >= vTilePos.x - vTileScale.x / 2.f &&
                vCheckPos.x <= vTilePos.x + vTileScale.x / 2.f &&
                vCheckPos.y >= vTilePos.y - vTileScale.y / 2.f &&
                vCheckPos.y <= vTilePos.y + vTileScale.y / 2.f)
            {
                return true; // 벽 발견
            }
        }
    }

    return false; // 벽 없음
}

bool CMonster::CheckGroundAhead()
{
    // 전방 바닥 체크 (절벽 방지)
    Vec2 vPos = GetPos();
    Vec2 vCheckPos = vPos;
    vCheckPos.x += m_fGroundCheckDist * m_iDir;
    vCheckPos.y += 32.f; // 발 아래쪽 체크

    // 현재 씬의 타일들과 충돌 체크
    CScene* pCurScene = CSceneMgr::GetInst()->GetCurScene();
    const vector<CObject*>& vecTiles = pCurScene->GetGroupObject(GROUP_TYPE::TILE);

    for (size_t i = 0; i < vecTiles.size(); ++i)
    {
        CTile* pTile = dynamic_cast<CTile*>(vecTiles[i]);
        if (pTile && pTile->IsSolid())
        {
            Vec2 vTilePos = pTile->GetPos();
            Vec2 vTileScale = pTile->GetScale();

            // 바닥 타일이 있는지 체크
            if (vCheckPos.x >= vTilePos.x - vTileScale.x / 2.f &&
                vCheckPos.x <= vTilePos.x + vTileScale.x / 2.f &&
                vCheckPos.y >= vTilePos.y - vTileScale.y / 2.f &&
                vCheckPos.y <= vTilePos.y + vTileScale.y / 2.f)
            {
                return true; // 바닥 발견
            }
        }
    }

    return false; // 바닥 없음 (절벽)
}

void CMonster::TurnAround()
{
    m_iDir *= -1; // 방향 반전
}

void CMonster::Render(HDC _dc)
{
    // 애니메이션 렌더링
    if (nullptr != m_pAnimator)
    {
        m_pAnimator->Render(_dc);
    }

    // 충돌체가 있으면 충돌체도 렌더링 (디버그용)
    if (nullptr != GetCollider())
        GetCollider()->Render(_dc);
}

void CMonster::OnCollisionEnter(CCollider* _pOther)
{
    CObject* pOtherObj = _pOther->GetOwner();

    // 타일과의 충돌 처리 (플레이어와 동일한 로직)
    CTile* pTile = dynamic_cast<CTile*>(pOtherObj);
    if (pTile && pTile->IsSolid())
    {
        Vec2 vMonsterPos = GetPos();
        Vec2 vTilePos = pTile->GetPos();
        Vec2 vVelocity = m_pRigidBody->GetVelocity();

        // 충돌체 크기
        Vec2 vMonsterColliderScale = GetCollider()->GetScale();
        Vec2 vTileColliderScale = pTile->GetCollider()->GetScale();

        // 몬스터가 타일 위에서 아래로 떨어지고 있을 때만 착지
        if (vVelocity.y >= 0.f && vMonsterPos.y < vTilePos.y)
        {
            float tileTop = vTilePos.y - vTileColliderScale.y / 2.f;
            float monsterHalfHeight = vMonsterColliderScale.y / 2.f;

            float newY = tileTop - monsterHalfHeight;
            vMonsterPos.y = newY;
            SetPos(vMonsterPos);

            m_pRigidBody->SetVelocityY(0.f);
            m_pRigidBody->SetGround(true);
        }
    }
}

void CMonster::OnCollision(CCollider* _pOther)
{
    // 타일과의 지속적인 충돌 처리
    CObject* pOtherObj = _pOther->GetOwner();

    CTile* pTile = dynamic_cast<CTile*>(pOtherObj);
    if (pTile && pTile->IsSolid())
    {
        // 바닥 상태 유지 로직 (플레이어와 동일)
        Vec2 vMonsterPos = GetPos();
        Vec2 vTilePos = pTile->GetPos();

        Vec2 vMonsterColliderScale = GetCollider()->GetScale();
        Vec2 vTileColliderScale = pTile->GetCollider()->GetScale();

        float tileTop = vTilePos.y - vTileColliderScale.y / 2.f;
        float monsterBottom = vMonsterPos.y + vMonsterColliderScale.y / 2.f;

        if (abs(monsterBottom - tileTop) < 8.f && vMonsterPos.y < vTilePos.y)
        {
            m_pRigidBody->SetGround(true);

            Vec2 vVelocity = m_pRigidBody->GetVelocity();
            if (vVelocity.y > 0.f)
            {
                m_pRigidBody->SetVelocityY(0.f);
            }
        }
    }
}

void CMonster::OnCollisionExit(CCollider* _pOther)
{
    CObject* pOtherObj = _pOther->GetOwner();

    CTile* pTile = dynamic_cast<CTile*>(pOtherObj);
    if (pTile && pTile->IsSolid())
    {
        // 플레이어와 동일한 로직
        Vec2 vVelocity = m_pRigidBody->GetVelocity();

        if (vVelocity.y < -30.f || abs(vVelocity.x) > 50.f)
        {
            m_pRigidBody->SetGround(false);
        }
    }
}