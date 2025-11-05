#pragma once
#include <string>
#include "engine/Anim.h"

namespace game {
    // CSV를 읽어 Animator에 클립 등록. clearExisting=true면 기존 클립 지움.
    bool LoadAnimCSV ( const char* filename , engine::Animator* anim , bool clearExisting = true );
}
