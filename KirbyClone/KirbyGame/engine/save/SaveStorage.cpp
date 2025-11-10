//
// Responsibility: Implementation of k=v text I/O for SaveData with minimal, robust parsing.
// Non-Goals:      JSON/YAML support, streaming/async I/O, logging framework integration.
// Call-Context:   Main thread; creates "saves" directory on Save().
//
#include "engine/save/SaveStorage.h"
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <cctype>

namespace fs = std::filesystem;

namespace {
    // ---- slot clamp ----
    inline int clamp_slot ( int s ) { return ( s < 1 ) ? 1 : ( s > 3 ) ? 3 : s; }

    // ---- trim helpers ----
    inline void rtrim_inplace ( std::string& s ) {
        while ( !s.empty ( ) && ( s.back ( ) == '\r' || s.back ( ) == '\n' || s.back ( ) == ' ' || s.back ( ) == '\t' ) ) s.pop_back ( );
    }
    inline std::string ltrim ( std::string s ) {
        size_t i = 0; while ( i < s.size ( ) && ( s[ i ] == ' ' || s[ i ] == '\t' ) ) ++i; return s.substr ( i );
    }
    inline std::string trim ( std::string s ) { rtrim_inplace ( s ); return ltrim ( std::move ( s ) ); }

    inline bool parse_bool ( std::string v ) {
        for ( auto& c : v ) c = char ( std::tolower ( unsigned char ( c ) ) );
        return ( v == "1" || v == "true" || v == "yes" || v == "on" );
    }
}

namespace engine {

    std::string SaveStorage::SlotPath ( int slot ) {
        slot = clamp_slot ( slot );
        return "saves/slot" + std::to_string ( slot ) + ".sav";
    }

    bool SaveStorage::Load ( int slot , protocol::SaveData& out ) const noexcept {
        slot = clamp_slot ( slot );
        std::ifstream f ( SlotPath ( slot ) , std::ios::in | std::ios::binary );
        if ( !f ) return false;

        // Start from fresh defaults but keep as local temp until successful parse finishes.
        protocol::SaveData tmp; // defaults from protocol

        std::string line;
        while ( std::getline ( f , line ) ) {
            rtrim_inplace ( line );
            if ( line.empty ( ) || line[ 0 ] == '#' ) continue;

            const auto pos = line.find ( '=' );
            if ( pos == std::string::npos ) continue;

            const std::string key = trim ( line.substr ( 0 , pos ) );
            const std::string val = trim ( line.substr ( pos + 1 ) );

            if ( key == "version" ) { try { tmp.version = std::stoi ( val ); } catch ( ... ) {} }
            else if ( key == "lastHub" ) { tmp.lastHub = val; }
            else if ( key == "lastStage" ) { tmp.lastStage = val; }
            else if ( key == "lastSpawn" ) { tmp.lastSpawn = val; }
            else if ( key.rfind ( "flag:" , 0 ) == 0 ) {
                const std::string flagName = key.substr ( 5 );
                tmp.flags[ flagName ] = parse_bool ( val );
            }
        }

        // Basic sanity: version field must exist/align (allow forward-compat within reason).
        if ( tmp.version <= 0 ) return false;

        out = std::move ( tmp );
        return true;
    }

    bool SaveStorage::Save ( int slot , const protocol::SaveData& in ) const noexcept {
        slot = clamp_slot ( slot );
        try {
            fs::create_directories ( "saves" );
        }
        catch ( ... ) {
            // directory creation failed; still try write (may succeed if already exists)
        }

        std::ofstream f ( SlotPath ( slot ) , std::ios::out | std::ios::binary | std::ios::trunc );
        if ( !f ) return false;

        f << "version=" << in.version << "\n";
        f << "lastHub=" << in.lastHub << "\n";
        f << "lastStage=" << in.lastStage << "\n";
        f << "lastSpawn=" << in.lastSpawn << "\n";
        for ( const auto& kv : in.flags ) {
            f << "flag:" << kv.first << "=" << ( kv.second ? "1" : "0" ) << "\n";
        }

        return true;
    }

} // namespace engine
