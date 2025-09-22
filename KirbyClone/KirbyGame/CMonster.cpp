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
#include "CTile.h"
#include "CAnimationDataMgr.h"
#include "CSoundMgr.h"

CMonster::CMonster()
    : CObject(OBJECT_TYPE::MONSTER_WADDLE_DEE)  // 기본값, 자식에서 변경
    , m_eCurState(MONSTER_STATE::IDLE)
    , m_ePrevState(MONSTER_STATE::END)
    , m_fStateTimer(0.f)
    , m_fSpeed(DEFAULT_SPEED)
    , m_iDir(1)
    , m_fIdleTime(DEFAULT_IDLE_TIME)
    , m_pEnemyTex(nullptr)
    , m_bEditorMode(false)
    , m_bWallCollision(false)
    , m_bGroundCollision(false)
    , m_bPrevWallCollision(false)
    , m_bPrevGroundCollision(false)
{
    // 기본 컴포넌트 생성
    CreateCollider();
    GetCollider()->SetScale(Vec2(56.f, 56.f));

    CreateAnimator();
    CreateRigidBody();

    // 리지드바디 기본 설정
    GetRigidBody()->SetMass(0.8f);
    GetRigidBody()->SetMaxVelocity(200.f);
    GetRigidBody()->SetFriction(0.f);  // 몬스터는 정속 이동이므로 마찰력 불필요
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
    // 에디터 모드에서는 최소한의 업데이트만 수행
    if (m_bEditorMode)
    {   
        // 애니메이터만 업데이트 (시각적 표시용)
        if (nullptr != GetAnimator())
            GetAnimator()->Update();
        return;
    }

    // 스테이지 경계 체크 (경계를 벗어나면 Dead 처리)
    CheckStageBounds();

    // 충돌 상태 업데이트 (이전 프레임 상태 저장)
    UpdateCollisionState();

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
        // 몬스터는 기본적으로 왼쪽을 보므로, 오른쪽을 볼 때 플립
        bool shouldFlip = IsFacingRight();
        pAnimator->SetFlipX(shouldFlip);
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
    // 빨아들려지는 중이거나 데미지 상태에서는 타일 충돌 무시
    if (m_eCurState == MONSTER_STATE::BEING_INHALED || m_eCurState == MONSTER_STATE::DAMAGE)
    {
        return;
    }

    if (!_pTile || !GetCollider() || !_pTile->GetCollider())
        return;

    CTile* pTile = dynamic_cast<CTile*>(_pTile);
    if (!pTile || !pTile->IsSolid())
        return;

    Vec2 vMyPos = GetPos();
    Vec2 vTilePos = _pTile->GetPos();
    Vec2 vMyScale = GetCollider()->GetScale();
    Vec2 vTileScale = _pTile->GetCollider()->GetScale();

    // 충돌 깊이 계산
    float fOverlapX = (vMyScale.x + vTileScale.x) / 2.f - abs(vMyPos.x - vTilePos.x);
    float fOverlapY = (vMyScale.y + vTileScale.y) / 2.f - abs(vMyPos.y - vTilePos.y);

    if (fOverlapX > 0.f && fOverlapY > 0.f)
    {
        // 더 작은 겹침을 우선으로 분리
        if (fOverlapX < fOverlapY)
        {
            // 수평 분리
            if (vMyPos.x < vTilePos.x)
            {
                // 몬스터가 타일 왼쪽에 있음 - 왼쪽으로 밀기
                SetPos(Vec2(vTilePos.x - (vMyScale.x + vTileScale.x) / 2.f, vMyPos.y));
                if (GetRigidBody())
                {
                    GetRigidBody()->SetVelocityX(0.f);
                    // 벽에 부딪혔으므로 방향 전환
                    TurnAround();
                }
            }
            else
            {
                // 몬스터가 타일 오른쪽에 있음 - 오른쪽으로 밀기  
                SetPos(Vec2(vTilePos.x + (vMyScale.x + vTileScale.x) / 2.f, vMyPos.y));
                if (GetRigidBody())
                {
                    GetRigidBody()->SetVelocityX(0.f);
                    // 벽에 부딪혔으므로 방향 전환
                    TurnAround();
                }
            }
        }
        else
        {
            // 수직 분리
            if (vMyPos.y < vTilePos.y)
            {
                // 몬스터가 타일 위에 있음 - 위로 밀기 (착지)
                SetPos(Vec2(vMyPos.x, vTilePos.y - (vMyScale.y + vTileScale.y) / 2.f));
                if (GetRigidBody())
                {
                    GetRigidBody()->SetVelocityY(0.f);
                    GetRigidBody()->SetGround(true);
                }
            }
            else
            {
                // 몬스터가 타일 아래에 있음 - 아래로 밀기 (천장 충돌)
                SetPos(Vec2(vMyPos.x, vTilePos.y + (vMyScale.y + vTileScale.y) / 2.f));
                if (GetRigidBody())
                {
                    GetRigidBody()->SetVelocityY(0.f);
                }
            }
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
            bool bLoop = (_eState != MONSTER_STATE::DAMAGE); // DAMAGE만 반복 안함, BEING_INHALED는 반복
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
            case MONSTER_STATE::BEING_INHALED:
                pAnimator->Play(L"DAMAGE", true);  // 빨아들어지는 동안 계속 재생
                break;
            // case MONSTER_STATE::EDITOR_IDLE: // 에디터 전용 상태 - 현재 미사용
            //     pAnimator->Play(L"IDLE", true);
            //     break;
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

void CMonster::SetEditorMode(bool _bEditorMode)
{
    m_bEditorMode = _bEditorMode;

    // 게임에서는 항상 false로 설정되므로 게임 모드로 초기화
    if (!_bEditorMode)
    {
        // 리지드바디 활성화
        if (GetRigidBody())
        {
            GetRigidBody()->SetUseGravity(true);
            GetRigidBody()->SetVelocity(Vec2(0.f, 0.f));
        }

        // 게임 상태로 초기화
        ChangeState(MONSTER_STATE::IDLE);
        m_fStateTimer = 0.f;
    }
}

void CMonster::MoveHorizontal(float speed)
{
    if (nullptr != GetRigidBody())
    {
        float targetVelocityX = speed * m_iDir;
        GetRigidBody()->SetVelocityX(targetVelocityX);
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
    case MONSTER_STATE::BEING_INHALED:
        UpdateBeingInhaled();
        break;
    case MONSTER_STATE::ATTACK_READY:
        UpdateAttackReady();
        break;
    case MONSTER_STATE::ATTACK:
        UpdateAttack();
        break;
    // case MONSTER_STATE::EDITOR_IDLE: // 에디터 전용 상태 - 현재 미사용
    //     UpdateEditorIdle();
    //     break;
    }
}

void CMonster::UpdateMove()
{
    // 자식 클래스에서 Move() 호출로 실제 이동 처리
    Move();
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
    // 방향 전환 중에는 멈춤
    if (nullptr != GetRigidBody())
    {
        GetRigidBody()->SetVelocityX(0.f);
    }
    
    // 방향 전환 시간
    if (m_fStateTimer >= TURN_DURATION)
    {
        // 실제 방향 전환
        TurnAround();
        ChangeState(MONSTER_STATE::WALK);  // 기본적으로 걷기로 복귀
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

void CMonster::UpdateBeingInhaled()
{
    // 빨아들려지는 중에는 일반 AI 정지
    // 물리 이동은 CPlayerInhaleSystem에서 처리됨
    // 타일 충돌과 데미지는 자식 클래스에서 처리
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


void CMonster::UpdateCollisionState()
{
    // 이전 프레임 충돌 상태 저장
    m_bPrevWallCollision = m_bWallCollision;
    m_bPrevGroundCollision = m_bGroundCollision;

    // 현재 프레임 충돌 상태 초기화 (OnCollision에서 다시 설정됨)
    m_bWallCollision = false;
    m_bGroundCollision = false;
}



void CMonster::HandleTileCollisionEnter(CTile* _pTile)
{
    Vec2 vMyPos = GetPos();
    Vec2 vTilePos = _pTile->GetPos();
    Vec2 vMyScale = GetCollider() ? GetCollider()->GetScale() : Vec2(56.f, 56.f);
    Vec2 vTileScale = _pTile->GetCollider() ? _pTile->GetCollider()->GetScale() : Vec2(64.f, 64.f);

    // 타일과의 상대적 위치를 계산하여 벽 충돌인지 바닥 충돌인지 판단
    float deltaX = abs(vMyPos.x - vTilePos.x);
    float deltaY = abs(vMyPos.y - vTilePos.y);

    float overlapX = (vMyScale.x + vTileScale.x) / 2.f - deltaX;
    float overlapY = (vMyScale.y + vTileScale.y) / 2.f - deltaY;

    // 겹침이 더 작은 축을 기준으로 충돌 방향 결정
    if (overlapX < overlapY)
    {
        // 좌우 충돌 (벽) - 방향 전환 처리
        TurnAround();
    }
}

void CMonster::UpdateTileCollisionState(CTile* _pTile)
{
    Vec2 vMyPos = GetPos();
    Vec2 vTilePos = _pTile->GetPos();
    Vec2 vMyScale = GetCollider() ? GetCollider()->GetScale() : Vec2(56.f, 56.f);
    Vec2 vTileScale = _pTile->GetCollider() ? _pTile->GetCollider()->GetScale() : Vec2(64.f, 64.f);

    // 타일과의 상대적 위치를 계산하여 벽 충돌인지 바닥 충돌인지 판단
    float deltaX = abs(vMyPos.x - vTilePos.x);
    float deltaY = abs(vMyPos.y - vTilePos.y);

    float overlapX = (vMyScale.x + vTileScale.x) / 2.f - deltaX;
    float overlapY = (vMyScale.y + vTileScale.y) / 2.f - deltaY;

    // 겹침이 더 작은 축을 기준으로 충돌 방향 결정
    if (overlapX < overlapY)
    {
        // 좌우 충돌 (벽)
        m_bWallCollision = true;
    }
    else
    {
        // 상하 충돌 (바닥/천장)
        if (vMyPos.y > vTilePos.y)
        {
            // 바닥 충돌
            m_bGroundCollision = true;
        }
    }
}

void CMonster::CheckStageBounds()
{
    // 카메라의 스테이지 경계 정보 가져오기
    Vec2 vStageBoundsMin, vStageBoundsMax;
    CCamera::GetInst()->GetStageBounds(vStageBoundsMin, vStageBoundsMax);

    Vec2 vMyPos = GetPos();

    // 스테이지 경계를 벗어났는지 확인
    if (vMyPos.x < vStageBoundsMin.x - 100.f || vMyPos.x > vStageBoundsMax.x + 100.f ||
        vMyPos.y < vStageBoundsMin.y - 100.f || vMyPos.y > vStageBoundsMax.y + 100.f)
    {
        // 경계를 벗어나면 Dead 처리
        SetDead();
    }
}