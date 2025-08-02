#include "pch.h"
#include "CScene_Tool.h"
#include "CEditorCore.h"

#include "CKeyMgr.h"
#include "CCore.h"
#include "CCamera.h"
#include "CTimeMgr.h"
#include "CEditorFileManager.h"
#include "CEditorObjectManager.h"

CScene_Tool::CScene_Tool()
    : m_pEditorCore(nullptr)
{
}

CScene_Tool::~CScene_Tool()
{
    if (m_pEditorCore)
    {
        delete m_pEditorCore;
        m_pEditorCore = nullptr;
    }
}

void CScene_Tool::Enter()
{
    // 에디터 코어 시스템 생성 및 초기화
    m_pEditorCore = new CEditorCore();
    m_pEditorCore->Initialize(this);

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

    DeleteAllObject();
}

void CScene_Tool::Update()
{
    // 1. 에디터 시스템 업데이트 (입력, 카메라, 오브젝트 관리 등)
    if (m_pEditorCore)
    {
        m_pEditorCore->Update();
    }

    // 2. 기본 Scene 업데이트 (모든 오브젝트 업데이트)
    CScene::Update();
}

void CScene_Tool::Render(HDC _dc)
{
    // 에디터 렌더링은 EditorCore에서 통합 처리
    if (m_pEditorCore)
    {
        m_pEditorCore->Render(_dc);
    }
    else
    {
        // 에디터 시스템이 없을 경우 기본 Scene 렌더링만
        CScene::Render(_dc);
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
        m_pEditorCore->GetObjectManager()->SetPlayerSpawnPosition(Vec2(640.f, 400.f));
    }

    SetWindowText(CCore::GetInst()->GetMainHwnd(), L"Level cleared!");
}