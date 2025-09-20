#include "gamePCH.h"
#include "CBrontoBurt.h"
#include "CTimeMgr.h"
#include "CRigidBody.h"
#include "CCamera.h"
#include "CCore.h"

CBrontoBurt::CBrontoBurt()
    : m_fFlightTimer(0.f)
    , m_fWaveAmplitude(50.f)        // 사인파 진폭 50픽셀
    , m_fWaveFrequency(2.f)         // 사인파 주파수
    , m_vStartPos(Vec2(0.f, 0.f))
{
    // 오브젝트 타입 설정
    SetType(OBJECT_TYPE::MONSTER_BRONTO_BURT);

    // 브론토 버트 전용 설정
    m_fSpeed = 100.f;

    // 비행 몬스터이므로 중력 비활성화
    GetRigidBody()->SetUseGravity(false);

    // 시작 위치 저장
    m_vStartPos = GetPos();

    // 애니메이션 생성

    // 초기 상태를 FLY로 설정 (WALK가 아님!)
    // 애니메이션 로드
    LoadAnimationsFromFile(L"bronto_burt_animations.json");
    
    // 애니메이션 매핑 설정
    SetupAnimationMapping();

    ChangeState(MONSTER_STATE::FLY);
}

CBrontoBurt::~CBrontoBurt()
{
    // 상위 클래스에서 정리
}

void CBrontoBurt::Move()
{
    // 사인파 비행 패턴 업데이트
    UpdateFlightPattern();

    // 경계 체크 및 Dead 처리
    Vec2 currentPos = GetPos();
    
    // 1. 카메라 경계 체크
    Vec2 cameraPos = CCamera::GetInst()->GetLookAt();
    Vec2 resolution = CCore::GetInst()->GetResolution();
    float cameraLeft = cameraPos.x - resolution.x/2.f - 200.f;
    float cameraRight = cameraPos.x + resolution.x/2.f + 200.f;
    
    // 2. 스테이지 경계 체크
    Vec2 stageBoundsMin, stageBoundsMax;
    CCamera::GetInst()->GetStageBounds(stageBoundsMin, stageBoundsMax);
    
    // 3. 경계 벗어나면 Dead 처리
    if (currentPos.x < cameraLeft || currentPos.x > cameraRight ||
        currentPos.x < stageBoundsMin.x - 100.f || currentPos.x > stageBoundsMax.x + 100.f)
    {
        SetDead();
        return;  // Move() 함수 종료
    }
}

void CBrontoBurt::SetupAnimationMapping()
{
    // 브론토버트는 주로 FLY 상태를 사용
    m_mapStateToAnimation[MONSTER_STATE::FLY] = L"FLY";
    m_mapStateToAnimation[MONSTER_STATE::IDLE] = L"IDLE";
    m_mapStateToAnimation[MONSTER_STATE::DAMAGE] = L"DAMAGE";
    m_mapStateToAnimation[MONSTER_STATE::BEING_INHALED] = L"DAMAGE";
}

void CBrontoBurt::UpdateFlightPattern()
{
    // 사인파 패턴으로 상하 움직임 + 좌우 이동
    m_fFlightTimer += CTimeMgr::GetInst()->GetfDT();

    // 사인파를 이용한 Y 좌표 계산
    float fSinValue = sinf(m_fFlightTimer * m_fWaveFrequency);
    float fNewY = m_vStartPos.y + (fSinValue * m_fWaveAmplitude);

    // 수평 이동
    Vec2 currentPos = GetPos();
    float fNewX = currentPos.x + (m_fSpeed * m_iDir * CTimeMgr::GetInst()->GetfDT());

    // 새 위치 설정
    SetPos(Vec2(fNewX, fNewY));

    // 리지드바디 속도도 설정 (물리 시뮬레이션과 동기화)
    if (nullptr != GetRigidBody())
    {
        Vec2 velocity = Vec2(m_fSpeed * m_iDir,
            fSinValue * m_fWaveAmplitude * m_fWaveFrequency);
        GetRigidBody()->SetVelocity(velocity);
    }
}