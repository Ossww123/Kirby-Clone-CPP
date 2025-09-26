#include "gamePCH.h"
#include "CCore.h"

#include "CTimeMgr.h"
#include "CKeyMgr.h"
#include "CSceneMgr.h"
#include "CCollisionMgr.h"
#include "CEventMgr.h"
#include "CCamera.h"
#include "CPathMgr.h"
#include "CResMgr.h"
#include "CTileMgr.h"
#include "CAnimationDataMgr.h"
#include "CSoundMgr.h"
#include "SceneChangeSystem.h"
#include "BossSystem.h"
#include "DoorSystem.h"
#include "DamageSystem.h"
#include "GameFlowSystem.h"
#include "CollisionEventSystem.h"
#include "CFadeEffect.h"


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
	// 시스템 구독 해제
	CollisionEventSystem::Shutdown();
	GameFlowSystem::Shutdown();
	DamageSystem::Shutdown();
	DoorSystem::Shutdown();
	BossSystem::Shutdown();
	SceneChangeSystem::Shutdown();

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

	DWORD style = ( DWORD ) GetWindowLongPtr ( m_hWnd , GWL_STYLE );
	DWORD exstyle = ( DWORD ) GetWindowLongPtr ( m_hWnd , GWL_EXSTYLE );
	BOOL  hasMenu = ( GetMenu ( m_hWnd ) != nullptr );   // 메뉴 존재 여부 동기화

	// 윈도우 크기 조정
	RECT rt = { 0, 0, ( long ) m_ptResolution.x, ( long ) m_ptResolution.y };
	AdjustWindowRectEx ( &rt , style , hasMenu , exstyle );
	SetWindowPos ( m_hWnd , nullptr , WINDOW_POS_X , WINDOW_POS_Y , rt.right - rt.left , rt.bottom - rt.top , 0 );

	// 더블 버퍼링 초기화
	m_hDC = GetDC ( m_hWnd );
	m_hBit = CreateCompatibleBitmap ( m_hDC , m_ptResolution.x , m_ptResolution.y );
	m_memDC = CreateCompatibleDC ( m_hDC );

	HBITMAP hOldBit = ( HBITMAP ) SelectObject ( m_memDC , m_hBit );
	DeleteObject ( hOldBit );

	// ── 엔진/리소스 매니저 ─────────────────────
	CTimeMgr::GetInst()->init();
	CKeyMgr::GetInst()->init();
	CPathMgr::GetInst()->init();
	CResMgr::GetInst()->init();
	CSoundMgr::GetInst()->init();
	CAnimationDataMgr::GetInst()->init();
	CTileMgr::GetInst()->init();

	// ── 이벤트 버스 먼저 ───────────────────────
	CEventMgr::GetInst()->init();             // (슬림: 큐 초기화만)

	// ── 이벤트 수행 시스템(구독 등록) ───────────
	CollisionEventSystem::Init();
	SceneChangeSystem::Init();
	BossSystem::Init();
	DoorSystem::Init();
	DamageSystem::Init();
	GameFlowSystem::Init();

	// ── 씬/충돌/카메라/연출 ────────────────────
	CSceneMgr::GetInst()->init();             // 씬 생성/Enter() 진행
	CCollisionMgr::GetInst()->init();
	CCamera::GetInst()->init(m_ptResolution.x, m_ptResolution.y);
	CFadeEffect::GetInst()->Init();

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