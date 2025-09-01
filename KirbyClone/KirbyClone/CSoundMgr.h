#pragma once
#include "CSound.h"

class CSoundMgr
{
    SINGLE(CSoundMgr);

private:
    map<wstring, CSound*> m_mapSound;   // 사운드 리소스 맵
    wstring m_strCurrentBGM;            // 현재 재생 중인 BGM 키
    int m_iBGMVolume;                   // BGM 볼륨 (0~1000)
    int m_iSFXVolume;                   // SFX 볼륨 (0~1000)

public:
    void init();
    void Release();

    // === 사운드 로드 및 관리 ===
    CSound* LoadSound(const wstring& _strKey, const wstring& _strPath, SOUND_TYPE _eType);
    CSound* FindSound(const wstring& _strKey);

    // === BGM 관리 ===
    void PlayBGM(const wstring& _strKey, bool _bLoop = true);
    void StopBGM();
    void PauseBGM();
    void ResumeBGM();

    // === SFX 관리 ===
    void PlaySFX(const wstring& _strKey);
    void PlaySFXLoop(const wstring& _strKey);  // SFX 루프 재생
    void StopSFX(const wstring& _strKey);      // 특정 SFX 정지
    void StopAllSFX();

    // === 볼륨 제어 ===
    void SetBGMVolume(int _iVolume);    // 0~1000
    void SetSFXVolume(int _iVolume);    // 0~1000
    void SetMasterVolume(int _iVolume); // 모든 사운드 볼륨

    // === 유틸리티 ===
    void StopAll();                     // 모든 사운드 정지
};

