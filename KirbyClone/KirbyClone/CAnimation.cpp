#include "pch.h"
#include "CAnimation.h"
#include "CTexture.h"
#include "CCamera.h"
#include "CTimeMgr.h"

#pragma comment(lib, "msimg32.lib")

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

    // 알파 채널 지원 여부에 따라 적절한 렌더링 방식 선택
    if (m_pTex->HasAlpha())
    {
        // 32비트 알파 채널 렌더링
        m_pTex->RenderSpriteWithAlpha(_dc,
            vRenderPos,
            frame.vLT,          // 소스 시작 위치
            frame.vSlice,       // 소스 크기
            frame.vSlice,       // 대상 크기 (동일하게)
            1.0f);              // 불투명도 100%
    }
    else
    {
        // 기존 마젠타 키 색상 방식 (24비트 호환)
        m_pTex->RenderSpriteWithColorKey(_dc,
            vRenderPos,
            frame.vLT,          // 소스 시작 위치
            frame.vSlice,       // 소스 크기
            frame.vSlice,       // 대상 크기
            RGB(255, 0, 255));  // 마젠타 투명색
    }
}

void CAnimation::Reset()
{
    m_iCurFrame = 0;
    m_fAccTime = 0.f;
    m_bFinish = false;
}

void CAnimation::RenderWithAlpha(HDC _dc, Vec2 _vPos, float _fAlpha)
{
    if (nullptr == m_pTex || m_vecFrame.empty())
        return;

    Vec2 vRenderPos = CCamera::GetInst()->GetRenderPos(_vPos);
    tAnimFrame& frame = m_vecFrame[m_iCurFrame];

    if (m_pTex->HasAlpha())
    {
        // 알파 채널 + 추가 투명도 적용
        m_pTex->RenderSpriteWithAlpha(_dc,
            vRenderPos,
            frame.vLT,
            frame.vSlice,
            frame.vSlice,
            _fAlpha);
    }
    else
    {
        // 알파 채널이 없으면 키 색상 방식으로만 가능
        if (_fAlpha > 0.5f)  // 50% 이상이면 완전 불투명으로 표시
        {
            m_pTex->RenderSpriteWithColorKey(_dc,
                vRenderPos,
                frame.vLT,
                frame.vSlice,
                frame.vSlice,
                RGB(255, 0, 255));
        }
        // 50% 미만이면 렌더링하지 않음 (키 색상 방식의 한계)
    }
}

void CAnimation::AddFrame(Vec2 _vLT, Vec2 _vSliceSize, float _fDuration)
{
    tAnimFrame frame;
    frame.vLT = _vLT;
    frame.vSlice = _vSliceSize;
    frame.fDuration = _fDuration;

    m_vecFrame.push_back(frame);
}

void CAnimation::ClearFrames()
{
    m_vecFrame.clear();
    m_iCurFrame = 0;
    m_fAccTime = 0.f;
    m_bFinish = false;
}

void CAnimation::RenderScaled(HDC _dc, Vec2 _vPos, float _fScale)
{
    if (nullptr == m_pTex || m_vecFrame.empty())
        return;

    // 카메라 좌표로 변환
    Vec2 vRenderPos = CCamera::GetInst()->GetRenderPos(_vPos);

    // 현재 프레임 정보 가져오기
    tAnimFrame& frame = m_vecFrame[m_iCurFrame];

    // 대상 크기 계산 (원본 크기 * 스케일)
    Vec2 vDestSize = frame.vSlice * _fScale;

    // 알파 채널 지원 여부에 따라 적절한 렌더링 방식 선택
    if (m_pTex->HasAlpha())
    {
        // 32비트 알파 채널 렌더링 (스케일 적용)
        m_pTex->RenderSpriteWithAlpha(_dc,
            vRenderPos,
            frame.vLT,          // 소스 시작 위치
            frame.vSlice,       // 소스 크기
            vDestSize,          // 대상 크기 (스케일 적용)
            1.0f);              // 불투명도 100%
    }
    else
    {
        // 기본 마젠타 키 색상 방식 (24비트 이하) - 확대 렌더링
        TransparentBlt(_dc,
            (int)(vRenderPos.x - vDestSize.x / 2.f),
            (int)(vRenderPos.y - vDestSize.y / 2.f),
            (int)vDestSize.x,
            (int)vDestSize.y,
            m_pTex->GetDC(),
            (int)frame.vLT.x,
            (int)frame.vLT.y,
            (int)frame.vSlice.x,
            (int)frame.vSlice.y,
            RGB(255, 0, 255)); // 마젠타 컬러키
    }
}
