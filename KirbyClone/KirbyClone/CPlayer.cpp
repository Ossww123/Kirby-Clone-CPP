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
    , m_fSpeed(150.f)           // 기본 걷기 속도
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
    m_pRigidBody->SetFriction(10.f);
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
    m_pAnimator->CreateAnimation(L"IDLE", pKirbyTex,
        Vec2(BORDER, BORDER),           // 시작 위치 (테두리 제외)
        Vec2(IMAGE_SIZE, IMAGE_SIZE),   // 프레임 크기
        Vec2(SPRITE_SIZE, 0),           // 다음 프레임까지의 간격
        0.15f,                          // 프레임 지속시간
        10,                             // 프레임 개수
        true);                          // 루프

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

    // 머금은 것을 뱉기 (Z키)
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

    // 빨아들이기 시작 (X키)
    if (KEY_TAP(KEY::X))
    {
        if (!m_bInhaling && !m_bHasMouthful)
        {
            StartInhale();
        }
    }

    // 빨아들이기 유지 중
    if (KEY_HOLD(KEY::X) && m_bInhaling)
    {
        UpdateInhale();
    }

    // 빨아들이기 해제 (X키를 뗄 때)
    if (KEY_AWAY(KEY::X))
    {
        if (m_bInhaling)
        {
            StopInhale();
        }
    }

    // 빨아들이기 중일 때는 이동 제한
    if (m_bInhaling)
    {
        m_pRigidBody->SetVelocityX(0.f);
        return;
    }

    // 이동 방향에 따라 빨아들이기 방향 업데이트
    if (KEY_HOLD(KEY::LEFT))
    {
        m_vInhaleDir = Vec2(-1.f, 0.f);
        float fCurrentSpeed = m_bRunMode ? m_fRunSpeed : m_fSpeed;
        if (m_bHasMouthful) fCurrentSpeed *= 0.7f;
        m_pRigidBody->SetVelocityX(-fCurrentSpeed);
    }
    else if (KEY_HOLD(KEY::RIGHT))
    {
        m_vInhaleDir = Vec2(1.f, 0.f);
        float fCurrentSpeed = m_bRunMode ? m_fRunSpeed : m_fSpeed;
        if (m_bHasMouthful) fCurrentSpeed *= 0.7f;
        m_pRigidBody->SetVelocityX(fCurrentSpeed);
    }
    else
    {
        if (m_pRigidBody->IsGround())
        {
            m_pRigidBody->SetVelocityX(0.f);
        }
    }

    // 점프
    if (KEY_TAP(KEY::SPACE) && m_pRigidBody->IsGround())
    {
        m_pRigidBody->SetGround(false);
        m_pRigidBody->SetVelocityY(-m_fJumpPower);
    }
}

// 빨아들이기 시작
void CPlayer::StartInhale()
{
    m_bInhaling = true;
    m_fInhaleTime = 0.f;
    m_vecInhaleTargets.clear();
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

    // 빨아들이기 상태 처리
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
    // 뱉기 애니메이션이 끝났는지 확인
    else if (m_eCurState == PLAYER_STATE::EXHALE)
    {
        if (m_pAnimator->GetCurAnim() && m_pAnimator->GetCurAnim()->IsFinish())
        {
            eNewState = PLAYER_STATE::IDLE;
        }
        else
        {
            eNewState = PLAYER_STATE::EXHALE; // 계속 뱉기 상태 유지
        }
    }
    // 삼키기 애니메이션이 끝났는지 확인
    else if (m_eCurState == PLAYER_STATE::SWALLOW)
    {
        if (m_pAnimator->GetCurAnim() && m_pAnimator->GetCurAnim()->IsFinish())
        {
            m_bHasMouthful = true;
            eNewState = PLAYER_STATE::MOUTHFUL_IDLE;
        }
        else
        {
            eNewState = PLAYER_STATE::SWALLOW; // 계속 삼키기 상태 유지
        }
    }
    // 일반적인 상태 판정
    else
    {
        // 공중에 있는 경우
        if (!m_pRigidBody->IsGround())
        {
            if (vVelocity.y < -50.f)  // 위로 올라가는 중
            {
                eNewState = m_bHasMouthful ? PLAYER_STATE::MOUTHFUL_JUMP : PLAYER_STATE::JUMP;
            }
            else  // 떨어지는 중
            {
                eNewState = PLAYER_STATE::FALL;
            }
        }
        // 땅에 있는 경우
        else
        {
            float fSpeedThreshold = 10.f;

            if (abs(vVelocity.x) > fSpeedThreshold)  // 이동 중
            {
                if (m_bHasMouthful)
                {
                    eNewState = m_bRunMode ? PLAYER_STATE::MOUTHFUL_RUN : PLAYER_STATE::MOUTHFUL_WALK;
                }
                else
                {
                    eNewState = m_bRunMode ? PLAYER_STATE::RUN : PLAYER_STATE::WALK;
                }
            }
            else  // 정지
            {
                eNewState = m_bHasMouthful ? PLAYER_STATE::MOUTHFUL_IDLE : PLAYER_STATE::IDLE;
            }
        }
    }

    // 상태가 변경되었다면 애니메이션 변경
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

void CPlayer::Render(HDC _dc)
{
    // 애니메이션 렌더링
    if (nullptr != m_pAnimator)
    {
        m_pAnimator->Render(_dc);
    }

    // 빨아들이기 중일 때 범위 표시 (디버그용)
    if (m_bInhaling)
    {
        RenderInhaleEffect(_dc);
    }

    // 충돌체가 있으면 충돌체도 렌더링
    if (nullptr != GetCollider())
        GetCollider()->Render(_dc);
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