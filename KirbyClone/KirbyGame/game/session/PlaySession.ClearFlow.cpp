//
// Responsibility: Stage clear sequence (emblem → autopilot → dance → fade → save+hub → fade-in).
// Non-Goals:      Hub blocker removal (Step 3), audio cues, complex autopilot pathing.
// Call-Context:   Main thread; ticked from PlaySession::FixedUpdate().
//
#include "game/session/PlaySession.h"
#include "protocol/SaveSchema.h"

namespace {
    // fallback: derive "door_mX" from "t1/s1/mX"
    std::string DeriveDoorKeyFromStageId ( const std::string& id ) {
        // find last '/' and take tail
        const auto p = id.find_last_of ( '/' );
        if ( p == std::string::npos ) return "door_default";
        return std::string ( "door_" ) + id.substr ( p + 1 ); // m1->door_m1
    }
}

namespace game {

    bool PlaySession::IsClearSequenceActive ( ) const {
        return m_clear.st != ClearState::Idle;
    }

    void PlaySession::BeginClearSequence ( ) {
        if ( IsClearSequenceActive ( ) ) return;

        // stage id / spawn key
        m_clear = {};
        m_clear.st = ClearState::Emblem;
        m_clear.t = 0.f;
        m_clear.stageId = m_stageId;

        m_clear.hubSpawnKey = DeriveDoorKeyFromStageId ( m_clear.stageId );

        // (stop directing for gaining Emblem)
    }

    void PlaySession::updateClearFlow ( double dt ) {
        if ( !IsClearSequenceActive ( ) ) return;

        m_clear.t += static_cast< float >( dt );

        switch ( m_clear.st ) {
        case ClearState::Emblem:
            if ( m_clear.t >= m_clear.tEmblem ) {
                m_clear.t = 0.f;
                m_clear.st = ClearState::AutoPilot;
                // AutoPilot: 간단 정렬(현 단계에선 이동 없이 포즈 유지)
                // (PlayerFSM 오토파일럿 확장은 별도 단계에서 추가)
            }
            break;

        case ClearState::AutoPilot:
            if ( m_clear.t >= m_clear.tAuto ) {
                m_clear.t = 0.f;
                m_clear.st = ClearState::Dance;
                // 댄스 시작(오버레이 전환)
                m_PlayerFSM.BeginDance ( );
            }
            break;

        case ClearState::Dance:
            if ( m_clear.t >= m_clear.tDance ) {
                m_clear.t = 0.f;
                m_clear.st = ClearState::FadeOut;
                StartFadeOut ( m_clear.tFade );
            }
            break;

        case ClearState::FadeOut:
            if ( !IsFading ( ) ) {
                m_clear.st = ClearState::SaveAndHub;

                // === Save ===
                if ( m_Session ) {
                    auto& sd = m_Session->MutData ( );
                    protocol::SetCleared ( sd , m_clear.stageId );
                    sd.lastHub = "t1/hub";
                    sd.lastStage = "t1/hub";
                    sd.lastSpawn = m_clear.hubSpawnKey; // 허브에서 이 스폰 키를 사용
                    ( void ) m_Session->SaveToDisk ( );
                }

                // === Hub Transition ===
                // out=0 to avoid double fade (we already faded out)
                StartTransitionTo ( "t1/hub" , /*out*/0.f , /*in*/m_clear.tFade , /*spawnOverride*/ m_clear.hubSpawnKey.c_str ( ) );
                m_clear.st = ClearState::FadeIn;
            }
            break;

        case ClearState::FadeIn:
            // 페이드 인 완료 + 전환 FSM 아이들 시 종료
            if ( !IsFading ( ) && m_trans.state == Transition::Idle ) {
                // 댄스 오버레이 종료(허브에서는 일반 상태)
                m_PlayerFSM.EndDance ( );

                m_clear.t = 0.f;
                m_clear.st = ClearState::Idle;
            }
            break;

        case ClearState::SaveAndHub:
        case ClearState::Idle:
        default:
            break;
        }
    }

} // namespace game
