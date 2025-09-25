#include "gamePCH.h"
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
    , m_vStageBoundsMin{}
    , m_vStageBoundsMax{}
    , m_bUseStageBounds(false)
    , m_bBossMode(false)
    , m_vBossLockPos{}
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
    // 보스 모드일 때는 카메라를 고정 위치에 고정
    if (m_bBossMode)
    {
        m_vLookAt = m_vBossLockPos;
        return;
    }

    if (!m_pTargetObj)
        return;

    // 타겟이 죽었다면 타겟을 nullptr로 설정
    if (!m_pTargetObj->IsAlive())
    {
        m_pTargetObj = nullptr;
        return;
    }

    // 타겟 위치로 LookAt 설정
    Vec2 vTargetPos = m_pTargetObj->GetPos();
    
    // 스테이지 경계가 설정되어 있다면 카메라 위치를 제한
    if (m_bUseStageBounds)
    {
        Vec2 vHalfResolution = m_vResolution * 0.5f;
        
        // 스테이지 끝에서 반 타일(32px) 여유 공간 추가
        const float STAGE_MARGIN = 32.f;
        
        // X축 제한: 카메라가 스테이지를 벗어나지 않도록 (여유 공간 포함)
        float fMinCameraX = m_vStageBoundsMin.x + vHalfResolution.x + STAGE_MARGIN;
        float fMaxCameraX = m_vStageBoundsMax.x - vHalfResolution.x - STAGE_MARGIN;
        
        // Y축 제한: 카메라가 스테이지를 벗어나지 않도록 (여유 공간 포함)
        float fMinCameraY = m_vStageBoundsMin.y + vHalfResolution.y + STAGE_MARGIN;
        float fMaxCameraY = m_vStageBoundsMax.y - vHalfResolution.y - STAGE_MARGIN;
        
        // 스테이지가 화면보다 작거나 같으면 중앙 고정
        if (fMinCameraX >= fMaxCameraX)
        {
            vTargetPos.x = (m_vStageBoundsMin.x + m_vStageBoundsMax.x) * 0.5f;
        }
        else
        {
            if (vTargetPos.x < fMinCameraX) vTargetPos.x = fMinCameraX;
            if (vTargetPos.x > fMaxCameraX) vTargetPos.x = fMaxCameraX;
        }
        
        if (fMinCameraY >= fMaxCameraY)
        {
            vTargetPos.y = (m_vStageBoundsMin.y + m_vStageBoundsMax.y) * 0.5f;
        }
        else
        {
            if (vTargetPos.y < fMinCameraY) vTargetPos.y = fMinCameraY;
            if (vTargetPos.y > fMaxCameraY) vTargetPos.y = fMaxCameraY;
        }
    }
    
    m_vLookAt = vTargetPos;
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

void CCamera::SetLookAtImmediate(Vec2 _vLook)
{
    m_vLookAt = _vLook;
    m_vCurLookAt = _vLook;  // 현재 위치도 즉시 설정
    m_vPrevLookAt = _vLook;  // 이전 위치도 설정해서 부드러운 이동 방지
    CalDiff();  // 차이값 재계산
}

void CCamera::SetStageBounds(Vec2 _vMin, Vec2 _vMax)
{
    m_vStageBoundsMin = _vMin;
    m_vStageBoundsMax = _vMax;
    m_bUseStageBounds = true;
}

Vec2 CCamera::GetCameraBounds() const
{
    Vec2 vHalfResolution = m_vResolution * 0.5f;
    Vec2 vTopLeft = m_vCurLookAt - vHalfResolution;
    Vec2 vBottomRight = m_vCurLookAt + vHalfResolution;
    return Vec2(vTopLeft.x, vTopLeft.y);
}

void CCamera::ClampPositionToCameraBounds(Vec2& _vPos) const
{
    Vec2 vHalfResolution = m_vResolution * 0.5f;
    const float PLAYER_MARGIN = 32.f; // 플레이어가 화면 경계에서 떨어져야 할 거리 (반 타일)
    
    // 카메라가 보는 영역의 경계 계산 (32px 마진 적용)
    Vec2 vCameraTopLeft = m_vCurLookAt - vHalfResolution + Vec2(PLAYER_MARGIN, PLAYER_MARGIN);
    Vec2 vCameraBottomRight = m_vCurLookAt + vHalfResolution - Vec2(PLAYER_MARGIN, PLAYER_MARGIN);
    
    // 플레이어 위치를 카메라 화면 경계 내로 제한 (32px 마진 포함)
    if (_vPos.x < vCameraTopLeft.x) _vPos.x = vCameraTopLeft.x;
    if (_vPos.x > vCameraBottomRight.x) _vPos.x = vCameraBottomRight.x;
    if (_vPos.y < vCameraTopLeft.y) _vPos.y = vCameraTopLeft.y;
    if (_vPos.y > vCameraBottomRight.y) _vPos.y = vCameraBottomRight.y;
}

void CCamera::StartBossMode(Vec2 _vLockPos)
{
    m_bBossMode = true;
    m_vBossLockPos = _vLockPos;
    
    // 즉시 보스 위치로 카메라 이동 (부드러운 전환 없이)
    m_vLookAt = _vLockPos;
    //m_vCurLookAt = m_vLookAt;
}

void CCamera::EndBossMode()
{
    m_bBossMode = false;
    // 타겟 추적을 다시 시작하려면 UpdateTargetTracking()에서 자동으로 처리됨
}