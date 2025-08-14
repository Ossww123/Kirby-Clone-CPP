#include "pch.h"
#include "CPlayerInhaleSystem.h"
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

    // 상태 머신을 통한 상태 변경
    if (m_pOwner)
    {
        m_pOwner->ChangeState(PLAYER_STATE::INHALE_READY);
    }
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

    // 빨아들이기 단계별 처리 (시간에 따른 상태 변경)
    if (m_fInhaleTime > 0.5f && m_fInhaleTime <= 1.0f)
    {
        if (m_pOwner->GetCurrentState() == PLAYER_STATE::INHALE_READY)
        {
            m_pOwner->ChangeState(PLAYER_STATE::INHALE_1);
        }
    }
    else if (m_fInhaleTime > 1.0f && m_fInhaleTime <= 2.0f)
    {
        if (m_pOwner->GetCurrentState() == PLAYER_STATE::INHALE_1)
        {
            m_pOwner->ChangeState(PLAYER_STATE::INHALE_2);
        }
    }
    else if (m_fInhaleTime > 2.0f)
    {
        if (m_pOwner->GetCurrentState() == PLAYER_STATE::INHALE_2)
        {
            m_pOwner->ChangeState(PLAYER_STATE::INHALE_HOLD);
        }
    }
}

void CPlayerInhaleSystem::StopInhale()
{
    m_bInhaling = false;
    m_fInhaleTime = 0.f;
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
    if (!_pTarget || m_bHasMouthful)
        return;

    // 삼키기 상태로 전환
    if (m_pOwner)
    {
        m_pOwner->ChangeState(PLAYER_STATE::SWALLOW);
    }

    // 물고 있는 상태로 설정
    m_bHasMouthful = true;
    m_pMouthfulTarget = _pTarget;
    m_eMouthfulType = _pTarget->GetType();

    // 대상 오브젝트 제거 이벤트 발생
    tEvent event = {};
    event.eType = EVENT_TYPE::DELETE_OBJECT;
    event.lParam = (DWORD_PTR)_pTarget;
    CEventMgr::GetInst()->AddEvent(event);

    // 빨아들이기 중지
    StopInhale();
}

void CPlayerInhaleSystem::SpitOut()
{
    if (!m_bHasMouthful || !m_pOwner)
        return;

    // 뱉기 애니메이션 재생
    m_pOwner->ChangeState(PLAYER_STATE::EXHALE);

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
    if (!_pTarget || !m_pOwner)
        return;

    Vec2 vPlayerPos = m_pOwner->GetPos();
    Vec2 vTargetPos = _pTarget->GetPos();
    Vec2 vDiff = vTargetPos - vPlayerPos;
    float fDistance = vDiff.Length();

    if (fDistance < 0.1f)
        return;

    // 몬스터를 플레이어 쪽으로 끌어당기는 힘
    Vec2 vToTarget = vDiff.GetNormalized();
    Vec2 vPullDirection = -vToTarget; // 플레이어 쪽으로
    Vec2 vForce = vPullDirection * 200.f;

    // 몬스터의 RigidBody에 힘 적용 (RigidBody가 있다면)
    CRigidBody* pTargetRigidBody = _pTarget->GetRigidBody();
    if (pTargetRigidBody)
    {
        // 현재 속도에 끌어당기는 힘 추가
        Vec2 vCurrentVel = pTargetRigidBody->GetVelocity();
        Vec2 vNewVel = vCurrentVel + vForce * CTimeMgr::GetInst()->GetfDT();
        pTargetRigidBody->SetVelocity(vNewVel);
    }

    // 매우 가까워지면 삼키기
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