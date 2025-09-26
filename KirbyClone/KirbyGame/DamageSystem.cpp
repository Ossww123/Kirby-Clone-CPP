#include "gamePCH.h"
#include "DamageSystem.h"
#include "CEventMgr.h"
#include "EventDef.h"

#include "CKirby.h"
#include "CKirbyHealthSystem.h"
#include "CKirbyMovement.h"

#include "CSceneMgr.h"
#include "CScene.h"
#include "CMonster.h"
#include "CBasicMonster.h"
#include "CBoss.h"
#include "CProjectile.h"

size_t DamageSystem::s_subPlayerDamage = 0;
size_t DamageSystem::s_subPlayerRecoil = 0;
size_t DamageSystem::s_subMonsterDamage = 0;

void DamageSystem::Init() {
    auto& bus = *CEventMgr::GetInst();
    s_subPlayerDamage = bus.Subscribe(EVENT_TYPE::PLAYER_DAMAGE, &DamageSystem::OnPlayerDamage, 10);
    s_subPlayerRecoil = bus.Subscribe(EVENT_TYPE::PLAYER_SLIDE_KICK_RECOIL, &DamageSystem::OnPlayerRecoil, 10);
    s_subMonsterDamage = bus.Subscribe(EVENT_TYPE::MONSTER_DAMAGE, &DamageSystem::OnMonsterDamage, 10);
}
void DamageSystem::Shutdown() {
    auto& bus = *CEventMgr::GetInst();
    if (s_subPlayerDamage)  bus.Unsubscribe(EVENT_TYPE::PLAYER_DAMAGE, s_subPlayerDamage), s_subPlayerDamage = 0;
    if (s_subPlayerRecoil)  bus.Unsubscribe(EVENT_TYPE::PLAYER_SLIDE_KICK_RECOIL, s_subPlayerRecoil), s_subPlayerRecoil = 0;
    if (s_subMonsterDamage) bus.Unsubscribe(EVENT_TYPE::MONSTER_DAMAGE, s_subMonsterDamage), s_subMonsterDamage = 0;
}

void DamageSystem::OnPlayerDamage(const tEvent& e) {
    auto* kirby = reinterpret_cast<CKirby*>(e.wParam);
    Vec2* pKnock = reinterpret_cast<Vec2*>(e.lParam);
    if (!kirby) { if (pKnock) delete pKnock; return; }

    auto* hs = kirby->GetHealth();
    if (hs && hs->IsAlive() && !hs->IsInvincible()) {
        Vec2 knock = pKnock ? *pKnock : Vec2(0, 0);
        hs->TakeDamage(1, knock);
    }
    if (pKnock) delete pKnock; // 프로토콜 준수
}

void DamageSystem::OnPlayerRecoil(const tEvent& e) {
    (void)e;
    // 필요 시 슬라이드킥 반동 등
    auto* sc = CSceneMgr::GetInst()->GetCurScene(); if (!sc) return;
    const auto& v = sc->GetGroupObject(GROUP_TYPE::PLAYER);
    if (v.empty()) return;
    if (auto* kirby = dynamic_cast<CKirby*>(v[0])) {
        if (auto* mv = kirby->GetMovement()) {
            mv->SlideKickRecoil(); // CKirbyMovement에 구현
        }
    }
}

void DamageSystem::OnMonsterDamage(const tEvent& e) {
    auto* mon = reinterpret_cast<CMonster*>(e.wParam);
    auto* prj = reinterpret_cast<CProjectile*>(e.lParam);
    if (!mon || !mon->IsAlive()) return;

    if (auto* boss = dynamic_cast<CBoss*>(mon)) {
        if (!prj) return;
        int dmg = 1;
        switch (prj->GetProjectileType()) {
        case PROJECTILE_TYPE::KIRBY_SLIDE_KICK:
        case PROJECTILE_TYPE::KIRBY_AIR_PUFF: return; // 보스는 피해 없음
        case PROJECTILE_TYPE::KIRBY_STAR: dmg = 12; break;
        default: break;
        }
        boss->TakeBossDamage(dmg);
        return;
    }

    if (auto* basic = dynamic_cast<CBasicMonster*>(mon)) {
        if (prj) basic->SetDamageSourcePos(prj->GetPos());
    }
    mon->TakeDamage();
}
