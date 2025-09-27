#include "gamePCH.h"
#include "CCamera.h"
#include "CTimeMgr.h"

CCamera::CCamera() {}
CCamera::~CCamera() {}

void CCamera::init(int _iWidth, int _iHeight)
{
    m_vResolution = Vec2((float)_iWidth, (float)_iHeight);
    const Vec2 vCenter = m_vResolution * 0.5f;
    m_vLookAt = vCenter;
    m_vCurLookAt = vCenter;
    m_vPrevLookAt = vCenter;
    CalDiff();
}

void CCamera::update()
{
    const float dt = CTimeMgr::GetInst()->GetfDT();

    if (!m_bSmooth)
    {
        if (m_vCurLookAt.x != m_vLookAt.x || m_vCurLookAt.y != m_vLookAt.y)
            m_vCurLookAt = m_vLookAt;
    }
    else
    {
        Vec2 delta = m_vLookAt - m_vCurLookAt;
        const float dist = delta.Length();
        if (dist < 0.1f) {
            m_vCurLookAt = m_vLookAt;
        }
        else {
            Vec2 dir = delta;
            dir.Normalize();
            const float step = dist * dt * m_fSmoothSpeed;
            m_vCurLookAt = (dist <= step) ? m_vLookAt : (m_vCurLookAt + dir * step);
        }
    }

    CalDiff();
    m_fTime += dt;
}

void CCamera::render(HDC _dc)
{
#ifdef CAMERA_DEBUG_DRAW
    // 화면 중심 십자만 그려줌(키 입력/게임 로직 의존 제거)
    const Vec2 vRenderPos = GetRenderPos(m_vCurLookAt);
    HPEN hPen = CreatePen(PS_SOLID, 2, RGB(255, 0, 255));
    HPEN hOld = (HPEN)SelectObject(_dc, hPen);

    MoveToEx(_dc, (int)vRenderPos.x - 8, (int)vRenderPos.y, nullptr);
    LineTo(_dc, (int)vRenderPos.x + 8, (int)vRenderPos.y);
    MoveToEx(_dc, (int)vRenderPos.x, (int)vRenderPos.y - 8, nullptr);
    LineTo(_dc, (int)vRenderPos.x, (int)vRenderPos.y + 8);

    SelectObject(_dc, hOld);
    DeleteObject(hPen);
#endif
}

void CCamera::SetLookAt(Vec2 _vLook)
{
    m_vLookAt = _vLook;
}

void CCamera::SetLookAtImmediate(Vec2 _vLook)
{
    m_vLookAt = _vLook;
    m_vCurLookAt = _vLook;
    m_vPrevLookAt = _vLook;
    CalDiff();
}

void CCamera::CalDiff()
{
    const Vec2 vCenter = m_vResolution * 0.5f;
    m_vDiff = m_vCurLookAt - vCenter;
    m_vPrevLookAt = m_vCurLookAt;
}
