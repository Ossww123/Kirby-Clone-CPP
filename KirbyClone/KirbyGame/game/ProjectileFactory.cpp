#include "game/ProjectileFactory.h"

#include <fstream>
#include <sstream>
#include <vector>
#include <algorithm>
#include <cctype>

namespace game {

    // ---------- internal helpers ----------
    namespace {
        inline void ltrim ( std::string& s ) {
            s.erase ( s.begin ( ) , std::find_if ( s.begin ( ) , s.end ( ) ,
                [ ] ( unsigned char ch ) { return !std::isspace ( ch ); } ) );
        }
        inline void rtrim ( std::string& s ) {
            s.erase ( std::find_if ( s.rbegin ( ) , s.rend ( ) ,
                [ ] ( unsigned char ch ) { return !std::isspace ( ch ); } ).base ( ) , s.end ( ) );
        }
        inline void trim ( std::string& s ) { ltrim ( s ); rtrim ( s ); }

        inline bool parseBool ( std::string v , bool def = false ) {
            trim ( v );
            std::transform ( v.begin ( ) , v.end ( ) , v.begin ( ) ,
                           [ ] ( unsigned char c ) { return ( char ) std::tolower ( c ); } );
            if ( v == "1" || v == "true" || v == "yes" || v == "y" ) return true;
            if ( v == "0" || v == "false" || v == "no" || v == "n" ) return false;
            return def;
        }

        inline bool startsWithHash ( const std::string& line ) {
            for ( char c : line ) {
                if ( !std::isspace ( ( unsigned char ) c ) ) return c == '#';
            }
            return false;
        }

        static std::vector<std::string> splitCSVLine ( const std::string& line ) {
            // Simple CSV split (no quoted commas support). Good enough for our numeric config.
            std::vector<std::string> out;
            std::string cur;
            cur.reserve ( line.size ( ) );
            for ( char ch : line ) {
                if ( ch == ',' ) {
                    trim ( cur );
                    out.push_back ( cur );
                    cur.clear ( );
                }
                else {
                    cur.push_back ( ch );
                }
            }
            trim ( cur );
            out.push_back ( cur );
            return out;
        }

        struct HeaderIndex {
            int id = -1;
            int width = -1 , height = -1;
            int speed = -1 , ttl = -1;
            int gravity = -1 , frictionAir = -1 , frictionGround = -1 , termVel = -1;
            int dieOnAnyWorldHit = -1 , ignoreOneWay = -1;
            int damage = -1 , knockbackX = -1 , knockbackY = -1;
        };

        HeaderIndex buildHeaderIndex ( const std::vector<std::string>& cols ) {
            HeaderIndex hi;
            for ( int i = 0; i < ( int ) cols.size ( ); ++i ) {
                const std::string& c = cols[ i ];
                if ( c == "id" ) hi.id = i;
                else if ( c == "width" ) hi.width = i;
                else if ( c == "height" ) hi.height = i;
                else if ( c == "speed" ) hi.speed = i;
                else if ( c == "ttl" ) hi.ttl = i;
                else if ( c == "gravity" ) hi.gravity = i;
                else if ( c == "frictionAir" ) hi.frictionAir = i;
                else if ( c == "frictionGround" ) hi.frictionGround = i;
                else if ( c == "termVel" ) hi.termVel = i;
                else if ( c == "dieOnAnyWorldHit" ) hi.dieOnAnyWorldHit = i;
                else if ( c == "ignoreOneWay" ) hi.ignoreOneWay = i;
                else if ( c == "damage" ) hi.damage = i;
                else if ( c == "knockbackX" ) hi.knockbackX = i;
                else if ( c == "knockbackY" ) hi.knockbackY = i;
            }
            return hi;
        }

        template <typename T>
        inline T getOr ( const std::vector<std::string>& row , int idx , T def ) {
            if ( idx < 0 || idx >= ( int ) row.size ( ) || row[ idx ].empty ( ) ) return def;
            std::istringstream is ( row[ idx ] );
            T v{};
            is >> v;
            if ( !is.fail ( ) ) return v;
            return def;
        }

        inline bool getOrBool ( const std::vector<std::string>& row , int idx , bool def ) {
            if ( idx < 0 || idx >= ( int ) row.size ( ) || row[ idx ].empty ( ) ) return def;
            return parseBool ( row[ idx ] , def );
        }
    } // namespace

    // ---------- registry ----------
    std::unordered_map<std::string , ProjDef>& ProjectileFactory::Registry ( ) {
        static std::unordered_map<std::string , ProjDef> R;
        return R;
    }

    void ProjectileFactory::Register ( const std::string& id , const ProjDef& d ) {
        if ( id.empty ( ) ) return;
        Registry ( )[ id ] = d;
    }

    const ProjDef* ProjectileFactory::Find ( const std::string& id ) {
        auto& R = Registry ( );
        auto it = R.find ( id );
        return ( it == R.end ( ) ) ? nullptr : &it->second;
    }

    // ---------- defaults ----------
    void ProjectileFactory::RegisterDefaults ( ) {
        Register ( "Star" , ProjDef{
            .width = 8, .height = 8, .speed = 620.f, .ttl = 1.5f,
            .gravity = 0.f, .frictionAir = 0.f, .frictionGround = 0.f, .termVel = 99999.f,
            .dieOnAnyWorldHit = true, .ignoreOneWay = true,
            .damage = 1, .knockback = { 300.f, -120.f }
        } );

        Register ( "AirPuff" , ProjDef{
            .width = 8, .height = 8, .speed = 420.f, .ttl = 0.6f,
            .gravity = 0.f, .frictionAir = 0.f, .frictionGround = 0.f, .termVel = 99999.f,
            .dieOnAnyWorldHit = true, .ignoreOneWay = true,
            .damage = 1, .knockback = { 120.f, -60.f }
        } );

        Register ( "FirePellet" , ProjDef{
            .width = 10, .height = 8, .speed = 360.f, .ttl = 0.9f,
            .gravity = 0.f, .frictionAir = 0.f, .frictionGround = 0.f, .termVel = 99999.f,
            .dieOnAnyWorldHit = true, .ignoreOneWay = true,
            .damage = 1, .knockback = { 220.f, -100.f }
        } );
    }

    // ---------- creation ----------
    std::unique_ptr<Projectile> ProjectileFactory::Create (
        const std::string& id ,
        const RECT& worldRect ,
        const engine::physics::CollisionSystem* col ,
        ProjOwner owner )
    {
        auto it = Registry ( ).find ( id );
        if ( it == Registry ( ).end ( ) ) return {};

        const ProjDef& d = it->second;

        Projectile::Cfg cfg;
        cfg.width = d.width;
        cfg.height = d.height;
        cfg.speed = d.speed;
        cfg.ttl = d.ttl;

        cfg.gravity = d.gravity;
        cfg.frictionAir = d.frictionAir;
        cfg.frictionGround = d.frictionGround;
        cfg.termVel = d.termVel;

        cfg.dieOnAnyWorldHit = d.dieOnAnyWorldHit;
        cfg.ignoreOneWay = d.ignoreOneWay;

        auto p = std::make_unique<Projectile> ( worldRect , col , owner , cfg );

        // Inject combat payload (optional, read-only to others)
        ProjPayload payload;
        payload.damage = d.damage;
        payload.knockback = d.knockback;
        p->SetPayload ( payload );

        return p;
    }

    // ---------- CSV loader ----------
    bool ProjectileFactory::LoadCSV ( const char* filename ) {
        if ( !filename ) return false;

        std::ifstream fs ( filename );
        if ( !fs.is_open ( ) ) return false;

        std::string line;
        if ( !std::getline ( fs , line ) ) return false;
        if ( startsWithHash ( line ) ) {
            // Skip comment header; find the actual header line
            bool gotHeader = false;
            while ( std::getline ( fs , line ) ) {
                if ( startsWithHash ( line ) ) continue;
                gotHeader = true;
                break;
            }
            if ( !gotHeader ) return false;
        }

        // Parse header
        auto headerCols = splitCSVLine ( line );
        for ( auto& c : headerCols ) trim ( c );
        HeaderIndex hi = buildHeaderIndex ( headerCols );
        if ( hi.id < 0 ) {
            // We require at least 'id' column.
            return false;
        }

        // Rows
        while ( std::getline ( fs , line ) ) {
            if ( line.empty ( ) || startsWithHash ( line ) ) continue;

            auto cols = splitCSVLine ( line );
            if ( hi.id >= ( int ) cols.size ( ) ) continue;

            std::string id = cols[ hi.id ];
            trim ( id );
            if ( id.empty ( ) ) continue;

            ProjDef d; // start from defaults

            if ( hi.width >= 0 )         d.width = getOr<float> ( cols , hi.width , d.width );
            if ( hi.height >= 0 )        d.height = getOr<float> ( cols , hi.height , d.height );
            if ( hi.speed >= 0 )         d.speed = getOr<float> ( cols , hi.speed , d.speed );
            if ( hi.ttl >= 0 )           d.ttl = getOr<float> ( cols , hi.ttl , d.ttl );

            if ( hi.gravity >= 0 )       d.gravity = getOr<float> ( cols , hi.gravity , d.gravity );
            if ( hi.frictionAir >= 0 )   d.frictionAir = getOr<float> ( cols , hi.frictionAir , d.frictionAir );
            if ( hi.frictionGround >= 0 )d.frictionGround = getOr<float> ( cols , hi.frictionGround , d.frictionGround );
            if ( hi.termVel >= 0 )       d.termVel = getOr<float> ( cols , hi.termVel , d.termVel );

            if ( hi.dieOnAnyWorldHit >= 0 ) d.dieOnAnyWorldHit = getOrBool ( cols , hi.dieOnAnyWorldHit , d.dieOnAnyWorldHit );
            if ( hi.ignoreOneWay >= 0 )     d.ignoreOneWay = getOrBool ( cols , hi.ignoreOneWay , d.ignoreOneWay );

            if ( hi.damage >= 0 )        d.damage = getOr<int> ( cols , hi.damage , d.damage );
            if ( hi.knockbackX >= 0 )    d.knockback.x = getOr<float> ( cols , hi.knockbackX , d.knockback.x );
            if ( hi.knockbackY >= 0 )    d.knockback.y = getOr<float> ( cols , hi.knockbackY , d.knockback.y );

            Register ( id , d );
        }

        return true;
    }

} // namespace game
