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

CEditorCameraController::CEditorCameraController()
    : m_pEditorCore(nullptr)
    , m_fCameraSpeed(1000.f)
    , m_fFastSpeed(1500.f)
    , m_fSlowSpeed(700.f)
    , m_bCameraMoving(false)
    , m_vLastCameraPos{}
    , m_bUseCameraBounds(false)
    , m_vCameraBoundsMin(Vec2(-2000.f, -2000.f))
    , m_vCameraBoundsMax(Vec2(4000.f, 1280.f))
{
}

CEditorCameraController::~CEditorCameraController()
{
}

void CEditorCameraController::Initialize(CEditorCore* _pCore)
{
    m_pEditorCore = _pCore;

    // 초기 카메라 위치 저장
    m_vLastCameraPos = CCamera::GetInst()->GetLookAt();
    m_bCameraMoving = false;

    // 기본 카메라 범위 설정 (실제 좌표계)
    m_bUseCameraBounds = true;
    m_vCameraBoundsMin = Vec2(0.f, 0.f);
    m_vCameraBoundsMax = Vec2(3840.f, 2160.f);

    // 초기 카메라 위치를 UI상 (0,0)으로 = 실제 (0, height)
    Vec2 vInitialPos = Vec2(0.f, 2160.f);  // UI상 (0,0) 위치
    CCamera::GetInst()->SetLookAt(vInitialPos);
    m_vLastCameraPos = vInitialPos;
    m_bCameraMoving = false;
}

void CEditorCameraController::Update()
{
    UpdateCameraMovement();
    UpdateCameraState();
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

void CEditorCameraController::UpdateCameraMovement()
{
    Vec2 vCameraPos = CCamera::GetInst()->GetLookAt();
    Vec2 vMoveDir = Vec2(0.f, 0.f);
    bool bInputDetected = false;

    // 화살표 키로 카메라 이동
    if (KEY_HOLD(KEY::UP))
    {
        vMoveDir.y -= 1.f;
        bInputDetected = true;
    }
    if (KEY_HOLD(KEY::DOWN))
    {
        vMoveDir.y += 1.f;
        bInputDetected = true;
    }
    if (KEY_HOLD(KEY::LEFT))
    {
        vMoveDir.x -= 1.f;
        bInputDetected = true;
    }
    if (KEY_HOLD(KEY::RIGHT))
    {
        vMoveDir.x += 1.f;
        bInputDetected = true;
    }

    // 대각선 이동 시 속도 정규화
    if (vMoveDir.x != 0.f && vMoveDir.y != 0.f)
    {
        vMoveDir.Normalize();
    }

    // 입력이 있을 때만 카메라 이동
    if (bInputDetected)
    {
        float fCurrentSpeed = GetCurrentSpeed();
        float fDeltaTime = CTimeMgr::GetInst()->GetfDT();

        Vec2 vMovement = vMoveDir * fCurrentSpeed * fDeltaTime;
        vCameraPos += vMovement;

        // 카메라 범위 제한 적용
        if (m_bUseCameraBounds)
        {
            ApplyCameraBounds(vCameraPos);
        }

        CCamera::GetInst()->SetLookAt(vCameraPos);
        m_bCameraMoving = true;
    }
    else
    {
        m_bCameraMoving = false;
    }

    // 특수 키 조합
    if (KEY_TAP(KEY::HOME))
    {
        ResetCameraToOrigin();
    }
    else if (KEY_TAP(KEY::F) && KEY_HOLD(KEY::CTRL))
    {
        FocusOnPlayerSpawn();
    }
    else if (KEY_TAP(KEY::O) && KEY_HOLD(KEY::ALT))
    {
        FocusOnObjects();
    }
}

void CEditorCameraController::UpdateCameraState()
{
    Vec2 vCurrentPos = CCamera::GetInst()->GetLookAt();

    // 카메라 위치가 변했는지 확인 (수동 이동 포함)
    Vec2 vPosDiff = vCurrentPos - m_vLastCameraPos;
    if (vPosDiff.Length() > 0.1f)
    {
        m_bCameraMoving = true;
    }

    m_vLastCameraPos = vCurrentPos;
}

void CEditorCameraController::ApplyCameraBounds(Vec2& _vCameraPos)
{
    if (_vCameraPos.x < m_vCameraBoundsMin.x)
        _vCameraPos.x = m_vCameraBoundsMin.x;
    if (_vCameraPos.x > m_vCameraBoundsMax.x)
        _vCameraPos.x = m_vCameraBoundsMax.x;
    if (_vCameraPos.y < m_vCameraBoundsMin.y)
        _vCameraPos.y = m_vCameraBoundsMin.y;
    if (_vCameraPos.y > m_vCameraBoundsMax.y)
        _vCameraPos.y = m_vCameraBoundsMax.y;
}

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

void CEditorCameraController::SetCameraPosition(Vec2 _vPos)
{
    if (m_bUseCameraBounds)
    {
        ApplyCameraBounds(_vPos);
    }

    CCamera::GetInst()->SetLookAt(_vPos);

    // 디버그 메시지
    wchar_t szBuffer[256];
    swprintf_s(szBuffer, L"Camera moved to (%.0f, %.0f)", _vPos.x, _vPos.y);
    SetWindowText(CCore::GetInst()->GetMainHwnd(), szBuffer);
}

void CEditorCameraController::MoveCameraBy(Vec2 _vOffset)
{
    Vec2 vCurrentPos = CCamera::GetInst()->GetLookAt();
    SetCameraPosition(vCurrentPos + _vOffset);
}

void CEditorCameraController::CenterCameraOn(Vec2 _vTarget)
{
    SetCameraPosition(_vTarget);
}

Vec2 CEditorCameraController::GetCameraPosition()
{
    return CCamera::GetInst()->GetLookAt();
}

void CEditorCameraController::SetCameraBounds(Vec2 _vMin, Vec2 _vMax)
{
    m_vCameraBoundsMin = _vMin;
    m_vCameraBoundsMax = _vMax;

    // 현재 카메라 위치가 범위를 벗어났다면 조정
    if (m_bUseCameraBounds)
    {
        Vec2 vCurrentPos = GetCameraPosition();
        ApplyCameraBounds(vCurrentPos);
        CCamera::GetInst()->SetLookAt(vCurrentPos);
    }
}

void CEditorCameraController::ResetCameraToOrigin()
{
    Vec2 vResolution = CCore::GetInst()->GetResolution();
    Vec2 vCenter = Vec2(vResolution.x / 2.f, vResolution.y / 2.f);

    SetCameraPosition(vCenter);
    SetWindowText(CCore::GetInst()->GetMainHwnd(), L"Camera reset to origin");
}

void CEditorCameraController::FocusOnPlayerSpawn()
{
    Vec2 vSpawnPos = m_pEditorCore->GetObjectManager()->GetPlayerSpawnPos();
    SetCameraPosition(vSpawnPos);
    SetWindowText(CCore::GetInst()->GetMainHwnd(), L"Camera focused on player spawn");
}

void CEditorCameraController::FocusOnObjects()
{
    CScene* pScene = m_pEditorCore->GetWorkingScene();

    // 모든 오브젝트의 중심점 계산
    Vec2 vMinPos = Vec2(FLT_MAX, FLT_MAX);
    Vec2 vMaxPos = Vec2(-FLT_MAX, -FLT_MAX);
    int objectCount = 0;

    // 모든 그룹에서 오브젝트 수집 (플레이어 제외)
    for (UINT i = 0; i < (UINT)GROUP_TYPE::END; ++i)
    {
        if (i == (UINT)GROUP_TYPE::PLAYER) continue;

        const vector<CObject*>& vecObj = pScene->GetGroupObject((GROUP_TYPE)i);
        for (size_t j = 0; j < vecObj.size(); ++j)
        {
            if (vecObj[j] && !vecObj[j]->IsDead())
            {
                Vec2 vPos = vecObj[j]->GetPos();

                if (vPos.x < vMinPos.x) vMinPos.x = vPos.x;
                if (vPos.y < vMinPos.y) vMinPos.y = vPos.y;
                if (vPos.x > vMaxPos.x) vMaxPos.x = vPos.x;
                if (vPos.y > vMaxPos.y) vMaxPos.y = vPos.y;

                objectCount++;
            }
        }
    }

    if (objectCount > 0)
    {
        // 오브젝트들의 중심점으로 카메라 이동
        Vec2 vCenter = Vec2((vMinPos.x + vMaxPos.x) / 2.f, (vMinPos.y + vMaxPos.y) / 2.f);
        SetCameraPosition(vCenter);

        wchar_t szBuffer[256];
        swprintf_s(szBuffer, L"Camera focused on %d objects", objectCount);
        SetWindowText(CCore::GetInst()->GetMainHwnd(), szBuffer);
    }
    else
    {
        // 오브젝트가 없으면 플레이어 스폰으로
        FocusOnPlayerSpawn();
        SetWindowText(CCore::GetInst()->GetMainHwnd(), L"No objects found - focused on player spawn");
    }
}

void CEditorCameraController::AutoSetBoundsFromObjects()
{
    CScene* pScene = m_pEditorCore->GetWorkingScene();

    Vec2 vMinPos = Vec2(0.f, -1000.f);  // 최소값은 고정
    Vec2 vMaxPos = Vec2(1000.f, 1280.f); // 기본값

    // 모든 오브젝트의 최대 범위 계산
    for (UINT i = 0; i < (UINT)GROUP_TYPE::END; ++i)
    {
        if (i == (UINT)GROUP_TYPE::PLAYER) continue;

        const vector<CObject*>& vecObj = pScene->GetGroupObject((GROUP_TYPE)i);
        for (size_t j = 0; j < vecObj.size(); ++j)
        {
            if (vecObj[j] && !vecObj[j]->IsDead())
            {
                Vec2 vPos = vecObj[j]->GetPos();
                Vec2 vScale = vecObj[j]->GetScale();

                // 오브젝트의 우하단 경계 계산
                float fRight = vPos.x + vScale.x / 2.f;
                float fTop = vPos.y - vScale.y / 2.f;

                if (fRight > vMaxPos.x) vMaxPos.x = fRight + 200.f; // 여유 공간 추가
                if (fTop < vMinPos.y) vMinPos.y = fTop - 200.f;     // 여유 공간 추가
            }
        }
    }

    // 계산된 경계 적용
    SetCameraBounds(vMinPos, vMaxPos);
    EnableCameraBounds(true);
}
