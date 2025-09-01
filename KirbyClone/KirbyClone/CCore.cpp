#include "pch.h"
#include "CCore.h"

#include "CTimeMgr.h"
#include "CKeyMgr.h"
#include "CSceneMgr.h"
#include "CCollisionMgr.h"
#include "CEventMgr.h"
#include "CCamera.h"
#include "CPathMgr.h"
#include "CResMgr.h"
#include "CGrid.h"
#include "CBackgroundMgr.h"
#include "CTileMgr.h"
#include "CStageMgr.h"
#include "CAnimationDataMgr.h"
#include "CSoundMgr.h"
#include "CUIMgr.h"
#include "CFadeEffect.h"

// === 정적 멤버 변수 정의 ===
bool CCore::s_bShowDebugVisuals = false;

CCore::CCore ( )
	: m_hWnd ( 0 )
	, m_ptResolution{}
	, m_hDC ( 0 )
	, m_hBit ( 0 )
	, m_memDC ( 0 )
{
}

CCore::~CCore ( )
{
	// 메인 DC 해제
	ReleaseDC ( m_hWnd , m_hDC );

	// 더블 버퍼링 리소스 정리
	DeleteDC ( m_memDC );
	DeleteObject ( m_hBit );
}

int CCore::init ( HWND _hWnd , POINT _ptResolution )
{
	// 기본 정보 설정
	m_hWnd = _hWnd;
	m_ptResolution = _ptResolution;

	// 윈도우 크기 조정
	RECT rt = { 0, 0, ( long ) m_ptResolution.x, ( long ) m_ptResolution.y };
	AdjustWindowRect ( &rt , WS_OVERLAPPEDWINDOW , true );
	SetWindowPos ( m_hWnd , nullptr , 100 , 100 , rt.right - rt.left , rt.bottom - rt.top , 0 );

	// 더블 버퍼링 초기화
	m_hDC = GetDC ( m_hWnd );
	m_hBit = CreateCompatibleBitmap ( m_hDC , m_ptResolution.x , m_ptResolution.y );
	m_memDC = CreateCompatibleDC ( m_hDC );

	HBITMAP hOldBit = ( HBITMAP ) SelectObject ( m_memDC , m_hBit );
	DeleteObject ( hOldBit );

	// 핵심 매니저 초기화
	CTimeMgr::GetInst ( )->init ( );
	CKeyMgr::GetInst ( )->init ( );
	CPathMgr::GetInst ( )->init ( );
	CResMgr::GetInst ( )->init ( );
	CSoundMgr::GetInst ( )->init ( );

	// 애니메이션 데이터 매니저 초기화
	CAnimationDataMgr::GetInst ( )->init ( );

	// 게임 콘텐츠 매니저들 초기화
	CBackgroundMgr::GetInst ( )->init ( );
	CTileMgr::GetInst ( )->init ( );
	CStageMgr::GetInst ( )->init ( );

	// 게임 로직 매니저 초기화
	CSceneMgr::GetInst ( )->init ( );
	CCollisionMgr::GetInst ( )->init ( );
	CEventMgr::GetInst ( )->init ( );
	CCamera::GetInst ( )->init ( m_ptResolution.x , m_ptResolution.y );
	CGrid::GetInst ( )->init ( );

	// UI 매니저 초기화
	CUIMgr::GetInst ( )->Init ( );
	CFadeEffect::GetInst ( )->Init ( );

	//// 테스트 코드
	//CAnimationDataMgr::GetInst()->init();
	//CAnimationDataMgr::GetInst()->CreateSampleAnimationFile(L"test_player.json");
	//CAnimationDataMgr::GetInst()->TestDirectLoad();

	//CAnimationDataMgr::GetInst()->TestLoadAnimationFile(L"test_player.json");

	return S_OK;
}

void CCore::progress ( )
{
	update ( );
	render ( );
}

void CCore::update ( )
{
	// 시간 및 입력 업데이트
	CTimeMgr::GetInst ( )->update ( );
	CKeyMgr::GetInst ( )->update ( );
	
	// CTRL 키로 디버그 시각 요소 토글
	if (CKeyMgr::GetInst()->IsKeyTap(KEY::CTRL))
	{
		ToggleDebugVisuals();
	}

	// 카메라 및 씬 업데이트
	CCamera::GetInst ( )->update ( );
	CSceneMgr::GetInst ( )->update ( );

	// 충돌 처리
	CCollisionMgr::GetInst ( )->update ( );

	// 이벤트 처리 (마지막)
	CEventMgr::GetInst ( )->update ( );
	
	// 페이드 효과 업데이트
	CFadeEffect::GetInst ( )->Update ( );
}

void CCore::render ( )
{
	// 백버퍼(m_memDC) 전체를 흰색으로 초기화
	Rectangle ( m_memDC , -1 , -1 , m_ptResolution.x + 1 , m_ptResolution.y + 1 );

	// 백버퍼에 씬 렌더링
	CSceneMgr::GetInst ( )->render ( m_memDC );
	CCamera::GetInst ( )->render ( m_memDC );
	
	// 페이드 효과 렌더링 (최상위)
	CFadeEffect::GetInst ( )->Render ( m_memDC );

	// 화면 출력
	BitBlt ( m_hDC , 0 , 0 , m_ptResolution.x , m_ptResolution.y ,
		m_memDC , 0 , 0 , SRCCOPY );

	// FPS 정보 업데이트
	CTimeMgr::GetInst ( )->render ( );
}

void CCore::SetGameResolution ( )
{
	ChangeResolution ( GAME_WIDTH , GAME_HEIGHT );
}

void CCore::SetToolResolution ( )
{
	ChangeResolution ( TOOL_WIDTH , TOOL_HEIGHT );
}

void CCore::ChangeResolution ( int _iWidth , int _iHeight )
{
	// 동일한 해상도면 변경 생략
	if ( m_ptResolution.x == _iWidth && m_ptResolution.y == _iHeight )
		return;

	// 해상도 설정
	m_ptResolution.x = _iWidth;
	m_ptResolution.y = _iHeight;

	// 윈도우 크기 조정
	UpdateWindowSize ( );

	// 백버퍼 정리 & 재생성
	RecreateBackBuffer ( );

	// 관련 시스템 업데이트
	CCamera::GetInst ( )->init ( m_ptResolution.x , m_ptResolution.y );
}

void CCore::UpdateWindowSize ( )
{
	RECT rt = { 0, 0, ( long ) m_ptResolution.x, ( long ) m_ptResolution.y };
	AdjustWindowRect ( &rt , WS_OVERLAPPEDWINDOW , true );
	SetWindowPos ( m_hWnd , nullptr , 100 , 100 ,
		rt.right - rt.left , rt.bottom - rt.top , 0 );
}

void CCore::RecreateBackBuffer ( )
{
	// 기존 백버퍼 정리
	if ( m_memDC ) DeleteDC ( m_memDC );
	if ( m_hBit ) DeleteObject ( m_hBit );

	// 새로운 백버퍼 생성
	m_hBit = CreateCompatibleBitmap ( m_hDC , m_ptResolution.x , m_ptResolution.y );
	m_memDC = CreateCompatibleDC ( m_hDC );

	HBITMAP hOldBit = ( HBITMAP ) SelectObject ( m_memDC , m_hBit );
	DeleteObject ( hOldBit );
}