#include "gamePCH.h"
#include "CSound.h"

#pragma comment(lib, "winmm.lib")  // mciSendString을 위한 라이브러리

int CSound::s_iSoundCount = 0;

CSound::CSound()
    : m_eType(SOUND_TYPE::SFX)
    , m_bLoop(false)
    , m_bLoaded(false)
    , m_iVolume(1000)
    , m_strAlias(L"")
{
    // 고유한 별칭 생성
    m_strAlias = L"Sound_" + to_wstring(s_iSoundCount++);
}

CSound::~CSound()
{
    Release();
}

bool CSound::Load(const wstring& _strPath)
{
    // 이미 로드된 경우 해제
    if (m_bLoaded)
    {
        Release();
    }

    // MCI 명령으로 사운드 파일 열기
    wstring strCommand = L"open \"" + _strPath + L"\" alias " + m_strAlias;
    SendMCICommand(strCommand);

    // 로드 성공 여부 확인
    strCommand = L"status " + m_strAlias + L" mode";
    WCHAR szReturn[128] = {};
    if (mciSendString(strCommand.c_str(), szReturn, 128, nullptr) == 0)
    {
        m_bLoaded = true;
        SetRelativePath(_strPath);  // 경로 저장
        return true;
    }

    return false;
}

void CSound::Release()
{
    if (m_bLoaded)
    {
        Stop();
        wstring strCommand = L"close " + m_strAlias;
        SendMCICommand(strCommand);
        m_bLoaded = false;
    }
}

void CSound::Play(bool _bLoop)
{
    if (!m_bLoaded)
        return;

    m_bLoop = _bLoop;

    // 재생 중이면 처음부터 다시 재생
    Stop();
    
    wstring strCommand = L"play " + m_strAlias;
    if (m_bLoop)
    {
        strCommand += L" repeat";
    }
    
    SendMCICommand(strCommand);
}

void CSound::Stop()
{
    if (!m_bLoaded)
        return;

    wstring strCommand = L"stop " + m_strAlias;
    SendMCICommand(strCommand);
    
    // 재생 위치를 처음으로 되돌리기
    strCommand = L"seek " + m_strAlias + L" to start";
    SendMCICommand(strCommand);
}

void CSound::Pause()
{
    if (!m_bLoaded)
        return;

    wstring strCommand = L"pause " + m_strAlias;
    SendMCICommand(strCommand);
}

void CSound::Resume()
{
    if (!m_bLoaded)
        return;

    wstring strCommand = L"resume " + m_strAlias;
    SendMCICommand(strCommand);
}

void CSound::SetVolume(int _iVolume)
{
    if (!m_bLoaded)
        return;

    // 볼륨 범위 제한 (0~1000)
    m_iVolume = max(0, min(1000, _iVolume));

    wstring strCommand = L"setaudio " + m_strAlias + L" volume to " + to_wstring(m_iVolume);
    SendMCICommand(strCommand);
}

bool CSound::IsPlaying() const
{
    if (!m_bLoaded)
        return false;

    wstring strCommand = L"status " + m_strAlias + L" mode";
    WCHAR szReturn[128] = {};
    
    if (mciSendString(strCommand.c_str(), szReturn, 128, nullptr) == 0)
    {
        wstring strMode = szReturn;
        return (strMode == L"playing");
    }

    return false;
}

void CSound::SendMCICommand(const wstring& _strCommand)
{
    MCIERROR dwError = mciSendString(_strCommand.c_str(), nullptr, 0, nullptr);
    
#ifdef _DEBUG
    if (dwError != 0)
    {
        WCHAR szError[256] = {};
        mciGetErrorString(dwError, szError, 256);
        wstring strError = L"MCI Error: " + wstring(szError) + L" (Command: " + _strCommand + L")";
        OutputDebugString(strError.c_str());
    }
#endif
}

wstring CSound::GetAliasName() const
{
    return m_strAlias;
}