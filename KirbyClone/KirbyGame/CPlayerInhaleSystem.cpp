#include "gamePCH.h"
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
#include "CAbilityStar.h"
#include "CAirParticle.h"
#include "CSoundMgr.h"

CPlayerInhaleSystem::CPlayerInhaleSystem(CPlayer* _pOwner)
    : m_pOwner(_pOwner)
    , m_bInhaling(false)
    , m_fInhaleTime(0.f)
    , m_fInhaleRange(200.f)
    , m_vInhaleDir(Vec2(1.f, 0.f))
    , m_bHasMouthful(false)
    , m_pMouthfulTarget(nullptr)
    , m_eMouthfulType(OBJECT_TYPE::END)
    , m_fParticleSpawnTimer(0.f)
    , m_fParticleSpawnInterval(0.05f)
    , m_bInhaleSoundPlaying(false)
{
}

CPlayerInhaleSystem::~CPlayerInhaleSystem()
{
    ClearParticles();
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
    
    // 파티클 시스템 업데이트
    UpdateParticles();
}

void CPlayerInhaleSystem::StartInhale()
{
    m_bInhaling = true;
    m_fInhaleTime = 0.f;
    m_vecInhaleTargets.clear();
    
    // 흡입 사운드 재생
    if (!m_bInhaleSoundPlaying)
    {
        CSoundMgr::GetInst()->PlaySFX(L"kirby_inhale");
        m_bInhaleSoundPlaying = true;
    }
    
    // 모든 몬스터의 빨아들이기 상태 초기화 (두 번째 빨아들이기를 위해)
    CScene* pCurScene = CSceneMgr::GetInst()->GetCurScene();
    if (pCurScene)
    {
        const vector<CObject*>& vecMonsters = pCurScene->GetGroupObject(GROUP_TYPE::MONSTER);
        for (CObject* pObj : vecMonsters)
        {
            CBasicMonster* pBasic = dynamic_cast<CBasicMonster*>(pObj);
            if (pBasic && !pObj->IsDead())
            {
                pBasic->SetInhaled(false);
            }
        }
    }
}

void CPlayerInhaleSystem::UpdateInhale()
{
    if (!m_bInhaling || !m_pOwner)
        return;

    m_fInhaleTime += CTimeMgr::GetInst()->GetfDT();

    // === 이벤트 기반 빨아들이기 ===
    // 빨아들이기 시작 시 한 번만 대상 찾아서 BEING_INHALED 상태로 설정
    if (m_vecInhaleTargets.empty())
    {
        UpdateInhaleTargets(); // 범위 내 몬스터를 찾아서 BEING_INHALED 상태로 설정
        return; // 이번 프레임에서는 여기서 종료
    }

    // 이미 등록된 대상들에만 빨아들이기 힘 적용
    // SwallowTarget 호출 시 벡터가 clear될 수 있으므로 안전하게 처리
    for (int i = (int)m_vecInhaleTargets.size() - 1; i >= 0; --i)
    {
        if (i >= (int)m_vecInhaleTargets.size()) // 벡터 크기가 변경되었으면 스킵
            continue;
            
        InhaleTargetInfo& targetInfo = m_vecInhaleTargets[i];
        
        if (!targetInfo.pTarget || targetInfo.pTarget->IsDead())
        {
            m_vecInhaleTargets.erase(m_vecInhaleTargets.begin() + i);
            continue;
        }
        
        ApplyInhaleForce(targetInfo.pTarget);
        
        // ApplyInhaleForce에서 SwallowTarget이 호출되어 벡터가 clear되었을 수 있음
        if (m_vecInhaleTargets.empty())
            break;
    }
}

void CPlayerInhaleSystem::StopInhale()
{
    m_bInhaling = false;
    m_fInhaleTime = 0.f;
    
    // 흡입 사운드 정지
    if (m_bInhaleSoundPlaying)
    {
        CSoundMgr::GetInst()->StopSFX(L"kirby_inhale");
        m_bInhaleSoundPlaying = false;
    }
    
    // 빨아들이는 중인 대상들의 상태 해제
    for (auto& targetInfo : m_vecInhaleTargets)
    {
        CMonster* pMonster = dynamic_cast<CMonster*>(targetInfo.pTarget);
        if (pMonster)
        {
            CBasicMonster* pBasic = dynamic_cast<CBasicMonster*>(pMonster);
            if (pBasic)
            {
                pBasic->SetInhaled(false);
            }
        }
        
        // CAbilityStar 상태 해제
        CAbilityStar* pAbilityStar = dynamic_cast<CAbilityStar*>(targetInfo.pTarget);
        if (pAbilityStar)
        {
            pAbilityStar->SetBeingInhaled(false);
        }
    }
    
    m_vecInhaleTargets.clear();
    
    // 파티클들도 즉시 제거
    ClearParticles();
}

void CPlayerInhaleSystem::UpdateInhaleTargets()
{
    if (!m_pOwner)
        return;

    m_vecInhaleTargets.clear();

    CScene* pCurScene = CSceneMgr::GetInst()->GetCurScene();
    const vector<CObject*>& vecMonsters = pCurScene->GetGroupObject(GROUP_TYPE::MONSTER);
    const vector<CObject*>& vecItems = pCurScene->GetGroupObject(GROUP_TYPE::ITEM);

    Vec2 vPlayerPos = m_pOwner->GetPos();

    // 몬스터 검사
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
                    // 이미 대상 목록에 있는지 확인
                    bool bAlreadyExists = false;
                    for (auto& targetInfo : m_vecInhaleTargets)
                    {
                        if (targetInfo.pTarget == pObj)
                        {
                            bAlreadyExists = true;
                            break;
                        }
                    }

                    // 새로운 대상이면 추가
                    if (!bAlreadyExists)
                    {
                        m_vecInhaleTargets.emplace_back(pObj, vMonsterPos);

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
    
    // 능력별 아이템 검사
    for (CObject* pObj : vecItems)
    {
        CAbilityStar* pAbilityStar = dynamic_cast<CAbilityStar*>(pObj);
        if (pAbilityStar && !pObj->IsDead())
        {
            Vec2 vItemPos = pObj->GetPos();
            Vec2 vDiff = vItemPos - vPlayerPos;
            float fDistance = vDiff.Length();

            // 빨아들이기 범위 안에 있는지 확인
            if (fDistance <= m_fInhaleRange && fDistance > 0.1f)
            {
                // 능력별 방향으로의 단위 벡터
                Vec2 vToItem = vDiff.GetNormalized();

                // 내 앞을 이용한 방향 확인 (빨아들이기 방향과 능력별 방향의 각도)
                float fDot = vToItem.Dot(m_vInhaleDir);

                // cos(60도) = 0.5이므로, 0.5보다 크면 대략 120도 범위 내
                if (fDot > 0.5f)
                {
                    // 이미 대상 목록에 있는지 확인
                    bool bAlreadyExists = false;
                    for (auto& targetInfo : m_vecInhaleTargets)
                    {
                        if (targetInfo.pTarget == pObj)
                        {
                            bAlreadyExists = true;
                            break;
                        }
                    }

                    // 새로운 대상이면 추가
                    if (!bAlreadyExists)
                    {
                        m_vecInhaleTargets.emplace_back(pObj, vItemPos);
                        
                        // CAbilityStar에게 빨아들이기 상태 설정
                        pAbilityStar->SetBeingInhaled(true);
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
        
        // 카피 능력 확인 및 설정 (몬스터 또는 능력별)
        COPY_ABILITY ability = COPY_ABILITY::NONE;
        
        // 몬스터인 경우
        CMonster* pMonster = dynamic_cast<CMonster*>(_pTarget);
        if (pMonster)
        {
            OBJECT_TYPE monsterType = pMonster->GetType();
            
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
            case OBJECT_TYPE::MONSTER_BRONTO_BURT:
                ability = COPY_ABILITY::NONE;  // 브론토버트는 기본 능력 없음
                break;
            default:
                ability = COPY_ABILITY::NONE;
                break;
            }
        }
        
        // OBJECT_TYPE부터 먼저 확인
        OBJECT_TYPE targetType = _pTarget->GetType();
        
        // 능력별인 경우 - 먼저 타입으로 확인
        if (targetType == OBJECT_TYPE::ITEM_ABILITY_STAR)
        {
            // dynamic_cast 시도
            CAbilityStar* pAbilityStar = dynamic_cast<CAbilityStar*>(_pTarget);
            if (pAbilityStar)
            {
                ability = pAbilityStar->GetAbility();
            }
            else
            {
                // 타입으로 직접 능력 가져오기 시도
                CAbilityStar* pDirectCast = static_cast<CAbilityStar*>(_pTarget);
                if (pDirectCast)
                {
                    ability = pDirectCast->GetAbility();
                }
            }
        }
        
        // 능력 정보는 저장하되, 실제 적용은 SWALLOW 완료 후로 연기
        if (ability != COPY_ABILITY::NONE)
        {
            // StateMachine에 능력 정보 저장 (실제 적용은 나중)
            m_pOwner->GetStateMachine()->SetPendingCopyAbility(ability);
            
        }
    }

    // 해당 몬스터를 대상 목록에서 제거 (다른 몬스터들은 계속 빨아들이기)
    for (auto it = m_vecInhaleTargets.begin(); it != m_vecInhaleTargets.end(); ++it)
    {
        if (it->pTarget == _pTarget)
        {
            m_vecInhaleTargets.erase(it);
            break;
        }
    }
    
    // 모든 몬스터가 처리되었으면 빨아들이기 시스템 정지
    if (m_vecInhaleTargets.empty())
    {
        StopInhale();
    }
}

void CPlayerInhaleSystem::SpitOut()
{
    if (!m_bHasMouthful || !m_pOwner)
        return;

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
        
        // EXHALE 시 노멀 애니메이션으로 복구 (능력은 유지하되 애니메이션만 변경)
        COPY_ABILITY currentAbility = m_pOwner->GetStateMachine()->GetCopyAbility();
        if (currentAbility != COPY_ABILITY::NONE)
        {
            // 노멀 애니메이션으로 복구
            m_pOwner->LoadCopyAbilityAnimations(COPY_ABILITY::NONE);
        }
    }
    
    // 빨아들이기 시스템 완전 초기화 (EXHALE 후 새로운 빨아들이기 가능하도록)
    m_bInhaling = false;
    m_fInhaleTime = 0.f;
    m_vecInhaleTargets.clear();
}

void CPlayerInhaleSystem::RenderInhaleEffect(HDC _dc)
{
    // 기존 선 렌더링 제거됨 - 이제 파티클 시스템으로 시각화
    return;
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

    // 몬스터 빨아들이기 가능 여부 확인
    CMonster* pMonster = dynamic_cast<CMonster*>(_pTarget);
    if (pMonster)
    {
        return pMonster->CanBeInhaled();
    }

    // 능력별 빨아들이기 가능 여부 확인
    CAbilityStar* pAbilityStar = dynamic_cast<CAbilityStar*>(_pTarget);
    if (pAbilityStar)
    {
        return true; // 모든 능력별은 빨아들이기 가능
    }

    return false;
}

void CPlayerInhaleSystem::ApplyInhaleForce(CObject* _pTarget)
{
    if (!_pTarget || !m_pOwner || _pTarget->IsDead())
        return;

    // InhaleTargetInfo에서 해당 대상을 찾기
    InhaleTargetInfo* pTargetInfo = nullptr;
    for (auto& targetInfo : m_vecInhaleTargets)
    {
        if (targetInfo.pTarget == _pTarget)
        {
            pTargetInfo = &targetInfo;
            break;
        }
    }

    if (!pTargetInfo)
        return;

    // 타이머 업데이트
    pTargetInfo->fInhaleTimer += CTimeMgr::GetInst()->GetfDT();

    Vec2 vPlayerPos = m_pOwner->GetPos();
    Vec2 vTargetPos = _pTarget->GetPos();
    Vec2 vDiff = vPlayerPos - vTargetPos; // 커비 위치 - 대상 위치
    float fDistance = vDiff.Length();

    // 플레이어와 몬스터의 크기를 고려한 충돌 감지
    Vec2 vPlayerScale = m_pOwner->GetScale();
    Vec2 vTargetScale = _pTarget->GetScale();
    float fCollisionDistance = (vPlayerScale.x + vTargetScale.x) * 0.3f; // 스케일의 30%를 충돌 거리로
    fCollisionDistance = max(25.f, fCollisionDistance); // 최소 25픽셀
    
    if (fDistance < fCollisionDistance)
    {
        SwallowTarget(_pTarget);
        return;
    }

    const float fMaxInhaleTime = 0.35f;
    float fCurrentTime = pTargetInfo->fInhaleTimer;

    if (fCurrentTime >= fMaxInhaleTime)
    {
        // 시간이 다 되었으면 강제로 빨아들이기
        SwallowTarget(_pTarget);
        return;
    }

    // === 블랙홀 효과: 실제 거리 기반 가속도 ===
    Vec2 vDirection = vDiff.GetNormalized();
    float fTotalDistance = (pTargetInfo->vInitialPos - vPlayerPos).Length();
    
    // 진행률을 거리 기반으로 계산 (실제로 얼마나 가까워졌는지)
    float fCurrentDistance = fDistance;
    float fDistanceProgress = 1.0f - (fCurrentDistance / fTotalDistance); // 가까워질수록 1에 가까워짐
    fDistanceProgress = max(0.0f, min(1.0f, fDistanceProgress)); // 0~1 범위로 제한
    
    // 시간 기반 진행률도 계산
    float fTimeProgress = fCurrentTime / fMaxInhaleTime;
    
    // 두 진행률 중 더 높은 것을 사용 (거리든 시간이든 빨리 진행되는 쪽)
    float fProgress = max(fDistanceProgress, fTimeProgress);
    
    // 지수적 가속 곡선 강화 (더 극적인 효과)
    float fSpeedMultiplier = powf(fProgress, 2.0f); // 제곱 곡선으로 변경 (더 부드러운 가속)
    
    // 거리에 따른 기본 속도 계산
    float fBaseSpeed = 800.f; // 기본 속도를 고정값으로 
    
    // 거리가 가까울수록 더 빠르게 (역제곱 법칙 적용)
    float fDistanceMultiplier = 1.0f;
    if (fCurrentDistance > 10.f)
    {
        fDistanceMultiplier = min(5.0f, (fTotalDistance * 0.5f) / fCurrentDistance);
    }
    
    // 최종 속도 = 기본속도 * 시간가속배율 * 거리배율
    float fCurrentSpeed = fBaseSpeed * (1.0f + fSpeedMultiplier * 3.0f) * fDistanceMultiplier;
    
    // 최소/최대 속도 제한
    const float fMinSpeedLimit = 100.f;   // 최소 속도
    const float fMaxSpeedLimit = 2500.f;  // 최대 속도
    fCurrentSpeed = max(fMinSpeedLimit, min(fMaxSpeedLimit, fCurrentSpeed));
    
    Vec2 vVelocity = vDirection * fCurrentSpeed;

    // RigidBody에 속도 적용 (몬스터용)
    CRigidBody* pTargetRigidBody = _pTarget->GetRigidBody();
    if (pTargetRigidBody)
    {
        pTargetRigidBody->SetVelocity(vVelocity);
    }
    else
    {
        // RigidBody가 없는 경우 (CAbilityStar 등) 직접 위치 이동
        CAbilityStar* pAbilityStar = dynamic_cast<CAbilityStar*>(_pTarget);
        if (pAbilityStar)
        {
            Vec2 vCurrentPos = _pTarget->GetPos();
            Vec2 vNewPos = vCurrentPos + (vVelocity * CTimeMgr::GetInst()->GetfDT());
            _pTarget->SetPos(vNewPos);
        }
    }
}

Vec2 CPlayerInhaleSystem::CalculateInhaleDirection()
{
    if (!m_pOwner)
        return Vec2(1.f, 0.f);

    return m_pOwner->IsFacingRight() ? Vec2(1.f, 0.f) : Vec2(-1.f, 0.f);
}

bool CPlayerInhaleSystem::HasBeingInhaledMonsters() const
{
    CScene* pCurScene = CSceneMgr::GetInst()->GetCurScene();
    if (!pCurScene)
        return false;
    
    const vector<CObject*>& vecMonsters = pCurScene->GetGroupObject(GROUP_TYPE::MONSTER);
    
    for (CObject* pObj : vecMonsters)
    {
        if (pObj && !pObj->IsDead())
        {
            CBasicMonster* pBasic = dynamic_cast<CBasicMonster*>(pObj);
            if (pBasic && pBasic->IsBeingInhaled())
            {
                return true;
            }
        }
    }
    
    return false;
}

// === 파티클 시스템 구현 ===

void CPlayerInhaleSystem::UpdateParticles()
{
    if (!m_bInhaling)
    {
        ClearParticles();
        return;
    }
    
    // 파티클 생성
    SpawnParticles();
    
    // 죽은 파티클 제거
    auto it = m_vecAirParticles.begin();
    while (it != m_vecAirParticles.end())
    {
        CAirParticle* pParticle = *it;
        
        // 디버그: 파티클 상태 확인
        bool bIsNull = !pParticle;
        bool bIsDead = pParticle ? pParticle->IsDead() : false;
        bool bIsNotAlive = pParticle ? !pParticle->IsAlive() : true;

        if (!pParticle || pParticle->IsDead() || !pParticle->IsAlive())
        {
            it = m_vecAirParticles.erase(it);
        }
        else
        {
            ++it;
        }
    }
}

void CPlayerInhaleSystem::SpawnParticles()
{
    if (!m_pOwner) return;
    
    // 파티클 개수 제한 (최대 10개)
    if (m_vecAirParticles.size() >= 10)
        return;
    
    float fDT = CTimeMgr::GetInst()->GetfDT();
    m_fParticleSpawnTimer += fDT;
    
    if (m_fParticleSpawnTimer >= m_fParticleSpawnInterval)
    {
        m_fParticleSpawnTimer = 0.f;
        
        // 커비 위치와 방향
        Vec2 vKirbyPos = m_pOwner->GetPos();
        Vec2 vInhaleDir = CalculateInhaleDirection();
        
        // 빨아들이기 범위 내 랜덤 위치에 파티클 생성 (2-4개로 줄임)
        int particleCount = 2 + rand() % 3;
        
        for (int i = 0; i < particleCount; ++i)
        {
            // 범위 내 랜덤 위치 계산
            float fAngle = (rand() % 120 - 60) * 3.14159f / 180.0f; // -60도 ~ +60도
            float fDistance = 50.f + (rand() % 150); // 50~200픽셀
            
            Vec2 vSpawnPos = vKirbyPos + Vec2(
                vInhaleDir.x * fDistance * cos(fAngle),
                vInhaleDir.y * fDistance * cos(fAngle) + sin(fAngle) * fDistance * 0.5f
            );
            
            // 파티클 생성
            CAirParticle* pParticle = new CAirParticle();
            if (pParticle)
            {
                pParticle->SetPos(vSpawnPos);
                pParticle->SetTarget(vKirbyPos + vInhaleDir * 30.f); // 커비 입 근처를 목표로
                pParticle->SetMoveSpeed(100.f + rand() % 100); // 100~200 속도
                pParticle->SetLifeTime(2.f); // 2초 생존
                
                // 초기화 (텍스처 로드 및 랜덤 스프라이트 선택)
                pParticle->Init();
                
                // 씬에 추가
                CREATE_OBJECT(pParticle, GROUP_TYPE::EFFECT);
                m_vecAirParticles.push_back(pParticle);
            }
        }
    }
}

void CPlayerInhaleSystem::ClearParticles()
{
    // 파티클들을 삭제 요청 (실제 삭제는 씬에서 처리)
    for (CAirParticle* pParticle : m_vecAirParticles)
    {
        if (pParticle)
        {
            DeleteObject(pParticle);
        }
    }
    m_vecAirParticles.clear();
    m_fParticleSpawnTimer = 0.f;
}