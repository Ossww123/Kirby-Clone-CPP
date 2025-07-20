#include "pch.h"
#include "CCore.h"

CCore::CCore()
	: m_hWnd(0)
	, m_ptResolution{}
	, m_hDC(0)
{

}

CCore::~CCore()
{

}

int CCore::init(HWND _hWnd, POINT _ptResolution)
{
	m_hWnd = _hWnd;
	m_ptResolution = _ptResolution;

	// 해상도에 맞게 윈도우 크기 조정
	RECT rt = { 0, 0, (long)m_ptResolution.x, (long)m_ptResolution.y };
	AdjustWindowRect(&rt, WS_OVERLAPPEDWINDOW, true);
	SetWindowPos(m_hWnd, nullptr, 100, 100, rt.right - rt.left, rt.bottom - rt.top,0);

	_ptResolution.x;
	_ptResolution.y;

	return S_OK;
}

void CCore::progress()
{
	update();

	render();
}

void CCore::update()
{
}


void CCore::render()
{
}
