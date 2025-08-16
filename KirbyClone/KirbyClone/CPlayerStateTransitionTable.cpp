#include "pch.h"
#include "CPlayerStateTransitionTable.h"
#include "CPlayer.h"
#include "CRigidBody.h"
#include "CPlayerMovement.h"
#include "CPlayerStateMachine.h"

CPlayerStateTransitionTable::CPlayerStateTransitionTable()
    : m_bSorted(false)
{
}

CPlayerStateTransitionTable::~CPlayerStateTransitionTable()
{
}

// === 전환 규칙 관리 ===

void CPlayerStateTransitionTable::AddTransition(PLAYER_STATE from, InputFlags inputs,
    PLAYER_STATE to, ConditionFunc condition,
    int priority, InputFlags blockedInputs)
{
    TransitionRule rule(from, inputs, to, condition, priority, blockedInputs);

    // 유효성 검사
    if (!IsValidTransitionRule(rule))
    {
        char debugMsg[256];
        sprintf_s(debugMsg, "Invalid transition rule: %d -> %d\n", (int)from, (int)to);
        OutputDebugStringA(debugMsg);
        return;
    }

    m_transitions.push_back(rule);
    m_bSorted = false;  // 재정렬 필요
}

void CPlayerStateTransitionTable::RemoveTransition(PLAYER_STATE from, PLAYER_STATE to)
{
    auto it = std::remove_if(m_transitions.begin(), m_transitions.end(),
        [from, to](const TransitionRule& rule) {
            return rule.fromState == from && rule.toState == to;
        });

    m_transitions.erase(it, m_transitions.end());
}

void CPlayerStateTransitionTable::ClearAllTransitions()
{
    m_transitions.clear();
    m_bSorted = true;  // 빈 상태는 정렬된 것으로 간주
}

// === 상태 전환 로직 ===

PLAYER_STATE CPlayerStateTransitionTable::GetNextState(PLAYER_STATE currentState,
    InputFlags currentInput,
    CPlayer* player)
{
    if (!player)
        return currentState;

    // 우선순위 정렬이 안되어 있으면 정렬
    if (!m_bSorted)
    {
        SortTransitionsByPriority();
    }

    // 현재 상태에서 시작하는 전환 규칙들을 우선순위 순으로 체크
    for (const auto& rule : m_transitions)
    {
        if (rule.fromState == currentState)
        {
            if (CheckTransitionCondition(rule, currentInput, player))
            {
                // 조건을 만족하는 첫 번째 규칙 적용
                char debugMsg[256];
                sprintf_s(debugMsg, "State transition: %d -> %d\n", (int)currentState, (int)rule.toState);
                OutputDebugStringA(debugMsg);

                return rule.toState;
            }
        }
    }

    // 적용 가능한 전환이 없으면 현재 상태 유지
    return currentState;
}

// === 디버깅/개발 지원 ===

std::vector<CPlayerStateTransitionTable::TransitionRule>
CPlayerStateTransitionTable::GetPossibleTransitions(PLAYER_STATE currentState) const
{
    std::vector<TransitionRule> result;

    for (const auto& rule : m_transitions)
    {
        if (rule.fromState == currentState)
        {
            result.push_back(rule);
        }
    }

    return result;
}

bool CPlayerStateTransitionTable::HasTransition(PLAYER_STATE from, PLAYER_STATE to) const
{
    for (const auto& rule : m_transitions)
    {
        if (rule.fromState == from && rule.toState == to)
        {
            return true;
        }
    }
    return false;
}

// === 초기화 (기본 전환 규칙들 설정) ===

void CPlayerStateTransitionTable::InitializeDefaultTransitions()
{
    using INPUT = CPlayerInputManager::INPUT_TYPE;

    // === 기본 이동 관련 전환 ===

    // IDLE -> WALK (좌우 이동)
    AddTransition(PLAYER_STATE::IDLE,
        (uint32_t)INPUT::MOVE_LEFT | (uint32_t)INPUT::MOVE_RIGHT,
        PLAYER_STATE::WALK,
        [](CPlayer* p) { return p->GetRigidBody() && p->GetRigidBody()->IsGround(); },
        100);

    // WALK -> IDLE (이동 입력 없음)
    AddTransition(PLAYER_STATE::WALK,
        0,  // 특정 입력 없음
        PLAYER_STATE::IDLE,
        [](CPlayer* p) {
            if (!p->GetMovement()) return false;
            return !p->GetMovement()->IsActuallyMoving() &&
                !p->GetMovement()->IsDecelerating();
        },
        50);

    // === 점프 관련 전환 ===

    // IDLE/WALK -> JUMP (점프 입력)
    AddTransition(PLAYER_STATE::IDLE,
        (uint32_t)INPUT::JUMP_TAP,
        PLAYER_STATE::JUMP,
        [](CPlayer* p) { return p->GetRigidBody() && p->GetRigidBody()->IsGround(); },
        200);

    AddTransition(PLAYER_STATE::WALK,
        (uint32_t)INPUT::JUMP_TAP,
        PLAYER_STATE::JUMP,
        [](CPlayer* p) { return p->GetRigidBody() && p->GetRigidBody()->IsGround(); },
        200);

    // JUMP -> FALL (하강 시작)
    AddTransition(PLAYER_STATE::JUMP,
        0,  // 특정 입력 없음
        PLAYER_STATE::FALL,
        [](CPlayer* p) {
            if (!p->GetRigidBody()) return false;
            return p->GetRigidBody()->GetVelocity().y >= 0.f;
        },
        300);

    // FALL -> IDLE (착지)
    AddTransition(PLAYER_STATE::FALL,
        0,  // 특정 입력 없음
        PLAYER_STATE::IDLE,
        [](CPlayer* p) { return p->GetRigidBody() && p->GetRigidBody()->IsGround(); },
        300);

    // === 크라우치 관련 전환 ===

    // IDLE -> CROUCH (DOWN 키)
    AddTransition(PLAYER_STATE::IDLE,
        (uint32_t)INPUT::MOVE_DOWN,
        PLAYER_STATE::CROUCH,
        [](CPlayer* p) {
            return p->GetRigidBody() && p->GetRigidBody()->IsGround() &&
                !p->HasMouthful();
        },
        150);

    // CROUCH -> IDLE (DOWN 키 해제)
    AddTransition(PLAYER_STATE::CROUCH,
        0,  // 특정 입력 없음
        PLAYER_STATE::IDLE,
        [](CPlayer* p) {
            if (!p->GetMovement()) return false;
            CPlayerInputManager* pInputMgr = p->GetStateMachine()->GetInputManager();
            return !pInputMgr->IsMovingDown();
        },
        100);

    // CROUCH -> SLIDE (점프 또는 액션 키)
    AddTransition(PLAYER_STATE::CROUCH,
        (uint32_t)INPUT::JUMP_TAP | (uint32_t)INPUT::ACTION_TAP,
        PLAYER_STATE::SLIDE,
        [](CPlayer* p) { return p->GetRigidBody() && p->GetRigidBody()->IsGround(); },
        250);

    // === 흡입 관련 전환 ===

    // IDLE -> INHALE_READY (액션 키)
    AddTransition(PLAYER_STATE::IDLE,
        (uint32_t)INPUT::ACTION_TAP,
        PLAYER_STATE::INHALE_READY,
        [](CPlayer* p) { return !p->HasMouthful(); },
        180);

    // === 더블탭 RUN 전환 ===

    // IDLE -> RUN (더블탭)
    AddTransition(PLAYER_STATE::IDLE,
        (uint32_t)INPUT::DOUBLE_TAP_LEFT | (uint32_t)INPUT::DOUBLE_TAP_RIGHT,
        PLAYER_STATE::RUN,
        [](CPlayer* p) { return p->GetRigidBody() && p->GetRigidBody()->IsGround(); },
        120);

    // RUN -> WALK (더블탭 아닌 일반 이동)
    AddTransition(PLAYER_STATE::RUN,
        (uint32_t)INPUT::MOVE_LEFT | (uint32_t)INPUT::MOVE_RIGHT,
        PLAYER_STATE::WALK,
        [](CPlayer* p) {
            if (!p->GetMovement()) return false;
            return !p->GetMovement()->IsRunMode();
        },
        80);

    // 정렬 플래그 설정
    m_bSorted = false;
}

// === 내부 로직 ===

bool CPlayerStateTransitionTable::CheckTransitionCondition(const TransitionRule& rule,
    InputFlags currentInput,
    CPlayer* player) const
{
    // 1. 필요한 입력이 있는지 체크
    if (rule.requiredInputs != 0)
    {
        // OR 조건: 필요한 입력 중 하나라도 있으면 됨
        if ((currentInput & rule.requiredInputs) == 0)
        {
            return false;
        }
    }

    // 2. 금지된 입력이 없는지 체크
    if (rule.blockedInputs != 0)
    {
        if ((currentInput & rule.blockedInputs) != 0)
        {
            return false;
        }
    }

    // 3. 추가 조건 함수 체크
    if (rule.condition)
    {
        if (!rule.condition(player))
        {
            return false;
        }
    }

    return true;
}

void CPlayerStateTransitionTable::SortTransitionsByPriority()
{
    std::sort(m_transitions.begin(), m_transitions.end(),
        [](const TransitionRule& a, const TransitionRule& b) {
            // 우선순위가 높은 것부터 (내림차순)
            return a.priority > b.priority;
        });

    m_bSorted = true;
}

// === 전환 규칙 검증 ===

bool CPlayerStateTransitionTable::IsValidTransitionRule(const TransitionRule& rule) const
{
    // 상태가 유효한지 체크
    if (rule.fromState == PLAYER_STATE::END || rule.toState == PLAYER_STATE::END)
    {
        return false;
    }

    // 자기 자신으로의 전환은 일반적으로 의미 없음 (하지만 허용)
    if (rule.fromState == rule.toState)
    {
        char debugMsg[128];
        sprintf_s(debugMsg, "Warning: Self-transition detected: %d -> %d\n",
            (int)rule.fromState, (int)rule.toState);
        OutputDebugStringA(debugMsg);
    }

    return true;
}