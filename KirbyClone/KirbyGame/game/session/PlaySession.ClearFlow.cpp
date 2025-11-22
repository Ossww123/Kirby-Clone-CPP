//
// Responsibility: Stage clear sequence (emblem → autopilot → dance → fade → save+hub → fade-in).
// Non-Goals:      Hub blocker removal (Step 3), audio cues, complex autopilot pathing.
// Call-Context:   Main thread; ticked from PlaySession::FixedUpdate().
//
#include "game/session/PlaySession.h"
#include "protocol/SaveSchema.h"
#include "game/data/StagePath.h"   // StageJsonPathFromId
#include <cwchar> 

#ifndef DBGLOG
#include <windows.h>
inline void DBGLOG ( const wchar_t* msg ) {
    ::OutputDebugStringW ( msg );
    ::OutputDebugStringW ( L"\n" );
}
#endif

namespace {
    // fallback: derive "door_mX" from "t1/s1/mX"
    std::string DeriveDoorKeyFromStageId ( const std::string& id ) {
        // find last '/' and take tail
        const auto p = id.find_last_of ( '/' );
        if ( p == std::string::npos ) return "door_default";
        return std::string ( "door_" ) + id.substr ( p + 1 ); // m1->door_m1
    }

    // "t1/s1/m4" -> "t1/s1"
    std::string MapIdToStageId ( const std::string& mapId ) {
        const auto p = mapId.find_last_of ( '/' );
        if ( p == std::string::npos ) return mapId;
        return mapId.substr ( 0 , p );
    }
}

namespace game {
    const wchar_t* PlaySession::ClearStateName ( ClearState st )
    {
        switch ( st ) {
        case ClearState::Idle:       return L"Idle";
        case ClearState::Emblem:     return L"Emblem";
        case ClearState::AutoPilot:  return L"AutoPilot";
        case ClearState::Dance:      return L"Dance";
        case ClearState::FadeOut:    return L"FadeOut";
        case ClearState::SaveAndHub: return L"SaveAndHub";
        case ClearState::FadeIn:     return L"FadeIn";
        default:                     return L"(unknown)";
        }
    }

    bool PlaySession::IsClearSequenceActive ( ) const {
        return m_clear.st != ClearState::Idle;
    }

    void PlaySession::BeginClearSequence ( ) {
        if ( IsClearSequenceActive ( ) ) return;

        // stage id / spawn key
        m_clear = {};
        m_clear.st = ClearState::Emblem;
        m_clear.t = 0.f;
        const std::string mapId = m_stageId;
        m_clear.stageId = MapIdToStageId ( mapId );

        m_clear.hubSpawnKey = DeriveDoorKeyFromStageId ( m_clear.stageId );

        // (stop directing for gaining Emblem)

        // --- Debug ---
        {
            wchar_t buf[ 256 ];
            std::swprintf (
                buf , _countof ( buf ) ,
                L"[Clear] BeginClearSequence  mapId=%hs  stageId=%hs  hubSpawn=%hs" ,
                mapId.c_str ( ) ,
                m_clear.stageId.c_str ( ) ,
                m_clear.hubSpawnKey.c_str ( )
            );
            DBGLOG ( buf );
        }
    }

    void PlaySession::updateClearFlow ( double dt ) {
        if ( !IsClearSequenceActive ( ) ) return;

        m_clear.t += static_cast< float >( dt );

        switch ( m_clear.st ) {
        case ClearState::Emblem:
            if ( m_clear.t >= m_clear.tEmblem ) {
                m_clear.t = 0.f;

                // --- Debug ---
                {
                    wchar_t buf[ 128 ];
                    std::swprintf (
                        buf , _countof ( buf ) ,
                        L"[Clear] Emblem done -> AutoPilot  stageId=%hs" ,
                        m_clear.stageId.c_str ( )
                    );
                    DBGLOG ( buf );
                }

                m_clear.st = ClearState::AutoPilot;
                // AutoPilot: 간단 정렬(현 단계에선 이동 없이 포즈 유지)
                // (PlayerFSM 오토파일럿 확장은 별도 단계에서 추가)
            }
            break;

        case ClearState::AutoPilot:
            if ( m_clear.t >= m_clear.tAuto ) {
                m_clear.t = 0.f;

                // --- Debug ---
                {
                    wchar_t buf[ 128 ];
                    std::swprintf (
                        buf , _countof ( buf ) ,
                        L"[Clear] AutoPilot done -> Dance"
                    );
                    DBGLOG ( buf );
                }

                m_clear.st = ClearState::Dance;
                // 댄스 시작(오버레이 전환)
                m_PlayerFSM.BeginDance ( );
            }
            break;

        case ClearState::Dance:
            if ( m_clear.t >= m_clear.tDance ) {
                m_clear.t = 0.f;

                // --- Debug ---
                DBGLOG ( L"[Clear] Dance done -> FadeOut" );

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

                    // --- Debug (before/after cleared flag) ---
                    {
                        wchar_t buf[ 256 ];
                        std::swprintf (
                            buf , _countof ( buf ) ,
                            L"[Clear] FadeOut done -> SaveAndHub  stageId=%hs  hubSpawn=%hs" ,
                            m_clear.stageId.c_str ( ) ,
                            m_clear.hubSpawnKey.c_str ( )
                        );
                        DBGLOG ( buf );
                    }

                    protocol::SetCleared ( sd , m_clear.stageId );
                    sd.lastHub = "t1/hub";
                    sd.lastStage = "t1/hub";
                    sd.lastSpawn = m_clear.hubSpawnKey; // 허브에서 이 스폰 키를 사용
                    ( void ) m_Session->SaveToDisk ( );
                }

                // === Hub Transition ===
                const std::string hubJson = game::StageJsonPathFromId ( "t1/hub" );
                StartTransitionTo ( hubJson ,
                                    /*out*/0.f ,
                                    /*in*/m_clear.tFade ,
                                    /*spawnOverride*/ m_clear.hubSpawnKey.c_str ( ) );

                // --- Debug ---
                {
                    wchar_t buf[ 256 ];
                    std::swprintf (
                        buf , _countof ( buf ) ,
                        L"[Clear] StartTransitionTo hub  json=%hs  spawn=%hs" ,
                        hubJson.c_str ( ) ,
                        m_clear.hubSpawnKey.c_str ( )
                    );
                    DBGLOG ( buf );
                }

                m_clear.st = ClearState::FadeIn;
            }
            break;

        case ClearState::FadeIn:
            // 페이드 인 완료 + 전환 FSM 아이들 시 종료
            if ( !IsFading ( ) && m_trans.state == Transition::Idle ) {
                // 댄스 오버레이 종료(허브에서는 일반 상태)
                m_PlayerFSM.EndDance ( );

                // --- Debug ---
                {
                    wchar_t buf[ 128 ];
                    std::swprintf (
                        buf , _countof ( buf ) ,
                        L"[Clear] FadeIn done -> Idle  stageId=%hs" ,
                        m_clear.stageId.c_str ( )
                    );
                    DBGLOG ( buf );
                }

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
