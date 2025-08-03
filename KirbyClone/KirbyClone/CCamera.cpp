#include "pch.h"
#include "CCamera.h"

#include "CObject.h"
#include "CCore.h"
#include "CTimeMgr.h"
#include "CKeyMgr.h"

CCamera::CCamera()
    : m_pTargetObj(nullptr)
    , m_vLookAt{}
    , m_vCurLookAt{}
    , m_vPrevLookAt{}
    , m_vResolution{}
    , m_vDiff{}
    , m_fTime(0.f)
    , m_fShakeTime(0.f)
    , m_fShakePower(0.f)
{
}

CCamera::~CCamera()
{
}

void CCamera::init(int _iWidth, int _iHeight)
{
    m_vResolution = Vec2((float)_iWidth, (float)_iHeight);

    // 카메라 초기 위치를 화면 중앙으로 설정
    Vec2 vCenter = m_vResolution * 0.5f;
    m_vLookAt = vCenter;
    m_vCurLookAt = vCenter;
}

void CCamera::update()
{
    UpdateTargetTracking();    // 타겟 추적
    UpdateManualInput();       // 키보드 입력  
    UpdateSmoothMovement();    // 부드러운 이동
    UpdateCameraShake();       // 화면 흔들림
    CalDiff();                 // 차이값 계산

    m_fTime += CTimeMgr::GetInst()->GetfDT();
}

void CCamera::UpdateTargetTracking()
{
    if (!m_pTargetObj)
        return;

    // 타겟이 죽었다면 타겟을 nullptr로 설정
    if (m_pTargetObj->IsDead())
    {
        m_pTargetObj = nullptr;
        return;
    }

    // 타겟 위치로 LookAt 설정
    m_vLookAt = m_pTargetObj->GetPos();
}

void CCamera::UpdateManualInput()
{
    float fDT = CTimeMgr::GetInst()->GetfDT();
    float fMoveDistance = MANUAL_SPEED * fDT;

    // WASD 키로 카메라 수동 조작
    if (KEY_HOLD(KEY::W))
        m_vLookAt.y -= fMoveDistance;
    if (KEY_HOLD(KEY::S))
        m_vLookAt.y += fMoveDistance;
    if (KEY_HOLD(KEY::A))
        m_vLookAt.x -= fMoveDistance;
    if (KEY_HOLD(KEY::D))
        m_vLookAt.x += fMoveDistance;
}

void CCamera::UpdateSmoothMovement()
{
    Vec2 vDistance = m_vLookAt - m_vCurLookAt;
    float fDistanceLength = vDistance.Length();

    // 거리가 0에 가까우면 즉시 이동
    if (fDistanceLength < 0.1f)
    {
        m_vCurLookAt = m_vLookAt;
        return;
    }

    // 부드러운 보간 이동
    Vec2 vDirection = vDistance.Normalize();
    float fSpeed = fDistanceLength * CTimeMgr::GetInst()->GetfDT() * SMOOTH_SPEED;

    // 너무 가까우면 즉시 이동
    if (fDistanceLength <= fSpeed)
    {
        m_vCurLookAt = m_vLookAt;
    }
    else
    {
        m_vCurLookAt += vDirection * fSpeed;
    }
}

void CCamera::UpdateCameraShake()
{
    if (m_fShakeTime <= 0.f)
        return;

    // 랜덤한 방향으로 흔들림
    Vec2 vShakeOffset;
    vShakeOffset.x = ((float)rand() / RAND_MAX - 0.5f) * m_fShakePower;
    vShakeOffset.y = ((float)rand() / RAND_MAX - 0.5f) * m_fShakePower;

    m_vCurLookAt += vShakeOffset;

    // 흔들림 시간 감소
    m_fShakeTime -= CTimeMgr::GetInst()->GetfDT();
    if (m_fShakeTime < 0.f)
    {
        m_fShakeTime = 0.f;
        m_fShakePower = 0.f;
    }
}

void CCamera::CalDiff()
{
    Vec2 vCenter = m_vResolution * 0.5f;
    m_vDiff = m_vCurLookAt - vCenter;
    m_vPrevLookAt = m_vCurLookAt;
}

void CCamera::CameraShake(float _fDuration, float _fPower)
{
    m_fShakeTime = _fDuration;
    m_fShakePower = _fPower;
}

void CCamera::render(HDC _dc)
{
    // 디버그 모드에서만 카메라 중심점 표시
    if (!KEY_HOLD(KEY::F))
        return;

    Vec2 vRenderPos = GetRenderPos(m_vCurLookAt);

    HPEN hPen = CreatePen(PS_SOLID, 3, RGB(255, 0, 255));
    HPEN hOldPen = (HPEN)SelectObject(_dc, hPen);

    // 십자가 그리기
    MoveToEx(_dc, (int)vRenderPos.x - 10, (int)vRenderPos.y, nullptr);
    LineTo(_dc, (int)vRenderPos.x + 10, (int)vRenderPos.y);
    MoveToEx(_dc, (int)vRenderPos.x, (int)vRenderPos.y - 10, nullptr);
    LineTo(_dc, (int)vRenderPos.x, (int)vRenderPos.y + 10);

    SelectObject(_dc, hOldPen);
    DeleteObject(hPen);
}