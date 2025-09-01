#pragma once
#include "CRes.h"

enum class SOUND_TYPE
{
    BGM,    // 배경음악 (한 번에 하나만)
    SFX,    // 효과음 (다중 재생 가능)
    END
};

class CSound : public CRes
{
public:
    CSound();
    virtual ~CSound();

public:
    // === 사운드 로드 및 해제 ===
    bool Load(const wstring& _strPath);
    void Release();

    // === 사운드 재생 제어 ===
    void Play(bool _bLoop = false);
    void Stop();
    void Pause();
    void Resume();

    // === 사운드 설정 ===
    void SetVolume(int _iVolume);      // 0~1000 (mci 기준)
    void SetType(SOUND_TYPE _eType) { m_eType = _eType; }

    // === 상태 확인 ===
    bool IsPlaying() const;
    SOUND_TYPE GetType() const { return m_eType; }

private:
    // === mciSendString 헬퍼 함수들 ===
    void SendMCICommand(const wstring& _strCommand);
    wstring GetAliasName() const;      // 고유한 별칭 생성

private:
    // === 사운드 정보 ===
    SOUND_TYPE  m_eType;               // 사운드 타입
    bool        m_bLoop;               // 루프 재생 여부
    bool        m_bLoaded;             // 로드 완료 여부
    int         m_iVolume;             // 볼륨 (0~1000)
    wstring     m_strAlias;            // MCI 별칭
    
    // === 정적 카운터 ===
    static int  s_iSoundCount;         // 고유 별칭 생성용
};