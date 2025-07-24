#include "pch.h"
#include "CPlayer.h"

#include "CKeyMgr.h"
#include "CTimeMgr.h"
#include "CCollider.h"
#include "CEventMgr.h"
#include "CCamera.h"
#include "CResMgr.h"
#include "CTexture.h"

#include "CCore.h"

CPlayer::CPlayer()
{
    // 충돌체 생성
    CreateCollider();
    GetCollider()->SetScale(Vec2(80.f, 80.f));  // 충돌 박스는 조금 작게

    // 플레이어 텍스처 로드
    SetTexture(CResMgr::GetInst()->LoadTexture(L"Player", L"texture\\Player.bmp"));
}

CPlayer::~CPlayer()
{
}

void CPlayer::Update()
{
    Vec2 vPos = GetPos();

    // 키 입력에 따른 이동
    if (KEY_HOLD(KEY::LEFT))
    {
        vPos.x -= 200.f * CTimeMgr::GetInst()->GetfDT();
    }

    if (KEY_HOLD(KEY::RIGHT))
    {
        vPos.x += 200.f * CTimeMgr::GetInst()->GetfDT();
    }

    if (KEY_HOLD(KEY::UP))
    {
        vPos.y -= 200.f * CTimeMgr::GetInst()->GetfDT();
    }

    if (KEY_HOLD(KEY::DOWN))
    {
        vPos.y += 200.f * CTimeMgr::GetInst()->GetfDT();
    }

    // 스페이스바로 중앙 이동
    if (KEY_TAP(KEY::SPACE))
    {
        vPos = Vec2(640.f, 400.f);
    }

    // C키로 카메라가 플레이어를 따라가도록 설정/해제
    if (KEY_TAP(KEY::C))
    {
        if (CCamera::GetInst()->GetTarget() == this)
        {
            // 이미 타겟이면 해제
            CCamera::GetInst()->SetTarget(nullptr);
            SetWindowText(CCore::GetInst()->GetMainHwnd(), L"카메라 추적 해제");
        }
        else
        {
            // 플레이어를 카메라 타겟으로 설정
            CCamera::GetInst()->SetTarget(this);
            SetWindowText(CCore::GetInst()->GetMainHwnd(), L"카메라 추적 시작");
        }
    }

    SetPos(vPos);
}


void CPlayer::OnCollisionEnter(CCollider* _pOther)
{
    CObject* pOtherObj = _pOther->GetOwner();

    // 충돌 시 화면 흔들림 효과
    CCamera::GetInst()->CameraShake(0.3f, 10.f);

    // 디버그 출력 (윈도우 타이틀에 표시)
    SetWindowText(CCore::GetInst()->GetMainHwnd(), L"충돌 시작!");

    // 필요하다면 여기서 추가 이벤트
    // 예: 파티클 이펙트, 사운드 재생, UI 업데이트 등
}

void CPlayer::OnCollision(CCollider* _pOther)
{
    // 지속적인 충돌 처리 (매 프레임 호출)
}

void CPlayer::OnCollisionExit(CCollider* _pOther)
{
    SetWindowText(CCore::GetInst()->GetMainHwnd(), L"충돌 끝!");
}