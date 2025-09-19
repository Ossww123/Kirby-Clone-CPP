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
        sprintf_s(debugMsg, "Invalid transition rule: %s -> %s\n", 
            PlayerStateToString(from), PlayerStateToString(to));
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
                sprintf_s(debugMsg, "State transition: %s -> %s\n", 
                    PlayerStateToString(currentState), PlayerStateToString(rule.toState));
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

// InitializeDefaultTransitions 제거됨 - CPlayerStateMachine에서 모든 전환 관리

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
        sprintf_s(debugMsg, "Warning: Self-transition detected: %s -> %s\n",
            PlayerStateToString(rule.fromState), PlayerStateToString(rule.toState));
        OutputDebugStringA(debugMsg);
    }

    return true;
}

// === 디버깅용 문자열 변환 ===

const char* CPlayerStateTransitionTable::PlayerStateToString(PLAYER_STATE state) const
{
    switch (state)
    {
    case PLAYER_STATE::IDLE:              return "IDLE";
    case PLAYER_STATE::WALK:              return "WALK";
    case PLAYER_STATE::RUN:               return "RUN";
    case PLAYER_STATE::JUMP:              return "JUMP";
    case PLAYER_STATE::FALL0:             return "FALL0";
    case PLAYER_STATE::FALL1:             return "FALL1";
    case PLAYER_STATE::FALL2:             return "FALL2";
    case PLAYER_STATE::BOUNCE:            return "BOUNCE";
    case PLAYER_STATE::CROUCH:            return "CROUCH";
    case PLAYER_STATE::SLIDE:             return "SLIDE";
    case PLAYER_STATE::SLIDE_KICK_RECOIL: return "SLIDE_KICK_RECOIL";
    case PLAYER_STATE::DAMAGE:            return "DAMAGE";
    case PLAYER_STATE::HOVER:             return "HOVER";
    case PLAYER_STATE::HOVER_EXHALE:      return "HOVER_EXHALE";
    case PLAYER_STATE::INHALE:            return "INHALE";
    case PLAYER_STATE::INHALE_KEEP:       return "INHALE_KEEP";
    case PLAYER_STATE::INHALE_SUCCESS:    return "INHALE_SUCCESS";
    case PLAYER_STATE::EXHALE:            return "EXHALE";
    case PLAYER_STATE::SWALLOW:           return "SWALLOW";
    case PLAYER_STATE::MOUTHFUL_IDLE:     return "MOUTHFUL_IDLE";
    case PLAYER_STATE::MOUTHFUL_WALK:     return "MOUTHFUL_WALK";
    case PLAYER_STATE::MOUTHFUL_RUN:      return "MOUTHFUL_RUN";
    case PLAYER_STATE::MOUTHFUL_JUMP:     return "MOUTHFUL_JUMP";
    case PLAYER_STATE::MOUTHFUL_FALL:     return "MOUTHFUL_FALL";
    case PLAYER_STATE::MOUTHFUL_DAMAGE:   return "MOUTHFUL_DAMAGE";
    case PLAYER_STATE::ATTACK:            return "ATTACK";
    case PLAYER_STATE::ATTACK_HOLD:       return "ATTACK_HOLD";
    case PLAYER_STATE::END:               return "END";
    default:                              return "UNKNOWN";
    }
}