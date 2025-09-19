#include "pch.h"
#include "CEditorCameraController.h"
#include "CEditorCore.h"
#include "CEditorObjectManager.h"

#include "CCamera.h"
#include "CKeyMgr.h"
#include "CTimeMgr.h"
#include "CCore.h"
#include "CScene.h"
#include "CObject.h"

// ============================================
// 생명주기 함수
// ============================================

CEditorCameraController::CEditorCameraController()
    : m_pEditorCore(nullptr)
    , m_fCameraSpeed(1000.f)
    , m_fFastSpeed(1500.f)
    , m_fSlowSpeed(700.f)
    , m_bCameraMoving(false)
    , m_vLastCameraPos{}
    , m_vCameraBoundsMin(Vec2(0.f, 0.f))
    , m_vCameraBoundsMax(Vec2(4096.f, 640.f))
{
}

CEditorCameraController::~CEditorCameraController()
{
}

// ============================================
// 핵심 생명주기 함수
// ============================================

void CEditorCameraController::Initialize(CEditorCore* _pCore)
{
    // 에디터 코어 참조 설정
    m_pEditorCore = _pCore;

    // 카메라 범위 제한 설정 (스테이지 이미지 4배 스케일 기준)
    m_vCameraBoundsMin = Vec2(0.f, 0.f);
    m_vCameraBoundsMax = Vec2(4096.f, 640.f);

    // 초기 카메라 위치 설정 (화면 하단 좌측)
    Vec2 vInitialPos = Vec2(0.f, 640.f);
    CCamera::GetInst()->SetLookAt(vInitialPos);

    // 상태 초기화
    m_vLastCameraPos = vInitialPos;
    m_bCameraMoving = false;
}

void CEditorCameraController::Update()
{
    UpdateCameraMovement();
    UpdateCameraState();
}

// ============================================
// 카메라 움직임 내부 처리
// ============================================

void CEditorCameraController::UpdateCameraMovement()
{
    // 키보드 입력 처리
    Vec2 vMoveDir = Vec2(0.f, 0.f);
    bool bInputDetected = false;
    ProcessKeyboardInput(vMoveDir, bInputDetected);

    // 입력이 있을 때만 카메라 이동
    if (bInputDetected)
    {
        // 이동 거리 계산
        float fCurrentSpeed = GetCurrentSpeed();
        float fDeltaTime = CTimeMgr::GetInst()->GetfDT();
        Vec2 vMovement = vMoveDir * fCurrentSpeed * fDeltaTime;

        // 카메라 위치 업데이트
        Vec2 vCameraPos = CCamera::GetInst()->GetLookAt();
        vCameraPos += vMovement;

        // 범위 제한 적용 후 설정
        ApplyCameraBounds(vCameraPos);
        CCamera::GetInst()->SetLookAt(vCameraPos);

        m_bCameraMoving = true;
    }
    else
    {
        m_bCameraMoving = false;
    }
}

void CEditorCameraController::UpdateCameraState()
{
    // 카메라 이동 상태 업데이트
    Vec2 vCurrentPos = CCamera::GetInst()->GetLookAt();
    Vec2 vPosDiff = vCurrentPos - m_vLastCameraPos;

    if (vPosDiff.Length() > 0.1f)
    {
        m_bCameraMoving = true;
    }

    m_vLastCameraPos = vCurrentPos;
}

void CEditorCameraController::ProcessKeyboardInput(Vec2& _vMoveDir, bool& _bInputDetected)
{
    // 화살표 키로 카메라 이동
    if (KEY_HOLD(KEY::UP))
    {
        _vMoveDir.y -= 1.f;
        _bInputDetected = true;
    }
    if (KEY_HOLD(KEY::DOWN))
    {
        _vMoveDir.y += 1.f;
        _bInputDetected = true;
    }
    if (KEY_HOLD(KEY::LEFT))
    {
        _vMoveDir.x -= 1.f;
        _bInputDetected = true;
    }
    if (KEY_HOLD(KEY::RIGHT))
    {
        _vMoveDir.x += 1.f;
        _bInputDetected = true;
    }

    // 대각선 이동 시 속도 정규화
    if (_vMoveDir.x != 0.f && _vMoveDir.y != 0.f)
    {
        _vMoveDir.Normalize();
    }
}

// ============================================
// 속도 계산 내부 함수
// ============================================

float CEditorCameraController::GetCurrentSpeed()
{
    // Shift 키: 빠른 이동
    if (KEY_HOLD(KEY::SHIFT))
    {
        return m_fFastSpeed;
    }
    // Ctrl 키: 느린 이동 (정밀 조작)
    else if (KEY_HOLD(KEY::CTRL))
    {
        return m_fSlowSpeed;
    }
    // 기본 속도
    else
    {
        return m_fCameraSpeed;
    }
}

// ============================================
// 카메라 위치 제어
// ============================================

void CEditorCameraController::SetCameraPosition(Vec2 _vPos)
{
    // 범위 제한 적용
    ApplyCameraBounds(_vPos);

    // 카메라 위치 설정
    CCamera::GetInst()->SetLookAt(_vPos);
}

void CEditorCameraController::MoveCameraBy(Vec2 _vOffset)
{
    // 현재 위치에서 오프셋만큼 이동
    Vec2 vCurrentPos = CCamera::GetInst()->GetLookAt();
    SetCameraPosition(vCurrentPos + _vOffset);
}

void CEditorCameraController::CenterCameraOn(Vec2 _vTarget)
{
    SetCameraPosition(_vTarget);
}

void CEditorCameraController::ResetCameraPosition()
{
    if (m_pEditorCore && m_pEditorCore->GetWorkingScene())
    {
        // 카메라를 UI상 (0,0)으로 이동 = 실제 (0, height)
        Vec2 vMapSize = m_pEditorCore->GetMapSize();
        Vec2 vUIZeroPos = Vec2(0.f, vMapSize.y);
        CCamera::GetInst()->SetLookAt(vUIZeroPos);
    }
}

void CEditorCameraController::ResetCameraToOrigin()
{
    // 화면 중앙으로 카메라 리셋
    Vec2 vResolution = CCore::GetInst()->GetResolution();
    Vec2 vCenter = Vec2(vResolution.x / 2.f, vResolution.y / 2.f);
    SetCameraPosition(vCenter);
}

// ============================================
// 카메라 범위 제한
// ============================================

void CEditorCameraController::SetCameraBounds(Vec2 _vMin, Vec2 _vMax)
{
    // 범위 설정
    m_vCameraBoundsMin = _vMin;
    m_vCameraBoundsMax = _vMax;

    // 현재 카메라 위치가 범위를 벗어났다면 조정
    Vec2 vCurrentPos = GetCameraPosition();
    ApplyCameraBounds(vCurrentPos);
    CCamera::GetInst()->SetLookAt(vCurrentPos);
}

// ============================================
// 범위 제한 내부 처리
// ============================================

void CEditorCameraController::ApplyCameraBounds(Vec2& _vCameraPos)
{
    // X축 범위 제한
    if (_vCameraPos.x < m_vCameraBoundsMin.x)
        _vCameraPos.x = m_vCameraBoundsMin.x;
    if (_vCameraPos.x > m_vCameraBoundsMax.x)
        _vCameraPos.x = m_vCameraBoundsMax.x;

    // Y축 범위 제한
    if (_vCameraPos.y < m_vCameraBoundsMin.y)
        _vCameraPos.y = m_vCameraBoundsMin.y;
    if (_vCameraPos.y > m_vCameraBoundsMax.y)
        _vCameraPos.y = m_vCameraBoundsMax.y;
}

// ============================================
// Getter 함수들
// ============================================

Vec2 CEditorCameraController::GetCameraPosition() const
{
    return CCamera::GetInst()->GetLookAt();
}