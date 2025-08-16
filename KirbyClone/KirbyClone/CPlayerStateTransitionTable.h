#pragma once
#include "CPlayerInputManager.h"

// 전방 선언
class CPlayer;

class CPlayerStateTransitionTable
{
public:
    using InputFlags = CPlayerInputManager::InputFlags;
    using ConditionFunc = std::function<bool(CPlayer*)>;

    // 전환 규칙 구조체
    struct TransitionRule
    {
        PLAYER_STATE fromState;         // 현재 상태
        InputFlags requiredInputs;      // 필요한 입력 (이 입력들이 모두 있어야 함)
        InputFlags blockedInputs;       // 금지된 입력 (이 입력들이 있으면 안됨)
        ConditionFunc condition;        // 추가 조건 함수 (nullptr이면 항상 true)
        PLAYER_STATE toState;           // 목표 상태
        int priority;                   // 우선순위 (높을수록 먼저 체크)

        TransitionRule()
            : fromState(PLAYER_STATE::END)
            , requiredInputs(0)
            , blockedInputs(0)
            , condition(nullptr)
            , toState(PLAYER_STATE::END)
            , priority(0)
        {}

        TransitionRule(PLAYER_STATE from, InputFlags inputs, PLAYER_STATE to,
            ConditionFunc cond = nullptr, int prio = 0, InputFlags blocked = 0)
            : fromState(from)
            , requiredInputs(inputs)
            , blockedInputs(blocked)
            , condition(cond)
            , toState(to)
            , priority(prio)
        {}
    };

public:
    CPlayerStateTransitionTable();
    ~CPlayerStateTransitionTable();

public:
    // === 전환 규칙 관리 ===
    void AddTransition(PLAYER_STATE from, InputFlags inputs, PLAYER_STATE to,
        ConditionFunc condition = nullptr, int priority = 0,
        InputFlags blockedInputs = 0);

    void RemoveTransition(PLAYER_STATE from, PLAYER_STATE to);
    void ClearAllTransitions();

public:
    // === 상태 전환 로직 ===
    PLAYER_STATE GetNextState(PLAYER_STATE currentState, InputFlags currentInput,
        CPlayer* player);

    // === 디버깅/개발 지원 ===
    std::vector<TransitionRule> GetPossibleTransitions(PLAYER_STATE currentState) const;
    bool HasTransition(PLAYER_STATE from, PLAYER_STATE to) const;
    int GetTransitionCount() const { return (int)m_transitions.size(); }

public:
    // === 초기화 (기본 전환 규칙들 설정) ===
    void InitializeDefaultTransitions();

private:
    // === 내부 로직 ===
    bool CheckTransitionCondition(const TransitionRule& rule, InputFlags currentInput,
        CPlayer* player) const;
    void SortTransitionsByPriority();

    // === 전환 규칙 검증 ===
    bool IsValidTransitionRule(const TransitionRule& rule) const;

private:
    // === 멤버 변수 ===
    std::vector<TransitionRule> m_transitions;  // 모든 전환 규칙들
    bool m_bSorted;                             // 우선순위 정렬 여부
};