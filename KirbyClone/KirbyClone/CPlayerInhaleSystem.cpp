#include "pch.h"
#include "CPlayerInhaleSystem.h"
#include "CPlayerStateMachine.h"
#include "CPlayer.h"
#include "CRigidBody.h"
#include "CMonster.h"
#include "CSceneMgr.h"
#include "CScene.h"
#include "CTimeMgr.h"
#include "CEventMgr.h"
#include "CCamera.h"
#include "CBasicMonster.h"

CPlayerInhaleSystem::CPlayerInhaleSystem(CPlayer* _pOwner)
    : m_pOwner(_pOwner)
    , m_bInhaling(false)
    , m_fInhaleTime(0.f)
    , m_fInhaleRange(200.f)
    , m_vInhaleDir(Vec2(1.f, 0.f))
    , m_bHasMouthful(false)
    , m_pMouthfulTarget(nullptr)
    , m_eMouthfulType(OBJECT_TYPE::END)
{
}

CPlayerInhaleSystem::~CPlayerInhaleSystem()
{
}

void CPlayerInhaleSystem::Init()
{
    // 초기화 코드 (필요시 추가)
    m_vecInhaleTargets.clear();
    UpdateInhaleDirection();
}

void CPlayerInhaleSystem::Update()
{
    if (m_bInhaling)
    {
        UpdateInhale();
    }

    // 빨아들이기 방향 업데이트
    UpdateInhaleDirection();
}

void CPlayerInhaleSystem::StartInhale()
{
    m_bInhaling = true;
    m_fInhaleTime = 0.f;
    m_vecInhaleTargets.clear();
}

void CPlayerInhaleSystem::UpdateInhale()
{
    if (!m_bInhaling || !m_pOwner)
        return;

    m_fInhaleTime += CTimeMgr::GetInst()->GetfDT();

    // 빨아들이기 범위 내의 것들 찾기
    UpdateInhaleTargets();

    // 각 대상에 빨아들이기 힘 적용
    for (CObject* pTarget : m_vecInhaleTargets)
    {
        if (pTarget && !pTarget->IsDead())
        {
            ApplyInhaleForce(pTarget);
        }
    }
}

void CPlayerInhaleSystem::StopInhale()
{
    m_bInhaling = false;
    m_fInhaleTime = 0.f;
    
    // 빨아들이는 중인 몬스터들의 상태 해제
    for (CObject* pTarget : m_vecInhaleTargets)
    {
        CMonster* pMonster = dynamic_cast<CMonster*>(pTarget);
        if (pMonster)
        {
            CBasicMonster* pBasic = dynamic_cast<CBasicMonster*>(pMonster);
            if (pBasic)
            {
                pBasic->SetInhaled(false);
            }
        }
    }
    
    m_vecInhaleTargets.clear();
}

void CPlayerInhaleSystem::UpdateInhaleTargets()
{
    if (!m_pOwner)
        return;

    m_vecInhaleTargets.clear();

    CScene* pCurScene = CSceneMgr::GetInst()->GetCurScene();
    const vector<CObject*>& vecMonsters = pCurScene->GetGroupObject(GROUP_TYPE::MONSTER);

    Vec2 vPlayerPos = m_pOwner->GetPos();

    for (CObject* pObj : vecMonsters)
    {
        if (IsValidInhaleTarget(pObj))
        {
            Vec2 vMonsterPos = pObj->GetPos();
            Vec2 vDiff = vMonsterPos - vPlayerPos;
            float fDistance = vDiff.Length();

            // 빨아들이기 범위 안에 있는지 확인
            if (fDistance <= m_fInhaleRange && fDistance > 0.1f)
            {
                // 몬스터 방향으로의 단위 벡터
                Vec2 vToMonster = vDiff.GetNormalized();

                // 내 앞을 이용한 방향 확인 (빨아들이기 방향과 몬스터 방향의 각도)
                float fDot = vToMonster.Dot(m_vInhaleDir);

                // cos(60도) = 0.5이므로, 0.5보다 크면 대략 120도 범위 내
                if (fDot > 0.5f)
                {
                    m_vecInhaleTargets.push_back(pObj);

                    CMonster* pMonster = dynamic_cast<CMonster*>(pObj);
                    if (pMonster && pMonster->CanBeInhaled())
                    {
                        CBasicMonster* pBasic = dynamic_cast<CBasicMonster*>(pMonster);
                        if (pBasic && !pBasic->IsBeingInhaled())
                        {
                            pBasic->SetInhaled(true);
                            pBasic->OnInhaleStart();
                        }
                    }
                }
            }
        }
    }
}

void CPlayerInhaleSystem::SwallowTarget(CObject* _pTarget)
{
    if (!_pTarget || m_bHasMouthful || _pTarget->IsDead())
        return;

    // 이미 삭제 예정인 오브젝트라면 무시
    if (_pTarget->IsDead())
        return;

    // 물고 있는 상태로 설정
    m_bHasMouthful = true;
    m_pMouthfulTarget = _pTarget;
    m_eMouthfulType = _pTarget->GetType();

    // 오브젝트를 Dead 상태로 설정 (오브젝트 풀 시스템)
    _pTarget->SetDead();

    // StateMachine에 빨아들이기 성공 알림 (InhaleCount 설정)
    if (m_pOwner && m_pOwner->GetStateMachine())
    {
        // 빨아들인 개수 설정 (상태 전환 테이블이 이를 감지해서 INHALE_SUCCESS로 전환)
        m_pOwner->GetStateMachine()->SetInhaleCount(INHALE_COUNT::ONE);
        
        // 카피 능력 확인 및 설정
        CMonster* pMonster = dynamic_cast<CMonster*>(_pTarget);
        if (pMonster)
        {
            OBJECT_TYPE monsterType = pMonster->GetType();
            COPY_ABILITY ability = COPY_ABILITY::NONE;
            
            switch (monsterType)
            {
            case OBJECT_TYPE::MONSTER_HOT_HEAD:
                ability = COPY_ABILITY::FIRE;
                break;
            case OBJECT_TYPE::MONSTER_WADDLE_DOO:
                ability = COPY_ABILITY::BEAM;
                break;
            case OBJECT_TYPE::MONSTER_SPARKY:
                ability = COPY_ABILITY::SPARK;
                break;
            default:
                ability = COPY_ABILITY::NONE;
                break;
            }
            
            m_pOwner->GetStateMachine()->SetCopyAbility(ability);
        }
    }

    // 빨아들이기 중지 (시스템 정리)
    StopInhale();
}

void CPlayerInhaleSystem::SpitOut()
{
    if (!m_bHasMouthful || !m_pOwner)
        return;

    // 뱉기 애니메이션 재생
    //m_pOwner->ChangeState(PLAYER_STATE::EXHALE);

    // 투사체 생성 (나중에 구현 가능)
    // Vec2 vSpitPos = m_pOwner->GetPos() + m_vInhaleDir * 50.f;
    // CreateSpitProjectile(vSpitPos, m_vInhaleDir);

    // 물고 있는 상태 해제
    ReleaseMouthful();
}

void CPlayerInhaleSystem::ReleaseMouthful()
{
    m_bHasMouthful = false;
    m_pMouthfulTarget = nullptr;
    m_eMouthfulType = OBJECT_TYPE::END;
    
    // StateMachine의 빨아들이기 상태도 초기화
    if (m_pOwner && m_pOwner->GetStateMachine())
    {
        m_pOwner->GetStateMachine()->SetInhaleCount(INHALE_COUNT::NONE);
        // 뱉을 때만 카피 능력을 유지하고, 삼킬 때는 유지
    }
}

void CPlayerInhaleSystem::RenderInhaleEffect(HDC _dc)
{
    if (!m_bInhaling || !m_pOwner)
        return;

    Vec2 vPlayerPos = CCamera::GetInst()->GetRenderPos(m_pOwner->GetPos());

    // 빨아들이기 방향으로 이펙트 그리기
    Vec2 vEffectStart = vPlayerPos + m_vInhaleDir * 30.f; // 플레이어 앞쪽에서 시작
    Vec2 vEffectEnd = vPlayerPos + m_vInhaleDir * m_fInhaleRange;

    // 빨아들이기 범위 표시 (부채꼴 모양)
    HPEN hPen = CreatePen(PS_SOLID, 2, RGB(100, 150, 255));
    HPEN hOldPen = (HPEN)SelectObject(_dc, hPen);

    // 빨아들이기 중심선
    MoveToEx(_dc, (int)vEffectStart.x, (int)vEffectStart.y, nullptr);
    LineTo(_dc, (int)vEffectEnd.x, (int)vEffectEnd.y);

    // 빨아들이기 범위의 상하 경계선 (대략 60도 각도)
    float fAngle = 0.52f; // 약 30도 (라디안)

    // 상단 경계선
    Vec2 vUpperDir = Vec2(
        m_vInhaleDir.x * cos(-fAngle) - m_vInhaleDir.y * sin(-fAngle),
        m_vInhaleDir.x * sin(-fAngle) + m_vInhaleDir.y * cos(-fAngle)
    );
    Vec2 vUpperEnd = vPlayerPos + vUpperDir * m_fInhaleRange;
    MoveToEx(_dc, (int)vEffectStart.x, (int)vEffectStart.y, nullptr);
    LineTo(_dc, (int)vUpperEnd.x, (int)vUpperEnd.y);

    // 하단 경계선
    Vec2 vLowerDir = Vec2(
        m_vInhaleDir.x * cos(fAngle) - m_vInhaleDir.y * sin(fAngle),
        m_vInhaleDir.x * sin(fAngle) + m_vInhaleDir.y * cos(fAngle)
    );
    Vec2 vLowerEnd = vPlayerPos + vLowerDir * m_fInhaleRange;
    MoveToEx(_dc, (int)vEffectStart.x, (int)vEffectStart.y, nullptr);
    LineTo(_dc, (int)vLowerEnd.x, (int)vLowerEnd.y);

    // 원래 펜으로 복구
    SelectObject(_dc, hOldPen);
    DeleteObject(hPen);
}

void CPlayerInhaleSystem::UpdateInhaleDirection()
{
    if (!m_pOwner)
        return;

    // 플레이어가 바라보는 방향에 따라 빨아들이기 방향 설정
    m_vInhaleDir = m_pOwner->IsFacingRight() ? Vec2(1.f, 0.f) : Vec2(-1.f, 0.f);
}

bool CPlayerInhaleSystem::IsValidInhaleTarget(CObject* _pTarget)
{
    if (!_pTarget || _pTarget->IsDead())
        return false;

    // 몬스터만 빨아들이기 가능
    CMonster* pMonster = dynamic_cast<CMonster*>(_pTarget);
    if (!pMonster)
        return false;

    return pMonster->CanBeInhaled();
}

void CPlayerInhaleSystem::ApplyInhaleForce(CObject* _pTarget)
{
    if (!_pTarget || !m_pOwner || _pTarget->IsDead())
        return;

    Vec2 vPlayerPos = m_pOwner->GetPos();
    Vec2 vTargetPos = _pTarget->GetPos();
    Vec2 vDiff = vTargetPos - vPlayerPos;
    float fDistance = vDiff.Length();

    if (fDistance < 0.1f)
        return;

    // 몬스터를 플레이어 쪽으로 끌어당기는 힘 (속도 증가)
    Vec2 vToTarget = vDiff.GetNormalized();
    Vec2 vPullDirection = -vToTarget; // 플레이어 쪽으로
    Vec2 vForce = vPullDirection * 600.f; // 200.f -> 600.f로 증가

    // 몬스터의 RigidBody에 힘 적용 (RigidBody가 있다면)
    CRigidBody* pTargetRigidBody = _pTarget->GetRigidBody();
    if (pTargetRigidBody)
    {
        // 직접 속도 설정 (더 확실한 제어)
        pTargetRigidBody->SetVelocity(vForce);
    }

    // 매우 가까워지면 빨아들이기 성공
    if (fDistance < 30.f)
    {
        SwallowTarget(_pTarget);
    }
}

Vec2 CPlayerInhaleSystem::CalculateInhaleDirection()
{
    if (!m_pOwner)
        return Vec2(1.f, 0.f);

    return m_pOwner->IsFacingRight() ? Vec2(1.f, 0.f) : Vec2(-1.f, 0.f);
}