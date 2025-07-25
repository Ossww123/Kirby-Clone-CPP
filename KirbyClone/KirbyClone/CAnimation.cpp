#include "pch.h"
#include "CAnimation.h"
#include "CTexture.h"
#include "CCamera.h"
#include "CTimeMgr.h"

CAnimation::CAnimation()
    : m_strName{}
    , m_pTex(nullptr)
    , m_vecFrame{}
    , m_iCurFrame(0)
    , m_fAccTime(0.f)
    , m_bFinish(false)
    , m_bLoop(true)
{
}

CAnimation::~CAnimation()
{
}

void CAnimation::Create(CTexture* _pTex, Vec2 _vLT, Vec2 _vSliceSize, Vec2 _vStep, float _fDuration, int _iFrameCount, bool _bLoop)
{
    m_pTex = _pTex;
    m_bLoop = _bLoop;

    for (int i = 0; i < _iFrameCount; ++i)
    {
        tAnimFrame frame;
        frame.vLT = Vec2(_vLT.x + _vStep.x * i, _vLT.y);
        frame.vSlice = _vSliceSize;
        frame.fDuration = _fDuration;

        m_vecFrame.push_back(frame);
    }
}

void CAnimation::Update()
{
    if (m_bFinish)
        return;

    m_fAccTime += CTimeMgr::GetInst()->GetfDT();

    // 현재 프레임의 지속시간을 넘었다면 다음 프레임으로
    if (m_fAccTime >= m_vecFrame[m_iCurFrame].fDuration)
    {
        ++m_iCurFrame;
        m_fAccTime = 0.f;

        // 마지막 프레임에 도달했다면
        if (m_iCurFrame >= (int)m_vecFrame.size())
        {
            if (m_bLoop)
            {
                m_iCurFrame = 0;  // 반복
            }
            else
            {
                --m_iCurFrame;   // 마지막 프레임에서 정지
                m_bFinish = true;
            }
        }
    }
}

void CAnimation::Render(HDC _dc, Vec2 _vPos)
{
    if (nullptr == m_pTex || m_vecFrame.empty())
        return;

    // 카메라 좌표로 변환
    Vec2 vRenderPos = CCamera::GetInst()->GetRenderPos(_vPos);

    // 현재 프레임 정보 가져오기
    tAnimFrame& frame = m_vecFrame[m_iCurFrame];

    // 스프라이트 시트에서 해당 프레임 영역만 그리기
    BitBlt(_dc,
        (int)(vRenderPos.x - frame.vSlice.x / 2.f),     // 대상 좌상단 X
        (int)(vRenderPos.y - frame.vSlice.y / 2.f),     // 대상 좌상단 Y
        (int)frame.vSlice.x,                            // 가로 크기
        (int)frame.vSlice.y,                            // 세로 크기
        m_pTex->GetDC(),                                // 소스 DC
        (int)frame.vLT.x,                               // 소스 좌상단 X
        (int)frame.vLT.y,                               // 소스 좌상단 Y
        SRCCOPY);
}

void CAnimation::Reset()
{
    m_iCurFrame = 0;
    m_fAccTime = 0.f;
    m_bFinish = false;
}