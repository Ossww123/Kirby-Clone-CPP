#pragma once

// 전방 선언
class CAnimator;
class CAnimation;
class CTexture;

class CAnimationDataMgr
{
    SINGLE(CAnimationDataMgr);

public:
    // === 매니저 초기화 함수 ===
    void init();

public:
    // === 파일 로딩 ===
    tAnimationFileData LoadAnimationFile(const wstring& _strFilePath);
    bool SaveAnimationFile(const wstring& _strFilePath, const tAnimationFileData& _data);

public:
    // === 애니메이터에 로딩 (CAnimator 직접) ===
    void LoadAnimationsIntoAnimator(CAnimator* _pAnimator, const wstring& _strFilePath);

public:
    // === 유틸리티 함수 모음 ===
    CAnimation* CreateTemporaryAnimation(const tAnimationData& _data, CTexture* _pTexture);
    vector<wstring> GetAllAnimationFiles(const wstring& _strDirectory);
    bool IsValidAnimationFile(const wstring& _strFilePath);

public:
    // === 유틸리티 함수 ===
    tAnimationData ConvertFromCAnimation(CAnimation* _pAnim);
    wstring GetAnimationDirectory() const { return m_strAnimationDir; }
    void SetAnimationDirectory(const wstring& _strDir) { m_strAnimationDir = _strDir; }

    // 테스트용 함수
    bool CreateSampleAnimationFile(const wstring& _strFileName);
    bool TestLoadAnimationFile(const wstring& _strFileName);
    bool TestDirectLoad();  // 직접 로드 테스트 추가

private:
    // === JSON 파싱 관련 내부 함수 ===
    tAnimationFileData ParseJsonToAnimationData(const wstring& _strJsonContent);
    wstring SerializeAnimationDataToJson(const tAnimationFileData& _data);

    // JSON 파싱 헬퍼 함수
    wstring ExtractNumberFromJson(const wstring& _content, size_t _startPos);
    wstring ExtractStringFromJson(const wstring& _content, size_t _startPos);
    Vec2 ExtractVec2FromJson(const wstring& _content, size_t _startPos);
    void ParseAnimationsSection(const wstring& _content, size_t _startPos, tAnimationFileData& _result);
    tAnimationData ParseSingleAnimation(const wstring& _content, size_t _startPos, size_t _endPos);
    tAnimFrame ParseSingleFrame(const wstring& _content, size_t _startPos, size_t _endPos);

private:
    // === 파일 처리 관련 함수 ===
    wstring ReadTextFile(const wstring& _strFilePath);
    bool WriteTextFile(const wstring& _strFilePath, const wstring& _strContent);
    wstring GetFullPath(const wstring& _strRelativePath);

private:
    // === 유효성 검사 ===
    bool ValidateAnimationData(const tAnimationFileData& _data);
    bool ValidateFrameData(const tAnimFrame& _frameData);

private:
    // === 베이스 변수들 ===
    wstring m_strAnimationDir;      // 애니메이션 파일들이 저장된 디렉토리

    // 캐싱 (최적화용 - 나중에 구현 예정)
    map<wstring, tAnimationFileData> m_mapCachedData;
    bool m_bUseCaching;
};