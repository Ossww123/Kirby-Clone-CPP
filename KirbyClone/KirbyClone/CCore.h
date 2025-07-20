#pragma once

class CCore
{
	SINGLE(CCore);
private:
	HWND	m_hWnd;			// 메인 윈도우 핸들
	POINT	m_ptResolution; // 메인 윈도우 해상도
	HDC		m_hDC;			// 메인 윈도우에 Draw 할 DC


public:
	int init(HWND _hWnd, POINT _ptResolution);
	void progress();

private:
	void update();			// 물체들의 변경점 체크
	void render();

};

