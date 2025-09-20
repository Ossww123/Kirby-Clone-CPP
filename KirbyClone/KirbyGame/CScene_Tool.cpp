#include "pch.h"
#include "CScene_Tool.h"
#include "CEditorCore.h"
#include "CCore.h"
#include "CCamera.h"
#include "CEditorFileManager.h"
#include "CEditorObjectManager.h"

CScene_Tool::CScene_Tool()
    : m_pEditorCore(nullptr)
{
}

CScene_Tool::~CScene_Tool()
{
    // 안전장치
    if (m_pEditorCore)
    {
        delete m_pEditorCore;
        m_pEditorCore = nullptr;
    }
}

void CScene_Tool::Enter()
{
    // 카메라 초기 위치 설정
    CCamera::GetInst()->SetLookAt(Vec2(960.f, 960.f));

    // 에디터 코어 시스템 생성 및 초기화
    m_pEditorCore = new CEditorCore();
    m_pEditorCore->Initialize(this);

    // 윈도우 타이틀 변경
    SetWindowText(CCore::GetInst()->GetMainHwnd(), L"Level Editor - Ready!");
}

void CScene_Tool::Exit()
{
    // 에디터 시스템 종료
    if (m_pEditorCore)
    {
        m_pEditorCore->Shutdown();
        delete m_pEditorCore;
        m_pEditorCore = nullptr;
    }

    // 씬 객체 정리
    DeleteAllObject();
}

void CScene_Tool::Update()
{
    // 에디터 코어 업데이트
    if (m_pEditorCore)
    {
        m_pEditorCore->Update();
    }
}

void CScene_Tool::Render(HDC _dc)
{
    // 에디터 코어 렌더링
    if (m_pEditorCore)
    {
        m_pEditorCore->Render(_dc);
    }
}

void CScene_Tool::LoadLevel(const wstring& _strFileName)
{
    // 파일 매니저를 통한 레벨 로드
    if (m_pEditorCore && m_pEditorCore->GetFileManager())
    {
        m_pEditorCore->GetFileManager()->LoadLevel(_strFileName);
    }
}

void CScene_Tool::SaveLevel(const wstring& _strFileName)
{
    // 파일 매니저를 통한 레벨 저장
    if (m_pEditorCore && m_pEditorCore->GetFileManager())
    {
        m_pEditorCore->GetFileManager()->SaveLevel(_strFileName);
    }
}

void CScene_Tool::ClearLevel()
{
    // Scene의 기본 DeleteAllObject 사용
    DeleteAllObject();

    // 플레이어 스폰 위치 초기화
    if (m_pEditorCore && m_pEditorCore->GetObjectManager())
    {
        m_pEditorCore->GetObjectManager()->SetPlayerSpawnPos(Vec2(320.f, 320.f));
    }

    // 윈도우 타이틀 업데이트
    SetWindowText(CCore::GetInst()->GetMainHwnd(), L"Level cleared!");
}