#include "pch.h"
#include "CScene_Tool.h"
#include "CEditorCore.h"

#include "CKeyMgr.h"
#include "CCore.h"
#include "CCamera.h"
#include "CTimeMgr.h"
#include "CEditorFileManager.h"
#include "CEditorObjectManager.h"
#include "CBackground.h"
#include "CStageMgr.h"

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
    CCamera::GetInst()->SetLookAt(Vec2(960.f, 960.f));

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
    // 기존 업데이트 로직
    CScene::Update();

    // 스테이지 매니저 업데이트 (새로 추가)
    CStageMgr::GetInst()->Update();

    // 에디터 코어 업데이트 (기존)
    if (m_pEditorCore)
    {
        m_pEditorCore->Update();
    }
}

void CScene_Tool::Render(HDC _dc)
{
    // 1. 배경 렌더링 (기존)
    if (m_pEditorCore->GetObjectManager()->GetCurrentBackground())
    {
        m_pEditorCore->GetObjectManager()->GetCurrentBackground()->Render(_dc);
    }

    // 2. 스테이지 이미지 렌더링 (새로 추가)
    CStageMgr::GetInst()->Render(_dc);

    // 3. 모든 게임 오브젝트 렌더링 (기존)
    CScene::Render(_dc);

    // 4. 에디터 관련 렌더링 (기존)
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
        m_pEditorCore->GetObjectManager()->SetPlayerSpawnPosition(Vec2(640.f, 400.f));
    }

    SetWindowText(CCore::GetInst()->GetMainHwnd(), L"Level cleared!");
}