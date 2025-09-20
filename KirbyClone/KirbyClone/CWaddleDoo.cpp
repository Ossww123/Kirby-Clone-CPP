#include "gamePCH.h"
#include "CWaddleDoo.h"
#include "CSceneMgr.h"
#include "CScene.h"
#include "CPlayer.h"
#include "CProjectileFactory.h"
#include "CProjectile.h"
#include "CTimeMgr.h"
#include "CRigidBody.h"
#include "CSoundMgr.h"

CWaddleDoo::CWaddleDoo()
    : m_bBeamFired(false)
    , m_fBeamSweepTimer(0.f)
    , m_fCurrentBeamAngle(-90.f)  // 위쪽 수직에서 시작
    , m_iBeamSweepStep(0)
{
    // 오브젝트 타입 설정
    SetType(OBJECT_TYPE::MONSTER_WADDLE_DOO);

    // 웨이들 두 전용 설정
    m_fSpeed = 60.f;                        // 웨이들 디보다 약간 느림
    SetAttackCooldown(5.f);                 // 4초 쿨타임
    m_fAttackRange = 200.f;                 // 200픽셀 범위
    m_fAttackReadyTime = 1.2f;              // 0.8초 준비
    m_fAttackDuration = 1.0f;               // 1.0초 공격

    // 애니메이션 로드
    LoadAnimationsFromFile(L"waddle_doo_animations.json");
    SetupAnimationMapping();

    // 초기 상태 설정
    ChangeState(MONSTER_STATE::IDLE);
}

CWaddleDoo::~CWaddleDoo()
{
    // 빔 투사체들 정리
    ClearBeamProjectiles();
    // 상위 클래스에서 정리
}

void CWaddleDoo::Move()
{
    // TURN 상태이거나 공격 관련 상태에서는 Move 로직 실행하지 않음
    MONSTER_STATE eCurrentState = GetCurrentState();
    if (eCurrentState == MONSTER_STATE::TURN ||
        eCurrentState == MONSTER_STATE::ATTACK_READY ||
        eCurrentState == MONSTER_STATE::ATTACK ||
        eCurrentState == MONSTER_STATE::DAMAGE ||
        eCurrentState == MONSTER_STATE::BEING_INHALED)
    {
        return;
    }

    // 플레이어 탐지 및 공격 체크 (공격 쿨타임이 끝났을 때만)
    if (CanAttack() && IsPlayerInRange())
    {
        ChangeState(MONSTER_STATE::ATTACK_READY);
        return;
    }

    // 앞방에 벽이 있거나 바닥이 없으면 방향 전환
    if (CheckWallAhead() || !CheckGroundAhead())
    {
        ChangeState(MONSTER_STATE::TURN);
        return;
    }

    // 계속 걷기
    MoveHorizontal(m_fSpeed);
}

void CWaddleDoo::Attack()
{
    // 빔 공격 실행 (상태 시작 시 한 번만)
    if (!m_bBeamFired)
    {
        ShootBeam();
        m_bBeamFired = true;
    }
}

void CWaddleDoo::UpdateAttackReady()
{
    // 부모 클래스 호출
    CCopyMonster::UpdateAttackReady();
    
    // ATTACK_READY 상태에서는 빔 발사 준비
    m_bBeamFired = false;  // 다음 공격을 위해 리셋
    m_fBeamSweepTimer = 0.f;  // 쓸기 타이머 리셋
    m_fCurrentBeamAngle = -90.f;  // 위쪽 수직에서 시작
    m_iBeamSweepStep = 0;  // 쓸기 단계 리셋
    ClearBeamProjectiles();  // 이전 빔들 정리
}

void CWaddleDoo::UpdateAttack()
{
    // 빔 쓸어내리기 업데이트
    UpdateBeamSweep();
    
    // 공격 시간이 끝나면 상태 변경
    if (m_fStateTimer >= m_fAttackDuration)
    {
        EndAttack();
    }
    else
    {
        // 공격 중에는 이동 정지
        if (nullptr != GetRigidBody())
        {
            GetRigidBody()->SetVelocityX(0.f);
        }
    }
}

void CWaddleDoo::SetupAnimationMapping()
{
    // 웨이들 두 상태별 애니메이션 매핑
    m_mapStateToAnimation[MONSTER_STATE::IDLE] = L"IDLE";
    m_mapStateToAnimation[MONSTER_STATE::WALK] = L"WALK";
    m_mapStateToAnimation[MONSTER_STATE::TURN] = L"WALK";
    m_mapStateToAnimation[MONSTER_STATE::ATTACK_READY] = L"ATTACK_READY";  // 뜸들이기
    m_mapStateToAnimation[MONSTER_STATE::ATTACK] = L"ATTACK";              // 빔 발사
    m_mapStateToAnimation[MONSTER_STATE::DAMAGE] = L"DAMAGE";
    m_mapStateToAnimation[MONSTER_STATE::BEING_INHALED] = L"DAMAGE";
}

void CWaddleDoo::ShootBeam()
{
    // 플레이어 방향으로 몸 돌리기
    AimAtPlayer();
    
    // 빔 투사체 생성
    CreateBeamProjectile();

    // TODO: 빔 발사 사운드
    // TODO: 빔 발사 이펙트
}

void CWaddleDoo::CreateBeamProjectile()
{
    // 빔 띠 형태로 6개 투사체 생성 (수직에서 시작)
    Vec2 vMyPos = GetPos();
    
    CScene* pScene = CSceneMgr::GetInst()->GetCurScene();
    if (!pScene)
    {
        return;
    }
    
    // 기존 빔들 정리
    ClearBeamProjectiles();
    
    // 빔 띠 설정
    const int BEAM_COUNT = 6;           // 빔 개수 (일직선으로 배치)
    const float BEAM_SPACING = 32.f;    // 빔 간 간격
    
    // 수직 방향 (-90도)에서 시작 (위쪽 방향)
    m_fCurrentBeamAngle = -90.f;
    float fRadians = m_fCurrentBeamAngle * 3.14159f / 180.f;
    Vec2 vDirection = Vec2(cosf(fRadians), sinf(fRadians));
    
    // 웨이들두 바라보는 방향 고려한 시작 위치
    Vec2 vStartPos = vMyPos;
    if (m_iDir > 0)  // 오른쪽을 보고 있으면
    {
        vStartPos.x += 20.f;  // 약간 앞쪽에서 시작
    }
    else  // 왼쪽을 보고 있으면
    {
        vStartPos.x -= 20.f;
    }

    CSoundMgr::GetInst ( )->PlaySFX ( L"waddledoo" );
    
    // 빔 띠의 가장 가까운 지점부터 멀리까지 생성
    for (int i = 0; i < BEAM_COUNT; ++i)
    {
        // 웨이들두로부터의 거리 (가까운 것부터)
        Vec2 vBeamPos = vStartPos;
        vBeamPos.x += vDirection.x * (i * BEAM_SPACING);
        vBeamPos.y += vDirection.y * (i * BEAM_SPACING);
        
        // 빔 투사체 생성
        CProjectile* pBeam = CProjectileFactory::CreateMonsterBeam(vBeamPos, vDirection, GROUP_TYPE::PROJ_MONSTER);
        if (pBeam)
        {
            // 빔 속도를 0으로 설정 (고정된 위치에서 각도만 변경)
            pBeam->SetSpeed(0.f);
            
            // 씬에 투사체 추가
            pScene->AddObject(pBeam, GROUP_TYPE::PROJ_MONSTER);
            
            // 빔 목록에 추가
            m_vecBeamProjectiles.push_back(pBeam);
        }
        else
        {
            wchar_t szDebug[256];
            swprintf_s(szDebug, L"WaddleDoo: Failed to create beam[%d] - CreateMonsterBeam returned null\n", i);
            OutputDebugStringW(szDebug);
        }
    }
}

void CWaddleDoo::UpdateBeamSweep()
{
    // 빔이 없으면 생성
    if (m_vecBeamProjectiles.empty() && m_iBeamSweepStep == 0)
    {
        CreateBeamProjectile();
        return;
    }
    
    if (m_vecBeamProjectiles.empty())
    {
        return;
    }
    
    // 쓸기 타이머 업데이트 (일정 프레임마다 각도 변경)
    m_fBeamSweepTimer += CTimeMgr::GetInst()->GetfDT();
    
    const float SWEEP_INTERVAL = 0.1f;  // 0.1초마다 각도 변경
    const float ANGLE_STEP = 15.f;      // 한 번에 15도씩 변경
    const int MAX_STEPS = 6;            // 총 6단계 (-90도 -> 0도)
    
    if (m_fBeamSweepTimer >= SWEEP_INTERVAL && m_iBeamSweepStep < MAX_STEPS)
    {
        m_fBeamSweepTimer = 0.f;
        m_iBeamSweepStep++;
        
        // 새로운 각도 계산 (-90도에서 0도까지)
        m_fCurrentBeamAngle = -90.f + (m_iBeamSweepStep * ANGLE_STEP);
        
        // 방향에 따라 각도 조정
        float fTargetAngle = m_fCurrentBeamAngle;
        if (m_iDir < 0)  // 왼쪽을 보고 있으면
        {
            fTargetAngle = -180.f - m_fCurrentBeamAngle;  // 대칭으로 뒤집기
        }
        
        float fRadians = fTargetAngle * 3.14159f / 180.f;
        Vec2 vDirection = Vec2(cosf(fRadians), sinf(fRadians));
        
        // 웨이들두에 가까운 투사체부터 2개씩 각도 변경
        int projectilesToUpdate = min(2 * (m_iBeamSweepStep), (int)m_vecBeamProjectiles.size());
        
        Vec2 vMyPos = GetPos();
        Vec2 vStartPos = vMyPos;
        if (m_iDir > 0)
        {
            vStartPos.x += 20.f;
        }
        else
        {
            vStartPos.x -= 20.f;
        }
        
        for (int i = 0; i < projectilesToUpdate; ++i)
        {
            if (i < (int)m_vecBeamProjectiles.size() && m_vecBeamProjectiles[i])
            {
                // 새로운 위치 계산
                Vec2 vNewPos = vStartPos;
                vNewPos.x += vDirection.x * (i * 32.f);
                vNewPos.y += vDirection.y * (i * 32.f);
                
                // 투사체 위치와 방향 업데이트
                m_vecBeamProjectiles[i]->SetPos(vNewPos);
                // TODO: 투사체 방향도 업데이트 필요하면 여기서
            }
        }
    }
    
    // 쓸기가 완료되면 빔들 정리
    if (m_iBeamSweepStep >= MAX_STEPS)
    {
        ClearBeamProjectiles();
    }
}

void CWaddleDoo::ClearBeamProjectiles()
{
    // 빔 투사체들을 삭제하지 않고 벡터에서만 제거
    // (실제 삭제는 씬에서 처리)
    for (CProjectile* pBeam : m_vecBeamProjectiles)
    {
        if (pBeam)
        {
            pBeam->SetDead();  // 삭제 표시
        }
    }
    m_vecBeamProjectiles.clear();
}