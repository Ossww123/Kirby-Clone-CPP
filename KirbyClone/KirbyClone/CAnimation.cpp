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

void CAnimation::Update()
{
    if (m_bFinish || m_vecFrame.empty())
        return;

    // 델타타임 누적
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
                --m_iCurFrame;   // 마지막 프레임에서 유지
                m_bFinish = true;
            }
        }
    }
}

void CAnimation::Render(HDC _dc, Vec2 _vPos)
{
    // �⺻ ũ��(1.0��)�� ������
    RenderScaled(_dc, _vPos, 1.0f);
}

void CAnimation::Reset()
{
    m_iCurFrame = 0;
    m_fAccTime = 0.f;
    m_bFinish = false;
}

void CAnimation::Create(CTexture* _pTex, Vec2 _vLT, Vec2 _vSliceSize, Vec2 _vStep,
    float _fDuration, int _iFrameCount, bool _bLoop)
{
    // 유효성 검사
    if (nullptr == _pTex || _iFrameCount <= 0 || _fDuration <= 0.f)
        return;

    // 기본 설정
    m_pTex = _pTex;
    m_bLoop = _bLoop;

    // 기존 프레임들 삭제
    m_vecFrame.clear();

    // 프레임들 생성
    for (int i = 0; i < _iFrameCount; ++i)
    {
        tAnimFrame frame;
        frame.vLT = Vec2(_vLT.x + _vStep.x * i, _vLT.y);
        frame.vSlice = _vSliceSize;
        frame.fDuration = _fDuration;

        m_vecFrame.push_back(frame);
    }
}



void CAnimation::AddFrame(Vec2 _vLT, Vec2 _vSliceSize, float _fDuration)
{
    // 유효성 검사
    if (_fDuration <= 0.f)
        return;

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

void CAnimation::RenderScaled(HDC _dc, Vec2 _vPos, float _fScale, bool _bFlipX)
{
    if (!IsValidRenderState())
        return;

    // 카메라 좌표 변환
    Vec2 vRenderPos = CCamera::GetInst()->GetRenderPos(_vPos);
    const tAnimFrame& frame = m_vecFrame[m_iCurFrame];
    Vec2 vDestSize = frame.vSlice * _fScale;

    // 실제 프레임 렌더링 (플립 포함)
    RenderFrame(_dc, vRenderPos, frame, vDestSize, _bFlipX);
}

bool CAnimation::IsValidCreateParams(CTexture* _pTex, int _iFrameCount, float _fDuration) const
{
    return (_pTex != nullptr && _iFrameCount > 0 && _fDuration > 0.f);
}

bool CAnimation::IsValidRenderState() const
{
    return (m_pTex != nullptr && !m_vecFrame.empty() &&
        m_iCurFrame >= 0 && m_iCurFrame < (int)m_vecFrame.size());
}

void CAnimation::CreateFrameSequence(Vec2 _vLT, Vec2 _vSliceSize, Vec2 _vStep,
    float _fDuration, int _iFrameCount)
{
    for (int i = 0; i < _iFrameCount; ++i)
    {
        tAnimFrame frame;
        frame.vLT = Vec2(_vLT.x + _vStep.x * i, _vLT.y);
        frame.vSlice = _vSliceSize;
        frame.fDuration = _fDuration;

        m_vecFrame.push_back(frame);
    }
}

void CAnimation::RenderFrame(HDC _dc, const Vec2& _vRenderPos, const tAnimFrame& _frame,
    const Vec2& _vDestSize, bool _bFlipX)
{
    if (_bFlipX)
    {
        // 플립을 위한 메모리 DC 2개 생성 (투명도 처리용)
        HDC memDC1 = CreateCompatibleDC(_dc);
        HDC memDC2 = CreateCompatibleDC(_dc);
        HBITMAP memBmp1 = CreateCompatibleBitmap(_dc, (int)_vDestSize.x, (int)_vDestSize.y);
        HBITMAP memBmp2 = CreateCompatibleBitmap(_dc, (int)_vDestSize.x, (int)_vDestSize.y);
        HBITMAP oldBmp1 = (HBITMAP)SelectObject(memDC1, memBmp1);
        HBITMAP oldBmp2 = (HBITMAP)SelectObject(memDC2, memBmp2);
        
        // 첫 번째 DC를 마젠타로 채움 (투명색)
        HBRUSH magentaBrush = CreateSolidBrush(RGB(255, 0, 255));
        RECT fillRect = {0, 0, (int)_vDestSize.x, (int)_vDestSize.y};
        FillRect(memDC1, &fillRect, magentaBrush);
        DeleteObject(magentaBrush);
        
        // 원본 이미지를 첫 번째 DC에 투명 블릿
        TransparentBlt(memDC1,
            0, 0,
            (int)_vDestSize.x,
            (int)_vDestSize.y,
            m_pTex->GetDC(),
            (int)_frame.vLT.x,
            (int)_frame.vLT.y,
            (int)_frame.vSlice.x,
            (int)_frame.vSlice.y,
            RGB(255, 0, 255));
        
        // 두 번째 DC를 마젠타로 채움
        HBRUSH magentaBrush2 = CreateSolidBrush(RGB(255, 0, 255));
        FillRect(memDC2, &fillRect, magentaBrush2);
        DeleteObject(magentaBrush2);
        
        // 첫 번째 DC를 두 번째 DC에 플립해서 복사
        StretchBlt(memDC2,
            (int)_vDestSize.x - 1, 0,  // xDest, yDest (시작 X, Y)
            -(int)_vDestSize.x,        // wDest (음수 너비로 플립)
            (int)_vDestSize.y,         // hDest (높이)
            memDC1,                    // hdcSrc
            0, 0,                      // xSrc, ySrc
            (int)_vDestSize.x,         // wSrc
            (int)_vDestSize.y,         // hSrc
            SRCCOPY);
        
        // 플립된 이미지를 화면에 투명 블릿
        TransparentBlt(_dc,
            (int)(_vRenderPos.x - _vDestSize.x / 2.f),
            (int)(_vRenderPos.y - _vDestSize.y / 2.f),
            (int)_vDestSize.x,
            (int)_vDestSize.y,
            memDC2,
            0, 0,
            (int)_vDestSize.x,
            (int)_vDestSize.y,
            RGB(255, 0, 255));
        
        // 리소스 정리
        SelectObject(memDC1, oldBmp1);
        SelectObject(memDC2, oldBmp2);
        DeleteObject(memBmp1);
        DeleteObject(memBmp2);
        DeleteDC(memDC1);
        DeleteDC(memDC2);
    }
    else
    {
        // 일반 렌더링
        TransparentBlt(_dc,
            (int)(_vRenderPos.x - _vDestSize.x / 2.f),
            (int)(_vRenderPos.y - _vDestSize.y / 2.f),
            (int)_vDestSize.x,
            (int)_vDestSize.y,
            m_pTex->GetDC(),
            (int)_frame.vLT.x,
            (int)_frame.vLT.y,
            (int)_frame.vSlice.x,
            (int)_frame.vSlice.y,
            RGB(255, 0, 255));  // 마젠타 컬러키
    }
}