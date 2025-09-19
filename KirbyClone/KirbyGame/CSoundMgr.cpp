#include "pch.h"
#include "CSoundMgr.h"
#include "CPathMgr.h"

CSoundMgr::CSoundMgr()
    : m_strCurrentBGM(L"")
    , m_iBGMVolume(1000)
    , m_iSFXVolume(1000)
{
}

CSoundMgr::~CSoundMgr()
{
    Release();
}

void CSoundMgr::init()
{
    m_strCurrentBGM = L"";
    m_iBGMVolume = 1000;
    m_iSFXVolume = 1000;
}

void CSoundMgr::Release()
{
    StopAll();

    for (auto& pair : m_mapSound)
    {
        delete pair.second;
    }
    m_mapSound.clear();
    m_strCurrentBGM = L"";
}

CSound* CSoundMgr::LoadSound(const wstring& _strKey, const wstring& _strPath, SOUND_TYPE _eType)
{
    CSound* pSound = FindSound(_strKey);
    if (pSound)
        return pSound;

    pSound = new CSound();
    pSound->SetType(_eType);
    
    // CPathMgr을 통해 절대 경로 구성
    wstring strFullPath = CPathMgr::GetInst()->GetContentPath() + _strPath;
    
    if (!pSound->Load(strFullPath))
    {
        delete pSound;
        return nullptr;
    }

    pSound->SetVolume(_eType == SOUND_TYPE::BGM ? m_iBGMVolume : m_iSFXVolume);
    m_mapSound.insert(make_pair(_strKey, pSound));
    
    return pSound;
}

CSound* CSoundMgr::FindSound(const wstring& _strKey)
{
    auto iter = m_mapSound.find(_strKey);
    if (iter == m_mapSound.end())
        return nullptr;
        
    return iter->second;
}

void CSoundMgr::PlayBGM(const wstring& _strKey, bool _bLoop)
{
    if (m_strCurrentBGM == _strKey)
        return;

    StopBGM();

    CSound* pSound = FindSound(_strKey);
    if (pSound && pSound->GetType() == SOUND_TYPE::BGM)
    {
        pSound->Play(_bLoop);
        m_strCurrentBGM = _strKey;
    }
}

void CSoundMgr::StopBGM()
{
    if (!m_strCurrentBGM.empty())
    {
        CSound* pSound = FindSound(m_strCurrentBGM);
        if (pSound)
        {
            pSound->Stop();
        }
        m_strCurrentBGM = L"";
    }
}

void CSoundMgr::PauseBGM()
{
    if (!m_strCurrentBGM.empty())
    {
        CSound* pSound = FindSound(m_strCurrentBGM);
        if (pSound)
        {
            pSound->Pause();
        }
    }
}

void CSoundMgr::ResumeBGM()
{
    if (!m_strCurrentBGM.empty())
    {
        CSound* pSound = FindSound(m_strCurrentBGM);
        if (pSound)
        {
            pSound->Resume();
        }
    }
}

void CSoundMgr::PlaySFX(const wstring& _strKey)
{
    CSound* pSound = FindSound(_strKey);
    if (pSound && pSound->GetType() == SOUND_TYPE::SFX)
    {
        pSound->Play(false);
    }
}

void CSoundMgr::PlaySFXLoop(const wstring& _strKey)
{
    CSound* pSound = FindSound(_strKey);
    if (pSound && pSound->GetType() == SOUND_TYPE::SFX)
    {
        pSound->Play(true);
    }
}

void CSoundMgr::StopSFX(const wstring& _strKey)
{
    CSound* pSound = FindSound(_strKey);
    if (pSound && pSound->GetType() == SOUND_TYPE::SFX)
    {
        pSound->Stop();
    }
}

void CSoundMgr::StopAllSFX()
{
    for (auto& pair : m_mapSound)
    {
        if (pair.second->GetType() == SOUND_TYPE::SFX && pair.second->IsPlaying())
        {
            pair.second->Stop();
        }
    }
}

void CSoundMgr::SetBGMVolume(int _iVolume)
{
    m_iBGMVolume = max(0, min(1000, _iVolume));
    
    for (auto& pair : m_mapSound)
    {
        if (pair.second->GetType() == SOUND_TYPE::BGM)
        {
            pair.second->SetVolume(m_iBGMVolume);
        }
    }
}

void CSoundMgr::SetSFXVolume(int _iVolume)
{
    m_iSFXVolume = max(0, min(1000, _iVolume));
    
    for (auto& pair : m_mapSound)
    {
        if (pair.second->GetType() == SOUND_TYPE::SFX)
        {
            pair.second->SetVolume(m_iSFXVolume);
        }
    }
}

void CSoundMgr::SetMasterVolume(int _iVolume)
{
    SetBGMVolume(_iVolume);
    SetSFXVolume(_iVolume);
}

void CSoundMgr::StopAll()
{
    for (auto& pair : m_mapSound)
    {
        if (pair.second->IsPlaying())
        {
            pair.second->Stop();
        }
    }
    m_strCurrentBGM = L"";
}
