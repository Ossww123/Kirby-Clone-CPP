#include "pch.h"
#include "CDoor.h"

#include "CPlayer.h"
#include "CKeyMgr.h"
#include "CEventMgr.h"
#include "CCore.h"
#include "CCamera.h"
#include "CCollider.h"
#include "CSceneMgr.h"

CDoor::CDoor()
    : m_eTargetScene(SCENE_TYPE::STAGE01)
    , m_vTargetPosition(Vec2(100.f, 400.f))
    , m_strTargetDoorID(L"")
    , m_strDoorID(L"door_default")
    , m_bPlayerNearby(false)
    , m_bIsLocked(false)
    , m_fInteractionRange(80.f)
    , m_fAnimTimer(0.f)
    , m_bShowPrompt(false)
{
    // 문 오브젝트 기본 설정
    SetScale(Vec2(64.f, 128.f));

    // 콜라이더 생성 및 설정
    CreateCollider();
    GetCollider()->SetOffsetPos(Vec2(0.f, 0.f));
    GetCollider()->SetScale(Vec2(60.f, 120.f));
}

CDoor::~CDoor()
{
}

void CDoor::Update()
{
    // 애니메이션 타이머 업데이트
    m_fAnimTimer += fDT;

    // 플레이어와의 상호작용 체크
    CheckPlayerInteraction();

    // 프롬프트 표시 상태 업데이트
    m_bShowPrompt = m_bPlayerNearby && !m_bIsLocked;
}

void CDoor::Render(HDC _dc)
{
    // 문 기본 렌더링
    Vec2 vRenderPos = CCamera::GetInst()->GetRenderPos(GetPos());
    Vec2 vScale = GetScale();

    // 문 모양 그리기 (임시 - 추후 스프라이트로 교체)
    HBRUSH hBrush;
    if (m_bIsLocked)
    {
        hBrush = CreateSolidBrush(RGB(139, 69, 19)); // 갈색 (잠긴 문)
    }
    else
    {
        hBrush = CreateSolidBrush(RGB(160, 82, 45)); // 밝은 갈색 (열린 문)
    }

    HBRUSH hOldBrush = (HBRUSH)SelectObject(_dc, hBrush);

    Rectangle(_dc,
        (int)(vRenderPos.x - vScale.x / 2),
        (int)(vRenderPos.y - vScale.y / 2),
        (int)(vRenderPos.x + vScale.x / 2),
        (int)(vRenderPos.y + vScale.y / 2));

    // 문 손잡이 그리기
    HBRUSH hHandleBrush = CreateSolidBrush(RGB(255, 215, 0)); // 금색
    SelectObject(_dc, hHandleBrush);

    Ellipse(_dc,
        (int)(vRenderPos.x + vScale.x / 4 - 8),
        (int)(vRenderPos.y - 8),
        (int)(vRenderPos.x + vScale.x / 4 + 8),
        (int)(vRenderPos.y + 8));

    SelectObject(_dc, hOldBrush);
    DeleteObject(hBrush);
    DeleteObject(hHandleBrush);

    // 시각 효과 렌더링
    RenderDoorEffect(_dc);

    // 프롬프트 렌더링
    if (m_bShowPrompt)
    {
        RenderPrompt(_dc);
    }
}

void CDoor::OnCollisionEnter(CCollider* _pOther)
{
    CObject* pOtherObj = _pOther->GetObj();
    if (pOtherObj->GetName() == L"Player")
    {
        m_bPlayerNearby = true;
    }
}

void CDoor::OnCollision(CCollider* _pOther)
{
    CObject* pOtherObj = _pOther->GetObj();
    if (pOtherObj->GetName() == L"Player")
    {
        m_bPlayerNearby = true;

        // 위쪽 화살표 키 입력 시 문 입장
        if (KEY_TAP(KEY::UP) && !m_bIsLocked)
        {
            ProcessDoorEnter();
        }
    }
}

void CDoor::OnCollisionExit(CCollider* _pOther)
{
    CObject* pOtherObj = _pOther->GetObj();
    if (pOtherObj->GetName() == L"Player")
    {
        m_bPlayerNearby = false;
    }
}

void CDoor::CheckPlayerInteraction()
{
    // 콜라이더로 이미 체크되므로 여기서는 추가 로직만
    // 예: 거리 기반 상호작용 범위 세밀 조정 등
}

void CDoor::ProcessDoorEnter()
{
    if (m_bIsLocked)
        return;

    // 씬 전환 이벤트 생성
    tEvent sceneChangeEvent;
    sceneChangeEvent.eEvent = EVENT_TYPE::SCENE_CHANGE;
    sceneChangeEvent.lParam = 0;
    sceneChangeEvent.wParam = (DWORD_PTR)m_eTargetScene;

    CEventMgr::GetInst()->AddEvent(sceneChangeEvent);

    // 플레이어 위치 이동 이벤트 생성 (씬 전환 후 실행됨)
    tEvent playerMoveEvent;
    playerMoveEvent.eEvent = EVENT_TYPE::PLAYER_TELEPORT;
    playerMoveEvent.lParam = (DWORD_PTR)&m_vTargetPosition;
    playerMoveEvent.wParam = (DWORD_PTR)m_strTargetDoorID.c_str();

    CEventMgr::GetInst()->AddEvent(playerMoveEvent);

    // 문 입장 효과음 재생 (사운드 시스템 구현 시)
    // CSoundMgr::GetInst()->PlaySFX(L"door_enter.wav");
}

void CDoor::RenderPrompt(HDC _dc)
{
    Vec2 vRenderPos = CCamera::GetInst()->GetRenderPos(GetPos());
    Vec2 vScale = GetScale();

    // 프롬프트 위치 (문 위쪽)
    Vec2 vPromptPos = Vec2(vRenderPos.x, vRenderPos.y - vScale.y / 2 - 30.f);

    // 배경 박스 그리기
    HBRUSH hBgBrush = CreateSolidBrush(RGB(0, 0, 0));
    HBRUSH hOldBrush = (HBRUSH)SelectObject(_dc, hBgBrush);

    Rectangle(_dc,
        (int)(vPromptPos.x - 60),
        (int)(vPromptPos.y - 15),
        (int)(vPromptPos.x + 60),
        (int)(vPromptPos.y + 15));

    SelectObject(_dc, hOldBrush);
    DeleteObject(hBgBrush);

    // 텍스트 그리기
    SetTextColor(_dc, RGB(255, 255, 255));
    SetBkMode(_dc, TRANSPARENT);

    RECT textRect;
    textRect.left = (int)(vPromptPos.x - 55);
    textRect.top = (int)(vPromptPos.y - 10);
    textRect.right = (int)(vPromptPos.x + 55);
    textRect.bottom = (int)(vPromptPos.y + 10);

    DrawText(_dc, L"↑ 키로 입장", -1, &textRect, DT_CENTER | DT_VCENTER);
}

void CDoor::RenderDoorEffect(HDC _dc)
{
    if (!m_bPlayerNearby)
        return;

    Vec2 vRenderPos = CCamera::GetInst()->GetRenderPos(GetPos());
    Vec2 vScale = GetScale();

    // 반짝이는 효과 (사인파 이용)
    float fAlpha = (sin(m_fAnimTimer * 3.f) + 1.f) * 0.5f;
    int iAlpha = (int)(fAlpha * 128 + 64); // 64~192 범위

    // 테두리 발광 효과
    HPEN hGlowPen = CreatePen(PS_SOLID, 3, RGB(255, 255, iAlpha));
    HPEN hOldPen = (HPEN)SelectObject(_dc, hGlowPen);

    HBRUSH hOldBrush = (HBRUSH)SelectObject(_dc, GetStockObject(NULL_BRUSH));

    Rectangle(_dc,
        (int)(vRenderPos.x - vScale.x / 2 - 2),
        (int)(vRenderPos.y - vScale.y / 2 - 2),
        (int)(vRenderPos.x + vScale.x / 2 + 2),
        (int)(vRenderPos.y + vScale.y / 2 + 2));

    SelectObject(_dc, hOldPen);
    SelectObject(_dc, hOldBrush);
    DeleteObject(hGlowPen);
}

void CDoor::SetEditorData(SCENE_TYPE _eScene, Vec2 _vPos, const wstring& _strTargetID)
{
    m_eTargetScene = _eScene;
    m_vTargetPosition = _vPos;
    m_strTargetDoorID = _strTargetID;
}