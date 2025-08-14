#include "pch.h"
#include "CInvincibleMonster.h"

#include "CCollider.h"
#include "CObject.h"
#include "CTimeMgr.h"
#include "CRigidBody.h"

CInvincibleMonster::CInvincibleMonster()
    : CMonster()
    , m_fEffectTimer(0.f)
    , m_bShowingEffect(false)
{
    // 무적 몬스터는 특별한 초기 설정 불필요
    // 자식 클래스에서 개별 설정
}

CInvincibleMonster::~CInvincibleMonster()
{
    // 상위 클래스에서 정리 처리
}

void CInvincibleMonster::OnCollisionEnter(CCollider* _pOther)
{
    HandleInvincibleCollision(_pOther);
}

void CInvincibleMonster::OnCollision(CCollider* _pOther)
{
    HandleInvincibleCollision(_pOther);
}

void CInvincibleMonster::OnCollisionExit(CCollider* _pOther)
{
    // 충돌 종료 시 특별한 처리 없음
}

void CInvincibleMonster::HandleInvincibleCollision(CCollider* _pOther)
{
    CObject* pOtherObj = _pOther->GetOwner();
    if (nullptr == pOtherObj)
        return;

    // 플레이어와 충돌 시
    if (pOtherObj->GetType() == OBJECT_TYPE::PLAYER)
    {
        PushAwayPlayer(_pOther);
        ShowInvincibleFeedback();
        PlayInvincibleSound();
    }

    // 플레이어의 공격과 충돌 시 (투사체 등)
    // TODO: 투사체 타입 체크 후 무효화 처리
    // if (pOtherObj->GetType() == OBJECT_TYPE::PLAYER_PROJECTILE)
    // {
    //     pOtherObj->SetDead();  // 투사체 제거
    //     CreateInvincibleEffect();
    // }
}

void CInvincibleMonster::PushAwayPlayer(CCollider* _pOther)
{
    CObject* pPlayer = _pOther->GetOwner();
    if (nullptr == pPlayer)
        return;

    // 플레이어를 밀어내는 방향 계산
    Vec2 vPlayerPos = pPlayer->GetPos();
    Vec2 vMyPos = GetPos();
    Vec2 vDirection = vPlayerPos - vMyPos;

    if (vDirection.Length() > 0.1f)
    {
        vDirection.Normalize();

        // 플레이어에게 밀어내는 힘 적용
        CRigidBody* pPlayerRigidBody = pPlayer->GetRigidBody();
        if (nullptr != pPlayerRigidBody)
        {
            Vec2 pushForce = vDirection * 300.f;  // 밀어내는 힘
            pPlayerRigidBody->AddForce(pushForce);
        }

        // TODO: 플레이어에게 데미지 주기
        // pPlayer->TakeDamage(1);
    }
}

void CInvincibleMonster::CreateInvincibleEffect()
{
    // 무적 이펙트 생성
    m_bShowingEffect = true;
    m_fEffectTimer = 0.5f;  // 0.5초간 이펙트 표시

    // TODO: 스파크 이펙트 생성
    // CSparkEffect* pEffect = new CSparkEffect;
    // pEffect->SetPos(GetPos());
    // pEffect->SetLifetime(0.5f);
    // 
    // CScene* pCurScene = CSceneMgr::GetInst()->GetCurScene();
    // pCurScene->AddObject(pEffect, GROUP_TYPE::EFFECT);
}

void CInvincibleMonster::TakeDamage()
{
    // 무적 몬스터는 데미지를 받지 않음
    ShowInvincibleFeedback();
    PlayInvincibleSound();
    CreateInvincibleEffect();
}

void CInvincibleMonster::ShowInvincibleFeedback()
{
    // 무적 피드백 표시 (깜빡임 등)
    m_bShowingEffect = true;
    m_fEffectTimer = 0.3f;

    // TODO: 깜빡임 효과
    // TODO: 무적 표시 UI
}

void CInvincibleMonster::PlayInvincibleSound()
{
    // 무적 사운드 재생
    // TODO: 사운드 매니저를 통한 무적 사운드 재생
    // CSoundMgr::GetInst()->PlaySFX(L"InvincibleHit");
}