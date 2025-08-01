#include "pch.h"
#include "CPlayer.h"

#include "CKeyMgr.h"
#include "CTimeMgr.h"
#include "CCollider.h"
#include "CEventMgr.h"
#include "CCamera.h"
#include "CResMgr.h"
#include "CTexture.h"
#include "CAnimator.h"
#include "CRigidBody.h"
#include "CTile.h"
#include "CMonster.h"
#include "CAnimation.h"
#include "CScene.h"
#include "CSceneMgr.h"

#include "CCore.h"

CPlayer::CPlayer()
    : m_pAnimator(nullptr)
    , m_pRigidBody(nullptr)
    , m_eCurState(PLAYER_STATE::IDLE)
    , m_ePrevState(PLAYER_STATE::END)
    , m_fSpeed(300.f)           // 기본 걷기 속도
    , m_fRunSpeed(250.f)        // 달리기 속도 (새로 추가)
    , m_fJumpPower(400.f)
    , m_bInhaling(false)        // 새로 추가
    , m_fInhaleTime(0.f)        // 새로 추가
    , m_bHasMouthful(false)     // 새로 추가
    , m_fInhaleRange(150.f)         // 빨아들이기 범위
    , m_vInhaleDir(Vec2(1.f, 0.f))  // 기본 오른쪽 방향
    , m_pMouthfulTarget(nullptr)
    , m_eMouthfulType(OBJECT_TYPE::END)
    , m_bPlayingInhaleEffect(false)
    , m_bRunMode(false)         // 새로 추가
    , m_bFacingRight(true)      // 기본적으로 오른쪽을 보고 시작
    , m_iLastMoveDir(0)         // 초기에는 정지 상태
    , m_bDirectionChanged(false)
    , m_fDeceleration(600.f)        // 감속도 (높을수록 빨리 멈춤)
    , m_fMinMovingSpeed(30.f)       // 최소 이동 속도
    , m_bIsDecelerating(false)
    , m_fDecelTimer(0.f)
    , m_bWasMovingLastFrame(false)
    , m_bInputPressed(false)
{
    // 충돌체 생성
    CreateCollider();
    GetCollider()->SetScale(Vec2(56.f, 56.f));

    // 애니메이터 생성
    CreateAnimator();
    m_pAnimator = GetAnimator();

    // 리지드바디 생성
    CreateRigidBody();
    m_pRigidBody = GetRigidBody();

    // 리지드바디 설정
    m_pRigidBody->SetMass(1.f);
    m_pRigidBody->SetMaxVelocity(500.f);
    m_pRigidBody->SetFriction(0.1f);
    m_pRigidBody->SetUseGravity(true);

    // 애니메이션 생성
    CreateAnimation();

    // 초기 애니메이션 재생
    m_pAnimator->Play(L"IDLE", true);
}

CPlayer::~CPlayer()
{
    // 부모 클래스에서 이미 m_pAnimator를 삭제하므로
    // 여기서는 nullptr로만 설정
    m_pAnimator = nullptr;
    m_pRigidBody = nullptr;
}

void CPlayer::CreateAnimation()
{
    // 새로운 커비 스프라이트 시트 로드
    CTexture* pKirbyTex = CResMgr::GetInst()->LoadTexture(L"KirbySprite", L"texture\\kirby\\kirby.bmp");

    // 각 스프라이트는 120x120이고, 실제 이미지는 2px 테두리 제외하고 116x116
    const int SPRITE_SIZE = 120;
    const int IMAGE_SIZE = 116;
    const int BORDER = 2;

    // ===========================================
    // 1행: IDLE 상태 (10프레임)
    // ===========================================
    CAnimation* pIdleAnim = new CAnimation();
    pIdleAnim->SetName(L"IDLE");
    pIdleAnim->SetTexture(pKirbyTex);
    pIdleAnim->SetLoop(true);

    // 깜빡임 시퀀스: 눈뜸(1.5초) → 깜빡(0.1초) → 눈뜸(1초) → 깜빡(0.1초) → 눈뜸(0.2초) → 깜빡(0.1초)

    // 1. 눈 뜨고 오래 대기 (1.5초)
    pIdleAnim->AddFrame(Vec2(BORDER, BORDER), Vec2(IMAGE_SIZE, IMAGE_SIZE), 1.5f);

    // 2. 첫 번째 깜빡임 (0.1초)
    pIdleAnim->AddFrame(Vec2(BORDER + SPRITE_SIZE, BORDER), Vec2(IMAGE_SIZE, IMAGE_SIZE), 0.1f);

    // 3. 눈 뜨고 중간 대기 (1초)
    pIdleAnim->AddFrame(Vec2(BORDER, BORDER), Vec2(IMAGE_SIZE, IMAGE_SIZE), 1.0f);

    // 4. 두 번째 깜빡임 (0.1초)
    pIdleAnim->AddFrame(Vec2(BORDER + SPRITE_SIZE, BORDER), Vec2(IMAGE_SIZE, IMAGE_SIZE), 0.1f);

    // 5. 눈 뜨고 짧은 대기 (0.2초)
    pIdleAnim->AddFrame(Vec2(BORDER, BORDER), Vec2(IMAGE_SIZE, IMAGE_SIZE), 0.2f);

    // 6. 세 번째 깜빡임 (0.1초)
    pIdleAnim->AddFrame(Vec2(BORDER + SPRITE_SIZE, BORDER), Vec2(IMAGE_SIZE, IMAGE_SIZE), 0.1f);

    // 애니메이터에 추가
    m_pAnimator->AddCustomAnimation(L"IDLE", pIdleAnim);

    // ===========================================
    // 2행: 걷기 상태 (10프레임)
    // ===========================================
    m_pAnimator->CreateAnimation(L"WALK", pKirbyTex,
        Vec2(BORDER, SPRITE_SIZE + BORDER),
        Vec2(IMAGE_SIZE, IMAGE_SIZE),
        Vec2(SPRITE_SIZE, 0),
        0.1f,                           // 걷기는 좀 더 빠르게
        10,
        true);

    // ===========================================
    // 3행: 뛰기 상태 (10프레임)
    // ===========================================
    m_pAnimator->CreateAnimation(L"RUN", pKirbyTex,
        Vec2(BORDER, SPRITE_SIZE * 2 + BORDER),
        Vec2(IMAGE_SIZE, IMAGE_SIZE),
        Vec2(SPRITE_SIZE, 0),
        0.08f,                          // 뛰기는 더 빠르게
        10,
        true);

    // ===========================================
    // 4행: 점프 상태 (10프레임)
    // ===========================================
    m_pAnimator->CreateAnimation(L"JUMP", pKirbyTex,
        Vec2(BORDER, SPRITE_SIZE * 3 + BORDER),
        Vec2(IMAGE_SIZE, IMAGE_SIZE),
        Vec2(SPRITE_SIZE, 0),
        0.1f,
        10,
        false);                         // 점프는 반복 안함

    // ===========================================
    // 5행: 낙하 - 땅에서 바운스 (10프레임)
    // ===========================================
    m_pAnimator->CreateAnimation(L"FALL", pKirbyTex,
        Vec2(BORDER, SPRITE_SIZE * 4 + BORDER),
        Vec2(IMAGE_SIZE, IMAGE_SIZE),
        Vec2(SPRITE_SIZE, 0),
        0.1f,
        10,
        true);

    // ===========================================
    // 6행: 공기머금기 (10프레임)
    // ===========================================
    m_pAnimator->CreateAnimation(L"INHALE_READY", pKirbyTex,
        Vec2(BORDER, SPRITE_SIZE * 5 + BORDER),
        Vec2(IMAGE_SIZE, IMAGE_SIZE),
        Vec2(SPRITE_SIZE, 0),
        0.1f,
        10,
        false);

    // ===========================================
    // 7행: 공기뱉기 (10프레임)
    // ===========================================
    m_pAnimator->CreateAnimation(L"EXHALE", pKirbyTex,
        Vec2(BORDER, SPRITE_SIZE * 6 + BORDER),
        Vec2(IMAGE_SIZE, IMAGE_SIZE),
        Vec2(SPRITE_SIZE, 0),
        0.08f,
        10,
        false);

    // ===========================================
    // 8행: 빨아들이기 1단계, 2단계, 숨참 (10프레임)
    // ===========================================
    // 1단계 (처음 3프레임)
    m_pAnimator->CreateAnimation(L"INHALE_1", pKirbyTex,
        Vec2(BORDER, SPRITE_SIZE * 7 + BORDER),
        Vec2(IMAGE_SIZE, IMAGE_SIZE),
        Vec2(SPRITE_SIZE, 0),
        0.15f,
        3,
        true);

    // 2단계 (다음 3프레임)
    m_pAnimator->CreateAnimation(L"INHALE_2", pKirbyTex,
        Vec2(BORDER + SPRITE_SIZE * 3, SPRITE_SIZE * 7 + BORDER),
        Vec2(IMAGE_SIZE, IMAGE_SIZE),
        Vec2(SPRITE_SIZE, 0),
        0.12f,
        3,
        true);

    // 숨참 (마지막 4프레임)
    m_pAnimator->CreateAnimation(L"INHALE_HOLD", pKirbyTex,
        Vec2(BORDER + SPRITE_SIZE * 6, SPRITE_SIZE * 7 + BORDER),
        Vec2(IMAGE_SIZE, IMAGE_SIZE),
        Vec2(SPRITE_SIZE, 0),
        0.2f,
        4,
        true);

    // ===========================================
    // 9행: 빨아들이기 공기 파티클 (10프레임) - 이펙트용
    // ===========================================
    m_pAnimator->CreateAnimation(L"INHALE_EFFECT", pKirbyTex,
        Vec2(BORDER, SPRITE_SIZE * 8 + BORDER),
        Vec2(IMAGE_SIZE, IMAGE_SIZE),
        Vec2(SPRITE_SIZE, 0),
        0.05f,
        10,
        true);

    // ===========================================
    // 10행: 삼키기 (3칸씩 사용하므로 3프레임)
    // ===========================================
    m_pAnimator->CreateAnimation(L"SWALLOW", pKirbyTex,
        Vec2(BORDER, SPRITE_SIZE * 9 + BORDER),
        Vec2(IMAGE_SIZE, IMAGE_SIZE),
        Vec2(SPRITE_SIZE, 0),
        0.15f,
        3,
        false);

    // ===========================================
    // 11행: 머금은 상태 IDLE (10프레임)
    // ===========================================
    m_pAnimator->CreateAnimation(L"MOUTHFUL_IDLE", pKirbyTex,
        Vec2(BORDER, SPRITE_SIZE * 10 + BORDER),
        Vec2(IMAGE_SIZE, IMAGE_SIZE),
        Vec2(SPRITE_SIZE, 0),
        0.15f,
        10,
        true);

    // ===========================================
    // 12행: 머금은 상태 걷기/뛰기 (10프레임)
    // ===========================================
    m_pAnimator->CreateAnimation(L"MOUTHFUL_WALK", pKirbyTex,
        Vec2(BORDER, SPRITE_SIZE * 11 + BORDER),
        Vec2(IMAGE_SIZE, IMAGE_SIZE),
        Vec2(SPRITE_SIZE, 0),
        0.1f,
        10,
        true);

    // 머금은 상태 뛰기는 걷기와 같은 애니메이션을 더 빠르게 재생
    m_pAnimator->CreateAnimation(L"MOUTHFUL_RUN", pKirbyTex,
        Vec2(BORDER, SPRITE_SIZE * 11 + BORDER),
        Vec2(IMAGE_SIZE, IMAGE_SIZE),
        Vec2(SPRITE_SIZE, 0),
        0.07f,                          // 더 빠르게
        10,
        true);

    // ===========================================
    // 13-14행: 머금은 상태 점프 (2행 사용, 20프레임)
    // ===========================================
    // 수동으로 프레임 생성 (2행에 걸쳐 있어서)
    CAnimation* pMouthfulJumpAnim = new CAnimation;
    pMouthfulJumpAnim->SetName(L"MOUTHFUL_JUMP");
    pMouthfulJumpAnim->SetTexture(pKirbyTex);
    pMouthfulJumpAnim->SetLoop(false);

    // 첫 번째 행 (10프레임)
    for (int i = 0; i < 10; ++i)
    {
        pMouthfulJumpAnim->AddFrame(
            Vec2(BORDER + i * SPRITE_SIZE, SPRITE_SIZE * 12 + BORDER),
            Vec2(IMAGE_SIZE, IMAGE_SIZE),
            0.1f
        );
    }

    // 두 번째 행 (10프레임) 
    for (int i = 0; i < 10; ++i)
    {
        pMouthfulJumpAnim->AddFrame(
            Vec2(BORDER + i * SPRITE_SIZE, SPRITE_SIZE * 13 + BORDER),
            Vec2(IMAGE_SIZE, IMAGE_SIZE),
            0.1f
        );
    }

    m_pAnimator->AddCustomAnimation(L"MOUTHFUL_JUMP", pMouthfulJumpAnim);

    // 초기 애니메이션 재생
    m_pAnimator->Play(L"IDLE", true);
}

void CPlayer::Update()
{
    UpdateMove();
    UpdateDirection();  // 방향 업데이트 추가
    UpdateState();

    // 리지드바디 업데이트
    if (nullptr != m_pRigidBody)
        m_pRigidBody->Update();

    // 애니메이터 업데이트
    if (nullptr != m_pAnimator)
        m_pAnimator->Update();

    // 빨아들이기 이펙트 업데이트
    if (m_bInhaling)
    {
        m_bPlayingInhaleEffect = true;
    }
    else
    {
        m_bPlayingInhaleEffect = false;
    }

    // C키로 카메라가 플레이어를 따라가도록 설정/해제
    if (KEY_TAP(KEY::C))
    {
        if (CCamera::GetInst()->GetTarget() == this)
        {
            CCamera::GetInst()->SetTarget(nullptr);
            SetWindowText(CCore::GetInst()->GetMainHwnd(), L"카메라 추적 해제");
        }
        else
        {
            CCamera::GetInst()->SetTarget(this);
            SetWindowText(CCore::GetInst()->GetMainHwnd(), L"카메라 추적 시작");
        }
    }
}

void CPlayer::UpdateMove()
{
    if (nullptr == m_pRigidBody)
        return;

    // 물고 있는 것을 뱉기 (Z키)
    if (KEY_TAP(KEY::Z) && m_bHasMouthful)
    {
        SpitOut();
        return;
    }

    // 달리기 모드 토글 (Shift 키)
    if (KEY_TAP(KEY::SHIFT))
    {
        m_bRunMode = !m_bRunMode;
    }

    // 흡입하기 관련 처리는 기존과 동일...
    if (KEY_TAP(KEY::X))
    {
        if (!m_bInhaling && !m_bHasMouthful)
        {
            StartInhale();
        }
    }

    if (KEY_HOLD(KEY::X) && m_bInhaling)
    {
        UpdateInhale();
    }

    if (KEY_AWAY(KEY::X))
    {
        if (m_bInhaling)
        {
            StopInhale();
        }
    }

    // 흡입하기 중일 때는 이동 제한
    if (m_bInhaling)
    {
        m_pRigidBody->SetVelocityX(0.f);
        m_bInputPressed = false;
        return;
    }

    // === 개선된 이동 입력 처리 ===
    int currentMoveDir = 0;
    m_bInputPressed = false;

    if (KEY_HOLD(KEY::LEFT))
    {
        currentMoveDir = -1;
        m_bInputPressed = true;
        m_bIsDecelerating = false;  // 입력이 있으면 감속 중단

        float fCurrentSpeed = m_bRunMode ? m_fRunSpeed : m_fSpeed;
        if (m_bHasMouthful) fCurrentSpeed *= 0.7f;
        m_pRigidBody->SetVelocityX(-fCurrentSpeed);
    }
    else if (KEY_HOLD(KEY::RIGHT))
    {
        currentMoveDir = 1;
        m_bInputPressed = true;
        m_bIsDecelerating = false;  // 입력이 있으면 감속 중단

        float fCurrentSpeed = m_bRunMode ? m_fRunSpeed : m_fSpeed;
        if (m_bHasMouthful) fCurrentSpeed *= 0.7f;
        m_pRigidBody->SetVelocityX(fCurrentSpeed);
    }
    else
    {
        currentMoveDir = 0;

        // === 핵심 수정: 감속 시작 조건 개선 ===
        if (m_pRigidBody->IsGround())
        {
            Vec2 velocity = m_pRigidBody->GetVelocity();

            // 현재 움직이고 있고 아직 감속 중이 아니라면 감속 시작
            if (abs(velocity.x) > m_fMinMovingSpeed && !m_bIsDecelerating)
            {
                m_bIsDecelerating = true;
                m_fDecelTimer = 0.f;

                // 디버그용 메시지
                wchar_t szBuffer[256];
                swprintf_s(szBuffer, L"감속 시작! 현재 속도: %.1f", abs(velocity.x));
                SetWindowText(CCore::GetInst()->GetMainHwnd(), szBuffer);
            }
        }
    }

    // 감속 처리
    if (m_bIsDecelerating)
    {
        ApplyDeceleration();
    }

    // 이동 방향 기록
    m_iLastMoveDir = currentMoveDir;

    // 점프
    if (KEY_TAP(KEY::SPACE) && m_pRigidBody->IsGround())
    {
        m_pRigidBody->SetGround(false);
        m_pRigidBody->SetVelocityY(-m_fJumpPower);
    }

    // 움직임 상태 업데이트
    UpdateMovementState();
}

void CPlayer::UpdateMovementState()
{
    bool isCurrentlyMoving = IsActuallyMoving();

    // 이전 프레임과 현재 프레임의 움직임 상태 비교
    if (m_bWasMovingLastFrame && !isCurrentlyMoving && !m_bInputPressed)
    {
        // 움직이던 중에 입력이 없어지고 실제로 멈춤 → 감속 완료
        m_bIsDecelerating = false;
    }

    m_bWasMovingLastFrame = isCurrentlyMoving;
}

bool CPlayer::IsActuallyMoving()
{
    if (!m_pRigidBody) return false;

    Vec2 velocity = m_pRigidBody->GetVelocity();
    return abs(velocity.x) > m_fMinMovingSpeed;
}

// === 방향 시스템 핵심 함수들 ===
void CPlayer::UpdateDirection()
{
    m_bDirectionChanged = false;

    // 이동 중일 때만 방향 업데이트
    if (m_iLastMoveDir != 0)
    {
        bool newFacingRight = (m_iLastMoveDir > 0);

        if (m_bFacingRight != newFacingRight)
        {
            SetFacingDirection(newFacingRight);
            m_bDirectionChanged = true;
        }
    }
}

void CPlayer::ApplyDeceleration()
{
    if (!m_pRigidBody) return;

    Vec2 velocity = m_pRigidBody->GetVelocity();
    float currentSpeedX = velocity.x;

    // 디버그용 현재 속도 출력
    static float debugTimer = 0.f;
    debugTimer += CTimeMgr::GetInst()->GetfDT();
    if (debugTimer > 0.1f)  // 0.1초마다 출력
    {
        wchar_t szBuffer[256];
        swprintf_s(szBuffer, L"감속 중... 속도: %.1f", abs(currentSpeedX));
        SetWindowText(CCore::GetInst()->GetMainHwnd(), szBuffer);
        debugTimer = 0.f;
    }

    if (abs(currentSpeedX) <= m_fMinMovingSpeed)
    {
        // 속도가 최소값 이하로 떨어지면 완전히 정지
        m_pRigidBody->SetVelocityX(0.f);
        m_bIsDecelerating = false;

        SetWindowText(CCore::GetInst()->GetMainHwnd(), L"감속 완료! IDLE 상태로 전환");
        return;
    }

    // 감속 적용
    float deltaTime = CTimeMgr::GetInst()->GetfDT();
    float decelAmount = m_fDeceleration * deltaTime;

    if (currentSpeedX > 0)
    {
        // 오른쪽으로 이동 중이면 왼쪽으로 감속
        float newSpeedX = currentSpeedX - decelAmount;
        if (newSpeedX < m_fMinMovingSpeed) newSpeedX = 0;  // 최소 속도 이하로 떨어지면 0
        m_pRigidBody->SetVelocityX(newSpeedX);
    }
    else if (currentSpeedX < 0)
    {
        // 왼쪽으로 이동 중이면 오른쪽으로 감속
        float newSpeedX = currentSpeedX + decelAmount;
        if (newSpeedX > -m_fMinMovingSpeed) newSpeedX = 0;  // 최소 속도 이하로 떨어지면 0
        m_pRigidBody->SetVelocityX(newSpeedX);
    }

    m_fDecelTimer += deltaTime;
}

void CPlayer::SetFacingDirection(bool _bRight)
{
    if (m_bFacingRight != _bRight)
    {
        m_bFacingRight = _bRight;

        // 흡입 방향도 같이 업데이트
        UpdateInhaleDirection();

        // 디버그 메시지 (개발 중에만 사용)
#ifdef _DEBUG
        wchar_t szBuffer[256];
        swprintf_s(szBuffer, L"플레이어 방향 변경: %s", _bRight ? L"오른쪽" : L"왼쪽");
        SetWindowText(CCore::GetInst()->GetMainHwnd(), szBuffer);
#endif
    }
}

void CPlayer::UpdateInhaleDirection()
{
    // 현재 바라보는 방향으로 흡입 방향 설정
    m_vInhaleDir = m_bFacingRight ? Vec2(1.f, 0.f) : Vec2(-1.f, 0.f);
}

// 흡입하기 시작 - 방향 시스템과 연동
void CPlayer::StartInhale()
{
    m_bInhaling = true;
    m_fInhaleTime = 0.f;
    m_vecInhaleTargets.clear();

    // 현재 바라보는 방향으로 흡입 방향 설정
    UpdateInhaleDirection();
}

// 빨아들이기 업데이트
void CPlayer::UpdateInhale()
{
    m_fInhaleTime += CTimeMgr::GetInst()->GetfDT();

    // 빨아들이기 범위 내의 적들 찾기
    CScene* pCurScene = CSceneMgr::GetInst()->GetCurScene();
    const vector<CObject*>& vecMonsters = pCurScene->GetGroupObject(GROUP_TYPE::MONSTER);

    Vec2 vPlayerPos = GetPos();
    Vec2 vInhalePos = vPlayerPos + m_vInhaleDir * m_fInhaleRange * 0.5f;

    for (CObject* pMonster : vecMonsters)
    {
        if (!pMonster || pMonster->IsDead())
            continue;

        Vec2 vMonsterPos = pMonster->GetPos();
        Vec2 vDiff = vPlayerPos - vMonsterPos;
        float fDistance = vDiff.Length();

        // 빨아들이기 범위 내에 있고, 플레이어가 바라보는 방향에 있는지 확인
        if (fDistance <= m_fInhaleRange)
        {
            // 방향 체크 (플레이어가 바라보는 방향 120도 범위)
            Vec2 vToMonster = vMonsterPos - vPlayerPos;
            vToMonster.Normalize();

            float fDot = m_vInhaleDir.x * vToMonster.x + m_vInhaleDir.y * vToMonster.y;
            if (fDot > 0.5f) // cos(60도) = 0.5, 즉 120도 범위
            {
                // 몬스터를 플레이어 쪽으로 끌어당기기
                CRigidBody* pMonsterRigid = pMonster->GetRigidBody();
                if (pMonsterRigid)
                {
                    Vec2 vPullForce = vDiff;
                    vPullForce.Normalize();
                    vPullForce *= 300.f; // 끌어당기는 힘

                    pMonsterRigid->AddForce(vPullForce);
                }

                // 충분히 가까우면 삼키기 대상으로 등록
                if (fDistance < 50.f)
                {
                    auto iter = find(m_vecInhaleTargets.begin(), m_vecInhaleTargets.end(), pMonster);
                    if (iter == m_vecInhaleTargets.end())
                    {
                        m_vecInhaleTargets.push_back(pMonster);
                    }
                }
            }
        }
    }
}

// 빨아들이기 중단
void CPlayer::StopInhale()
{
    m_bInhaling = false;

    // 빨아들인 적이 있다면 삼키기
    if (!m_vecInhaleTargets.empty())
    {
        // 가장 가까운 적을 삼키기
        CObject* pClosest = nullptr;
        float fMinDistance = FLT_MAX;
        Vec2 vPlayerPos = GetPos();

        for (CObject* pTarget : m_vecInhaleTargets)
        {
            if (!pTarget || pTarget->IsDead())
                continue;

            float fDistance = (pTarget->GetPos() - vPlayerPos).Length();
            if (fDistance < fMinDistance)
            {
                fMinDistance = fDistance;
                pClosest = pTarget;
            }
        }

        if (pClosest)
        {
            SwallowTarget(pClosest);
        }
        else
        {
            ChangeState(PLAYER_STATE::EXHALE);
        }
    }
    else
    {
        ChangeState(PLAYER_STATE::EXHALE);
    }

    m_vecInhaleTargets.clear();
    m_fInhaleTime = 0.f;
}

// 적 삼키기
void CPlayer::SwallowTarget(CObject* _pTarget)
{
    if (!_pTarget)
        return;

    // 삼키기 상태로 전환
    ChangeState(PLAYER_STATE::SWALLOW);

    // 적의 타입 저장
    CMonster* pMonster = dynamic_cast<CMonster*>(_pTarget);
    if (pMonster)
    {
        // 몬스터 타입에 따라 능력 결정 (나중에 확장 가능)
        m_eMouthfulType = OBJECT_TYPE::MONSTER_WADDLE_DEE; // 기본값
    }

    m_pMouthfulTarget = _pTarget;

    // 적을 삭제 (이벤트로 처리)
    tEvent event(EVENT_TYPE::DELETE_OBJECT, 0, (DWORD_PTR)_pTarget);
    CEventMgr::GetInst()->AddEvent(event);
}

// 머금은 것을 뱉기
void CPlayer::SpitOut()
{
    if (!m_bHasMouthful)
        return;

    // 뱉기 애니메이션 재생
    ChangeState(PLAYER_STATE::EXHALE);

    // 투사체 생성 (나중에 구현 가능)
    // Vec2 vSpitPos = GetPos() + m_vInhaleDir * 50.f;
    // CreateSpitProjectile(vSpitPos, m_vInhaleDir);

    // 머금은 상태 해제
    ReleaseMouthful();
}

// 머금은 상태 해제
void CPlayer::ReleaseMouthful()
{
    m_bHasMouthful = false;
    m_pMouthfulTarget = nullptr;
    m_eMouthfulType = OBJECT_TYPE::END;
}


void CPlayer::UpdateState()
{
    if (nullptr == m_pRigidBody)
        return;

    PLAYER_STATE eNewState = m_eCurState;
    Vec2 vVelocity = m_pRigidBody->GetVelocity();

    // 흡입하기 상태 처리 (기존과 동일)
    if (m_bInhaling)
    {
        if (m_fInhaleTime < 0.5f)
        {
            eNewState = PLAYER_STATE::INHALE_1;
        }
        else if (m_fInhaleTime < 1.0f)
        {
            eNewState = PLAYER_STATE::INHALE_2;
        }
        else
        {
            eNewState = PLAYER_STATE::INHALE_HOLD;
        }
    }
    // 뱉기 애니메이션이 끝나는지 확인
    else if (m_eCurState == PLAYER_STATE::EXHALE)
    {
        if (m_pAnimator->GetCurAnim() && m_pAnimator->GetCurAnim()->IsFinish())
        {
            eNewState = PLAYER_STATE::IDLE;
        }
        else
        {
            eNewState = PLAYER_STATE::EXHALE;
        }
    }
    // 삼키기 애니메이션이 끝나는지 확인
    else if (m_eCurState == PLAYER_STATE::SWALLOW)
    {
        if (m_pAnimator->GetCurAnim() && m_pAnimator->GetCurAnim()->IsFinish())
        {
            m_bHasMouthful = true;
            eNewState = PLAYER_STATE::MOUTHFUL_IDLE;
        }
        else
        {
            eNewState = PLAYER_STATE::SWALLOW;
        }
    }
    // === 개선된 일반적인 상태 판정 ===
    else
    {
        // 공중에 있는 경우
        if (!m_pRigidBody->IsGround())
        {
            if (vVelocity.y < -50.f)
            {
                eNewState = m_bHasMouthful ? PLAYER_STATE::MOUTHFUL_JUMP : PLAYER_STATE::JUMP;
            }
            else
            {
                eNewState = PLAYER_STATE::FALL;
            }
        }
        // 땅에 있는 경우
        else
        {
            // === 핵심 개선: 실제 움직임 기반 상태 결정 ===
            bool isActuallyMoving = IsActuallyMoving();

            if (isActuallyMoving || m_bIsDecelerating)
            {
                // 실제로 움직이고 있거나 감속 중이면 WALK/RUN 유지
                if (m_bHasMouthful)
                {
                    eNewState = m_bRunMode ? PLAYER_STATE::MOUTHFUL_RUN : PLAYER_STATE::MOUTHFUL_WALK;
                }
                else
                {
                    eNewState = m_bRunMode ? PLAYER_STATE::RUN : PLAYER_STATE::WALK;
                }
            }
            else
            {
                // 완전히 멈춘 상태에서만 IDLE
                eNewState = m_bHasMouthful ? PLAYER_STATE::MOUTHFUL_IDLE : PLAYER_STATE::IDLE;
            }
        }
    }

    // 상태가 변경되었으면 애니메이션 변경
    if (m_eCurState != eNewState)
    {
        ChangeState(eNewState);
    }
}


void CPlayer::ChangeState(PLAYER_STATE _eState)
{
    m_ePrevState = m_eCurState;
    m_eCurState = _eState;

    // 상태에 맞는 애니메이션 재생
    switch (m_eCurState)
    {
    case PLAYER_STATE::IDLE:
        m_pAnimator->Play(L"IDLE", true);
        break;
    case PLAYER_STATE::WALK:
        m_pAnimator->Play(L"WALK", true);
        break;
    case PLAYER_STATE::RUN:
        m_pAnimator->Play(L"RUN", true);
        break;
    case PLAYER_STATE::JUMP:
        m_pAnimator->Play(L"JUMP", false);
        break;
    case PLAYER_STATE::FALL:
        m_pAnimator->Play(L"FALL", true);
        break;
    case PLAYER_STATE::INHALE_READY:
        m_pAnimator->Play(L"INHALE_READY", false);
        break;
    case PLAYER_STATE::INHALE_1:
        m_pAnimator->Play(L"INHALE_1", true);
        break;
    case PLAYER_STATE::INHALE_2:
        m_pAnimator->Play(L"INHALE_2", true);
        break;
    case PLAYER_STATE::INHALE_HOLD:
        m_pAnimator->Play(L"INHALE_HOLD", true);
        break;
    case PLAYER_STATE::EXHALE:
        m_pAnimator->Play(L"EXHALE", false);
        break;
    case PLAYER_STATE::SWALLOW:
        m_pAnimator->Play(L"SWALLOW", false);
        break;
    case PLAYER_STATE::MOUTHFUL_IDLE:
        m_pAnimator->Play(L"MOUTHFUL_IDLE", true);
        break;
    case PLAYER_STATE::MOUTHFUL_WALK:
        m_pAnimator->Play(L"MOUTHFUL_WALK", true);
        break;
    case PLAYER_STATE::MOUTHFUL_RUN:
        m_pAnimator->Play(L"MOUTHFUL_RUN", true);
        break;
    case PLAYER_STATE::MOUTHFUL_JUMP:
        m_pAnimator->Play(L"MOUTHFUL_JUMP", false);
        break;
    }
}

// === 애니메이션 렌더링 시 방향 적용 ===
void CPlayer::Render(HDC _dc)
{
    // 애니메이션 렌더링
    if (nullptr != m_pAnimator)
    {
        CAnimation* pCurAnim = m_pAnimator->GetCurAnim();
        if (pCurAnim)
        {
            Vec2 vPos = GetPos();

            if (!m_bFacingRight)
            {
                // 왼쪽을 보고 있을 때 - 스프라이트 뒤집기
                Vec2 vRenderPos = CCamera::GetInst()->GetRenderPos(vPos);
                RenderFlippedAnimation(_dc, pCurAnim, vRenderPos);
            }
            else
            {
                // 오른쪽을 보고 있을 때 - 기본 렌더링
                pCurAnim->Render(_dc, vPos);
            }
        }
    }

    // 흡입하기 중일 때 범위 표시
    if (m_bInhaling)
    {
        RenderInhaleEffect(_dc);
    }

    // 충돌체 렌더링 (디버그용)
    if (nullptr != GetCollider())
        GetCollider()->Render(_dc);
}


void CPlayer::RenderFlippedAnimation(HDC _dc, CAnimation* _pAnim, Vec2 _vRenderPos)
{
    if (!_pAnim) return;

    CTexture* pTex = _pAnim->GetTexture();
    if (!pTex) return;

    if (_pAnim->GetCurFrame() >= _pAnim->GetMaxFrame()) return;

    tAnimFrame& frame = _pAnim->GetFrame(_pAnim->GetCurFrame());

    // 1단계: 원본을 메모리 DC에 복사
    HDC hSrcDC = CreateCompatibleDC(_dc);
    HBITMAP hSrcBitmap = CreateCompatibleBitmap(_dc, (int)frame.vSlice.x, (int)frame.vSlice.y);
    HBITMAP hOldSrcBitmap = (HBITMAP)SelectObject(hSrcDC, hSrcBitmap);

    // 원본 스프라이트를 그대로 복사
    BitBlt(hSrcDC,
        0, 0,
        (int)frame.vSlice.x, (int)frame.vSlice.y,
        pTex->GetDC(),
        (int)frame.vLT.x, (int)frame.vLT.y,
        SRCCOPY);

    // 2단계: 뒤집힌 버전을 만들 메모리 DC 생성
    HDC hFlipDC = CreateCompatibleDC(_dc);
    HBITMAP hFlipBitmap = CreateCompatibleBitmap(_dc, (int)frame.vSlice.x, (int)frame.vSlice.y);
    HBITMAP hOldFlipBitmap = (HBITMAP)SelectObject(hFlipDC, hFlipBitmap);

    // 3단계: 좌우 반전 복사 (StretchBlt 사용)
    StretchBlt(hFlipDC,
        (int)frame.vSlice.x - 1, 0,        // 목적지: 오른쪽 끝에서 시작
        -(int)frame.vSlice.x, (int)frame.vSlice.y,  // 음수 폭으로 뒤집기
        hSrcDC,
        0, 0,                              // 소스: 왼쪽부터
        (int)frame.vSlice.x, (int)frame.vSlice.y,
        SRCCOPY);

    // 4단계: 최종 화면에 투명 처리로 그리기
    TransparentBlt(_dc,
        (int)(_vRenderPos.x - frame.vSlice.x / 2.f),
        (int)(_vRenderPos.y - frame.vSlice.y / 2.f),
        (int)frame.vSlice.x, (int)frame.vSlice.y,
        hFlipDC,
        0, 0,
        (int)frame.vSlice.x, (int)frame.vSlice.y,
        RGB(255, 0, 255)); // 마젠타 투명 처리

    // 메모리 정리
    SelectObject(hSrcDC, hOldSrcBitmap);
    DeleteObject(hSrcBitmap);
    DeleteDC(hSrcDC);

    SelectObject(hFlipDC, hOldFlipBitmap);
    DeleteObject(hFlipBitmap);
    DeleteDC(hFlipDC);
}

// 빨아들이기 이펙트 렌더링
void CPlayer::RenderInhaleEffect(HDC _dc)
{
    Vec2 vPlayerPos = GetPos();
    Vec2 vRenderPos = CCamera::GetInst()->GetRenderPos(vPlayerPos);

    // 빨아들이기 방향으로 부채꼴 그리기
    HPEN hPen = CreatePen(PS_SOLID, 2, RGB(100, 200, 255));
    HPEN hOldPen = (HPEN)SelectObject(_dc, hPen);

    // 부채꼴의 시작과 끝 각도 계산
    float fBaseAngle = atan2(m_vInhaleDir.y, m_vInhaleDir.x);
    float fStartAngle = fBaseAngle - 3.14159f / 3.f; // -60도
    float fEndAngle = fBaseAngle + 3.14159f / 3.f;   // +60도

    // 부채꼴 그리기 (간단한 선들로 표현)
    for (int i = 0; i <= 10; ++i)
    {
        float fAngle = fStartAngle + (fEndAngle - fStartAngle) * i / 10.f;
        Vec2 vDir = Vec2(cos(fAngle), sin(fAngle));
        Vec2 vEndPos = vRenderPos + vDir * m_fInhaleRange;

        MoveToEx(_dc, (int)vRenderPos.x, (int)vRenderPos.y, nullptr);
        LineTo(_dc, (int)vEndPos.x, (int)vEndPos.y);
    }

    SelectObject(_dc, hOldPen);
    DeleteObject(hPen);

    // 빨아들이기 파티클 이펙트 (애니메이션으로 표현)
    if (m_fInhaleTime > 0.5f && !m_bPlayingInhaleEffect)
    {
        // 여기서 INHALE_EFFECT 애니메이션을 별도로 렌더링 가능
        // 또는 파티클 시스템 구현
    }
}

void CPlayer::OnCollisionEnter(CCollider* _pOther)
{
    CObject* pOtherObj = _pOther->GetOwner();

    // 타일과의 충돌 처리
    CTile* pTile = dynamic_cast<CTile*>(pOtherObj);
    if (pTile && pTile->IsSolid())
    {
        Vec2 vPlayerPos = GetPos();
        Vec2 vTilePos = pTile->GetPos();
        Vec2 vVelocity = m_pRigidBody->GetVelocity();

        // 충돌체 크기 가져오기 (실제 충돌체 크기 사용)
        Vec2 vPlayerColliderScale = GetCollider()->GetScale();
        Vec2 vTileColliderScale = pTile->GetCollider()->GetScale();

        // 플레이어가 타일 위에서 아래로 떨어지고 있을 때만 착지
        if (vVelocity.y >= 0.f && vPlayerPos.y < vTilePos.y)
        {
            // 플레이어 충돌체의 바닥면과 타일 충돌체의 윗면이 맞닿도록 배치
            float tileTop = vTilePos.y - vTileColliderScale.y / 2.f;
            float playerHalfHeight = vPlayerColliderScale.y / 2.f;

            // 플레이어 중심을 타일 윗면에서 플레이어 충돌체 높이의 절반만큼 위에 배치
            // 이렇게 하면 플레이어 충돌체 바닥이 타일 충돌체 윗면과 정확히 맞닿음
            float newY = tileTop - playerHalfHeight;

            vPlayerPos.y = newY;
            SetPos(vPlayerPos);

            // 수직 속도만 0으로 설정
            m_pRigidBody->SetVelocityY(0.f);
            m_pRigidBody->SetGround(true);

            SetWindowText(CCore::GetInst()->GetMainHwnd(), L"타일 위에 착지! (충돌체 맞닿음)");
        }
    }

    // 몬스터와의 충돌 처리 개선
    CMonster* pMonster = dynamic_cast<CMonster*>(pOtherObj);
    if (pMonster)
    {
        // 빨아들이기 중이고 범위 내에 있다면 자동으로 흡수
        if (m_bInhaling)
        {
            Vec2 vDiff = GetPos() - pMonster->GetPos();
            if (vDiff.Length() < 60.f)
            {
                SwallowTarget(pMonster);
                return;
            }
        }

        // 머금은 상태가 아니고 빨아들이기 중이 아니라면 데미지
        if (!m_bHasMouthful && !m_bInhaling)
        {
            CCamera::GetInst()->CameraShake(0.3f, 10.f);
            SetWindowText(CCore::GetInst()->GetMainHwnd(), L"몬스터와 충돌! 데미지!");

            // TODO: 실제 데미지 시스템 구현 시 여기에 추가
            // TakeDamage(1);
        }
    }
}

void CPlayer::OnCollision(CCollider* _pOther)
{
    // 지속적인 충돌 처리 - 충돌체가 계속 맞닿아 있어야 함
    CObject* pOtherObj = _pOther->GetOwner();

    CTile* pTile = dynamic_cast<CTile*>(pOtherObj);
    if (pTile && pTile->IsSolid())
    {
        Vec2 vPlayerPos = GetPos();
        Vec2 vTilePos = pTile->GetPos();
        Vec2 vVelocity = m_pRigidBody->GetVelocity();

        // 충돌체 크기
        Vec2 vPlayerColliderScale = GetCollider()->GetScale();
        Vec2 vTileColliderScale = pTile->GetCollider()->GetScale();

        // 플레이어가 타일 위에 올바르게 서 있는지 확인
        float tileTop = vTilePos.y - vTileColliderScale.y / 2.f;
        float playerBottom = vPlayerPos.y + vPlayerColliderScale.y / 2.f;

        // 플레이어 바닥과 타일 윗면이 거의 맞닿아 있고, 플레이어가 위에 있다면
        if (abs(playerBottom - tileTop) < 8.f && vPlayerPos.y < vTilePos.y)
        {
            // 미세한 위치 조정 (중력에 의한 약간의 침투 보정)
            if (playerBottom > tileTop + 2.f)  // 2픽셀 이상 침투했다면
            {
                float correctedY = tileTop - vPlayerColliderScale.y / 2.f;
                vPlayerPos.y = correctedY;
                SetPos(vPlayerPos);
            }

            m_pRigidBody->SetGround(true);

            // 아래로 떨어지는 속도가 있다면 제거
            if (vVelocity.y > 0.f)
            {
                m_pRigidBody->SetVelocityY(0.f);
            }
        }
    }
}

void CPlayer::OnCollisionExit(CCollider* _pOther)
{
    CObject* pOtherObj = _pOther->GetOwner();

    CTile* pTile = dynamic_cast<CTile*>(pOtherObj);
    if (pTile && pTile->IsSolid())
    {
        // 점프나 이동으로 타일에서 벗어날 때만 Ground 해제
        Vec2 vVelocity = m_pRigidBody->GetVelocity();

        // 위쪽으로 빠르게 이동 중이거나 (점프), 수평으로 이동해서 벗어났을 때
        if (vVelocity.y < -30.f || abs(vVelocity.x) > 50.f)
        {
            m_pRigidBody->SetGround(false);
        }

        SetWindowText(CCore::GetInst()->GetMainHwnd(), L"타일에서 벗어남");
    }
}