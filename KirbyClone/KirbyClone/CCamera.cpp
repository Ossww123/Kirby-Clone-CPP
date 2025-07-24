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
    , m_fSpeed(0.f)
    , m_fAccTime(0.f)
    , m_fAccPower(0.f)
{
}

CCamera::~CCamera()
{
}

void CCamera::init(int _iWidth, int _iHeight)
{
    Vec2 vResolution = Vec2((float)_iWidth, (float)_iHeight);
    m_vResolution = vResolution;

    // 카메라 초기 위치를 화면 중앙으로 설정
    m_vLookAt = Vec2(_iWidth / 2.f, _iHeight / 2.f);
    m_vCurLookAt = m_vLookAt;
}

void CCamera::update()
{
    // 타겟이 있다면 타겟을 따라가기
    if (m_pTargetObj)
    {
        if (m_pTargetObj->IsDead())
        {
            m_pTargetObj = nullptr;
        }
        else
        {
            m_vLookAt = m_pTargetObj->GetPos();
        }
    }

    // 카메라 이동 처리 (키 입력으로 수동 조작)
    if (KEY_HOLD(KEY::W))
    {
        m_vLookAt.y -= 300.f * CTimeMgr::GetInst()->GetfDT();
    }
    if (KEY_HOLD(KEY::S))
    {
        m_vLookAt.y += 300.f * CTimeMgr::GetInst()->GetfDT();
    }
    if (KEY_HOLD(KEY::A))
    {
        m_vLookAt.x -= 300.f * CTimeMgr::GetInst()->GetfDT();
    }
    if (KEY_HOLD(KEY::D))
    {
        m_vLookAt.x += 300.f * CTimeMgr::GetInst()->GetfDT();
    }

    // 부드러운 카메라 이동 (보간)
    Vec2 vDist = m_vLookAt - m_vCurLookAt;

    if (!vDist.Length() == 0.f)
    {
        Vec2 vLookDir = vDist;
        vLookDir.Normalize();

        float fSpeed = vDist.Length() * CTimeMgr::GetInst()->GetfDT() * 10.f;

        // 너무 가까우면 즉시 이동
        if (vDist.Length() <= fSpeed)
        {
            m_vCurLookAt = m_vLookAt;
        }
        else
        {
            m_vCurLookAt += vLookDir * fSpeed;
        }
    }

    // 화면 흔들림 효과 처리
    if (m_fAccTime > 0.f)
    {
        // 랜덤한 방향으로 흔들림
        Vec2 vShakeOffset;
        vShakeOffset.x = ((float)rand() / RAND_MAX - 0.5f) * m_fAccPower;
        vShakeOffset.y = ((float)rand() / RAND_MAX - 0.5f) * m_fAccPower;

        m_vCurLookAt += vShakeOffset;

        m_fAccTime -= CTimeMgr::GetInst()->GetfDT();
        if (m_fAccTime < 0.f)
        {
            m_fAccTime = 0.f;
            m_fAccPower = 0.f;
        }
    }

    // 카메라 차이값 계산
    CalDiff();

    m_fTime += CTimeMgr::GetInst()->GetfDT();
}

void CCamera::render(HDC _dc)
{
    // 카메라 관련 디버그 정보 출력 (필요시)
    if (KEY_HOLD(KEY::F))
    {
        // 카메라 중심점 표시
        Vec2 vRenderPos = GetRenderPos(m_vCurLookAt);

        HPEN hPen = CreatePen(PS_SOLID, 3, RGB(255, 0, 255)); // 보라색
        HPEN hOldPen = (HPEN)SelectObject(_dc, hPen);

        // 십자가 그리기
        MoveToEx(_dc, (int)vRenderPos.x - 10, (int)vRenderPos.y, nullptr);
        LineTo(_dc, (int)vRenderPos.x + 10, (int)vRenderPos.y);
        MoveToEx(_dc, (int)vRenderPos.x, (int)vRenderPos.y - 10, nullptr);
        LineTo(_dc, (int)vRenderPos.x, (int)vRenderPos.y + 10);

        SelectObject(_dc, hOldPen);
        DeleteObject(hPen);
    }
}

void CCamera::CalDiff()
{
    // 해상도 중심점
    Vec2 vResolution = CCore::GetInst()->GetResolution();
    Vec2 vCenter = vResolution / 2.f;

    m_vDiff = m_vCurLookAt - vCenter;

    m_vPrevLookAt = m_vCurLookAt;
}