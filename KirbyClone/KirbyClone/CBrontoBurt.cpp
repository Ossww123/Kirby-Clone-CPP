#include "pch.h"
#include "CBrontoBurt.h"
#include "CTimeMgr.h"
#include "CRigidBody.h"

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
    CreateAnimations();

    // 초기 상태를 FLY로 설정 (WALK가 아님!)
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

    // 화면 경계 체크
    Vec2 currentPos = GetPos();
    if (currentPos.x < -50.f || currentPos.x > 1970.f)  // 화면 경계
    {
        TurnAround();
        m_vStartPos.x = currentPos.x;  // 새로운 기준점 설정
    }
}

void CBrontoBurt::CreateAnimations()
{
    // 세 번째 행: 브론토 버트 - 시작 위치 (8, 72)
    Vec2 startPos = Vec2(8.f, 8.f + 64.f);

    // FLY 애니메이션 (1~4열)
    CreateBasicAnimation(L"FLY", startPos, 4, Vec2(32.f, 32.f), Vec2(32.f, 32.f), 0.15f, true);

    // DAMAGE 애니메이션 (5~8열) - 빨아들임 중에도 사용
    CreateBasicAnimation(L"DAMAGE", Vec2(startPos.x + 32.f * 4, startPos.y), 4,
        Vec2(32.f, 32.f), Vec2(32.f, 32.f), 0.15f, false);

    // IDLE은 FLY와 동일
    CreateBasicAnimation(L"IDLE", startPos, 1, Vec2(32.f, 32.f), Vec2(32.f, 32.f), 0.5f, true);
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