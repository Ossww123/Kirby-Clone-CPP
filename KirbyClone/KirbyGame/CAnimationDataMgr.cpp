#include "gamePCH.h"
#include "CAnimationDataMgr.h"
#include "CAnimator.h"
#include "CAnimation.h"
#include "CTexture.h"
#include "CResMgr.h"
#include "CPathMgr.h"

CAnimationDataMgr::CAnimationDataMgr()
    : m_strAnimationDir(L"content\\animation\\")
    , m_bUseCaching(false)
{
}

CAnimationDataMgr::~CAnimationDataMgr()
{
}

// === 핵심 생명주기 함수들 ===
void CAnimationDataMgr::init()
{
    // 애니메이션 디렉토리 경로 설정
    wstring strContentPath = CPathMgr::GetInst()->GetContentPath();
    m_strAnimationDir = strContentPath + L"animation\\";

    // 디렉토리가 없으면 생성
    CreateDirectory(m_strAnimationDir.c_str(), nullptr);
}

// === 파일 입출력 ===
tAnimationFileData CAnimationDataMgr::LoadAnimationFile(const wstring& _strFilePath)
{
    tAnimationFileData result;

    // 캐시 확인
    if (m_bUseCaching)
    {
        auto iter = m_mapCachedData.find(_strFilePath);
        if (iter != m_mapCachedData.end())
        {
            return iter->second;
        }
    }

    // 파일 읽기
    wstring strJsonContent = ReadTextFile(_strFilePath);
    if (strJsonContent.empty())
    {
        return result;
    }

    // JSON 파싱
    result = ParseJsonToAnimationData(strJsonContent);

    // 유효성 검사
    if (!ValidateAnimationData(result))
    {
        result = tAnimationFileData(); // 빈 데이터 반환
        return result;
    }

    // 캐시 저장
    if (m_bUseCaching)
    {
        m_mapCachedData[_strFilePath] = result;
    }

    return result;
}

bool CAnimationDataMgr::SaveAnimationFile(const wstring& _strFilePath, const tAnimationFileData& _data)
{
    // 유효성 검사
    if (!ValidateAnimationData(_data))
    {
        return false;
    }

    // JSON 직렬화
    wstring strJsonContent = SerializeAnimationDataToJson(_data);
    if (strJsonContent.empty())
    {
        return false;
    }

    // 파일 쓰기
    bool bSuccess = WriteTextFile(_strFilePath, strJsonContent);

    // 캐시 업데이트
    if (bSuccess && m_bUseCaching)
    {
        m_mapCachedData[_strFilePath] = _data;
    }

    return bSuccess;
}

// === 게임 런타임 지원 ===
void CAnimationDataMgr::LoadAnimationsIntoAnimator(CAnimator* _pAnimator, const wstring& _strFilePath)
{
    if (!_pAnimator) return;

    tAnimationFileData fileData = LoadAnimationFile(_strFilePath);
    if (fileData.mapAnimations.empty()) return;

    // 텍스처(시트) 로드
    CTexture* pTexture = CResMgr::GetInst()->LoadTexture(fileData.strTexturePath, fileData.strTexturePath);
    if (!pTexture) return;

    // 각 애니메이션을 Animator에 주입 (반드시 SetSheet 사용)
    for (auto& pair : fileData.mapAnimations)
    {
        const wstring& name = pair.first;
        const tAnimationData& animData = pair.second;

        CAnimation* pAnim = CreateTemporaryAnimation(animData, pTexture);
        if (pAnim) _pAnimator->AddCustomAnimation(name, pAnim);
    }
}

// === 에디터 지원 기능들 ===
CAnimation* CAnimationDataMgr::CreateTemporaryAnimation(const tAnimationData& _data, CTexture* _pTexture)
{
    if (!_pTexture || _data.vecFrames.empty())
        return nullptr;

    // CAnimation 객체 생성
    CAnimation* pAnim = new CAnimation;
    pAnim->SetName(_data.strName);
    pAnim->SetSheet(_pTexture);
    pAnim->SetLoop(_data.bLoop);

    // 프레임 데이터 추가
    for (const auto& frameData : _data.vecFrames)
    {
        pAnim->AddFrame(frameData.vLT, frameData.vSlice, frameData.fDuration);
    }

    return pAnim;
}

vector<wstring> CAnimationDataMgr::GetAllAnimationFiles(const wstring& _strDirectory)
{
    vector<wstring> result;

    // 검색할 디렉토리 경로 구성
    wstring searchPath = _strDirectory;
    if (searchPath.empty())
    {
        searchPath = m_strAnimationDir;
    }

    // 와일드카드 패턴 추가
    if (searchPath.back() != L'\\')
    {
        searchPath += L"\\";
    }
    searchPath += L"*.json";

    // Windows API로 파일 검색
    WIN32_FIND_DATA findData;
    HANDLE hFind = FindFirstFile(searchPath.c_str(), &findData);

    if (hFind != INVALID_HANDLE_VALUE)
    {
        do
        {
            // 디렉토리가 아닌 파일만 추가
            if (!(findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
            {
                wstring fileName = findData.cFileName;

                // 전체 경로 구성
                wstring fullPath = _strDirectory;
                if (fullPath.empty())
                {
                    fullPath = m_strAnimationDir;
                }
                if (fullPath.back() != L'\\')
                {
                    fullPath += L"\\";
                }
                fullPath += fileName;

                result.push_back(fullPath);
            }
        } while (FindNextFile(hFind, &findData));

        FindClose(hFind);
    }

    return result;
}

bool CAnimationDataMgr::IsValidAnimationFile(const wstring& _strFilePath)
{
    // 파일 존재 확인
    DWORD dwAttrib = GetFileAttributes(_strFilePath.c_str());
    if (dwAttrib == INVALID_FILE_ATTRIBUTES)
        return false;

    // 확장자 확인
    size_t dotPos = _strFilePath.find_last_of(L'.');
    if (dotPos == wstring::npos)
        return false;

    wstring extension = _strFilePath.substr(dotPos);
    return (extension == L".json" || extension == L".anim");
}

// 테스트용 함수들
bool CAnimationDataMgr::CreateSampleAnimationFile(const wstring& _strFileName)
{
    // 샘플 애니메이션 파일 데이터 생성
    tAnimationFileData sampleData;
    sampleData.strTexturePath = L"texture\\kirby\\kirby.bmp";  // 올바른 경로로 수정
    sampleData.vSpriteSize = Vec2(32.f, 32.f);
    sampleData.iBorder = 1;

    // IDLE 애니메이션 생성
    tAnimationData idleAnim;
    idleAnim.strName = L"IDLE";
    idleAnim.bLoop = true;

    // IDLE 프레임들
    tAnimFrame frame1(Vec2(1.f, 1.f), Vec2(32.f, 32.f), 2.0f);      // 2초 대기
    tAnimFrame frame2(Vec2(33.f, 1.f), Vec2(32.f, 32.f), 0.1f);     // 깜빡임
    tAnimFrame frame3(Vec2(1.f, 1.f), Vec2(32.f, 32.f), 1.0f);      // 1초 대기
    tAnimFrame frame4(Vec2(33.f, 1.f), Vec2(32.f, 32.f), 0.1f);     // 깜빡임

    idleAnim.vecFrames.push_back(frame1);
    idleAnim.vecFrames.push_back(frame2);
    idleAnim.vecFrames.push_back(frame3);
    idleAnim.vecFrames.push_back(frame4);

    sampleData.mapAnimations[L"IDLE"] = idleAnim;

    // 파일 저장
    wstring fullPath = GetFullPath(_strFileName);
    return SaveAnimationFile(fullPath, sampleData);
}

bool CAnimationDataMgr::TestLoadAnimationFile(const wstring& _strFileName)
{
    MessageBox(nullptr, (L"Testing load of: " + _strFileName).c_str(), L"Debug", MB_OK);

    // 파일 로드 테스트
    tAnimationFileData loadedData = LoadAnimationFile(_strFileName);

    // 로드 결과 확인
    bool bSuccess = true;
    wstring errorMsg = L"";

    // 기본 정보 확인
    if (loadedData.strTexturePath.empty())
    {
        bSuccess = false;
        errorMsg += L"Texture path is empty\n";
    }

    if (loadedData.mapAnimations.empty())
    {
        bSuccess = false;
        errorMsg += L"No animations found\n";
    }

    // 결과 출력 (디버그용)
    if (bSuccess)
    {
        wstring msg = L"Animation file loaded successfully!\n";
        msg += L"Texture: " + loadedData.strTexturePath + L"\n";
        msg += L"Animations: " + to_wstring(loadedData.mapAnimations.size()) + L"\n";

        for (const auto& pair : loadedData.mapAnimations)
        {
            msg += L"- " + pair.first + L" (" + to_wstring(pair.second.vecFrames.size()) + L" frames)\n";
        }

        MessageBox(nullptr, msg.c_str(), L"Animation Load Test", MB_OK);
    }
    else
    {
        wstring msg = L"Animation file load failed!\n" + errorMsg;
        MessageBox(nullptr, msg.c_str(), L"Animation Load Test", MB_OK | MB_ICONERROR);
    }

    return bSuccess;
}

bool CAnimationDataMgr::TestDirectLoad()
{
    // 직접 로드 테스트
    wstring testPath = m_strAnimationDir + L"test_direct.json";

    // 테스트 파일이 존재하는지 확인
    DWORD dwAttrib = GetFileAttributes(testPath.c_str());
    if (dwAttrib == INVALID_FILE_ATTRIBUTES)
    {
        return false;
    }

    // 로드 시도
    tAnimationFileData data = LoadAnimationFile(testPath);
    return !data.mapAnimations.empty();
}

// === JSON 파싱 관련 내부 구현 ===
tAnimationFileData CAnimationDataMgr::ParseJsonToAnimationData(const wstring& _strJsonContent)
{
    tAnimationFileData result;

    // 텍스처 경로 파싱 (키 이름 수정)
    size_t texturePos = _strJsonContent.find(L"\"texturePath\"");
    if (texturePos != wstring::npos)
    {
        result.strTexturePath = ExtractStringFromJson(_strJsonContent, texturePos);
    }

    // 스프라이트 크기 파싱 (키 이름 수정)
    size_t spriteSizePos = _strJsonContent.find(L"\"spriteSize\"");
    if (spriteSizePos != wstring::npos)
    {
        result.vSpriteSize = ExtractVec2FromJson(_strJsonContent, spriteSizePos);
    }

    // 보더 값 파싱
    size_t borderPos = _strJsonContent.find(L"\"border\"");
    if (borderPos != wstring::npos)
    {
        wstring borderStr = ExtractNumberFromJson(_strJsonContent, borderPos);
        result.iBorder = _wtoi(borderStr.c_str());
    }

    // 애니메이션 섹션 파싱
    size_t animationsPos = _strJsonContent.find(L"\"animations\"");
    if (animationsPos != wstring::npos)
    {
        ParseAnimationsSection(_strJsonContent, animationsPos, result);
    }

    return result;
}

wstring CAnimationDataMgr::SerializeAnimationDataToJson(const tAnimationFileData& _data)
{
    wstring result = L"{\n";

    // 텍스처 경로 (키 이름 수정)
    result += L"  \"texturePath\": \"" + _data.strTexturePath + L"\",\n";

    // 스프라이트 크기 (키 이름 수정)
    result += L"  \"spriteSize\": {\n";
    result += L"    \"x\": " + to_wstring(_data.vSpriteSize.x) + L",\n";
    result += L"    \"y\": " + to_wstring(_data.vSpriteSize.y) + L"\n";
    result += L"  },\n";

    // 보더
    result += L"  \"border\": " + to_wstring(_data.iBorder) + L",\n";

    // 애니메이션들
    result += L"  \"animations\": {\n";

    size_t animCount = 0;
    for (const auto& pair : _data.mapAnimations)
    {
        if (animCount > 0) result += L",\n";

        const wstring& animName = pair.first;
        const tAnimationData& animData = pair.second;

        result += L"    \"" + animName + L"\": {\n";
        result += L"      \"loop\": " + (animData.bLoop ? wstring(L"true") : wstring(L"false")) + L",\n";
        result += L"      \"frames\": [\n";

        for (size_t i = 0; i < animData.vecFrames.size(); ++i)
        {
            if (i > 0) result += L",\n";

            const tAnimFrame& frame = animData.vecFrames[i];
            result += L"        {\n";
            result += L"          \"vLT\": { \"x\": " + to_wstring(frame.vLT.x) + L", \"y\": " + to_wstring(frame.vLT.y) + L" },\n";
            result += L"          \"vSlice\": { \"x\": " + to_wstring(frame.vSlice.x) + L", \"y\": " + to_wstring(frame.vSlice.y) + L" },\n";
            result += L"          \"fDuration\": " + to_wstring(frame.fDuration) + L"\n";
            result += L"        }";
        }

        result += L"\n      ]\n";
        result += L"    }";
        animCount++;
    }

    result += L"\n  }\n";
    result += L"}";

    return result;
}

// JSON 파싱 도우미 함수들
wstring CAnimationDataMgr::ExtractNumberFromJson(const wstring& _content, size_t _startPos)
{
    size_t colonPos = _content.find(L':', _startPos);
    if (colonPos == wstring::npos) return L"";

    size_t numberStart = colonPos + 1;
    while (numberStart < _content.length() && (_content[numberStart] == L' ' || _content[numberStart] == L'\t' || _content[numberStart] == L'\n'))
        numberStart++;

    size_t numberEnd = numberStart;
    while (numberEnd < _content.length() &&
        (_content[numberEnd] >= L'0' && _content[numberEnd] <= L'9' ||
            _content[numberEnd] == L'.' || _content[numberEnd] == L'-'))
        numberEnd++;

    return _content.substr(numberStart, numberEnd - numberStart);
}

wstring CAnimationDataMgr::ExtractStringFromJson(const wstring& _content, size_t _startPos)
{
    size_t colonPos = _content.find(L':', _startPos);
    if (colonPos == wstring::npos) return L"";

    size_t quoteStart = _content.find(L'"', colonPos);
    if (quoteStart == wstring::npos) return L"";

    size_t quoteEnd = _content.find(L'"', quoteStart + 1);
    if (quoteEnd == wstring::npos) return L"";

    return _content.substr(quoteStart + 1, quoteEnd - quoteStart - 1);
}

Vec2 CAnimationDataMgr::ExtractVec2FromJson(const wstring& _content, size_t _startPos)
{
    Vec2 result;

    // 중괄호 범위 찾기 { ... }
    size_t braceStart = _content.find(L'{', _startPos);
    if (braceStart == wstring::npos) return result;

    size_t braceEnd = _content.find(L'}', braceStart);
    if (braceEnd == wstring::npos) return result;

    // 중괄호 안의 내용만 추출
    wstring vec2Content = _content.substr(braceStart, braceEnd - braceStart + 1);

    // x 값 추출 (중괄호 범위 내에서만)
    size_t xPos = vec2Content.find(L"\"x\"");
    if (xPos != wstring::npos)
    {
        wstring xStr = ExtractNumberFromJson(vec2Content, xPos);
        result.x = (float)_wtof(xStr.c_str());
    }

    // y 값 추출 (중괄호 범위 내에서만)
    size_t yPos = vec2Content.find(L"\"y\"");
    if (yPos != wstring::npos)
    {
        wstring yStr = ExtractNumberFromJson(vec2Content, yPos);
        result.y = (float)_wtof(yStr.c_str());
    }

    return result;
}

void CAnimationDataMgr::ParseAnimationsSection(const wstring& _content, size_t _startPos, tAnimationFileData& _result)
{
    size_t braceStart = _content.find(L'{', _startPos);
    if (braceStart == wstring::npos) return;

    size_t braceEnd = braceStart + 1;
    int braceCount = 1;

    // 중첩된 중괄호 처리
    while (braceEnd < _content.length() && braceCount > 0)
    {
        if (_content[braceEnd] == L'{') braceCount++;
        else if (_content[braceEnd] == L'}') braceCount--;
        braceEnd++;
    }

    wstring animSection = _content.substr(braceStart + 1, braceEnd - braceStart - 2);

    // 각 애니메이션 파싱
    size_t searchPos = 0;
    int foundAnimations = 0;

    while (searchPos < animSection.length())
    {
        size_t quoteStart = animSection.find(L'"', searchPos);
        if (quoteStart == wstring::npos) break;

        size_t quoteEnd = animSection.find(L'"', quoteStart + 1);
        if (quoteEnd == wstring::npos) break;

        wstring animName = animSection.substr(quoteStart + 1, quoteEnd - quoteStart - 1);

        size_t colonPos = animSection.find(L':', quoteEnd);
        if (colonPos == wstring::npos) break;

        size_t animDataStart = animSection.find(L'{', colonPos);
        if (animDataStart == wstring::npos) break;

        // 애니메이션 데이터 블록의 끝 찾기
        size_t animDataEnd = animDataStart + 1;
        int innerBraceCount = 1;
        while (animDataEnd < animSection.length() && innerBraceCount > 0)
        {
            if (animSection[animDataEnd] == L'{') innerBraceCount++;
            else if (animSection[animDataEnd] == L'}') innerBraceCount--;
            animDataEnd++;
        }

        if (innerBraceCount == 0)
        {
            // 단일 애니메이션 파싱
            tAnimationData animData = ParseSingleAnimation(animSection, animDataStart, animDataEnd);
            animData.strName = animName;
            _result.mapAnimations[animName] = animData;
            foundAnimations++;
        }

        searchPos = animDataEnd;
    }
}

tAnimationData CAnimationDataMgr::ParseSingleAnimation(const wstring& _content, size_t _startPos, size_t _endPos)
{
    tAnimationData result;

    wstring animContent = _content.substr(_startPos, _endPos - _startPos);

    // loop 파싱
    size_t loopPos = animContent.find(L"\"loop\"");
    if (loopPos != wstring::npos)
    {
        size_t truePos = animContent.find(L"true", loopPos);
        size_t falsePos = animContent.find(L"false", loopPos);

        if (truePos != wstring::npos && (falsePos == wstring::npos || truePos < falsePos))
        {
            result.bLoop = true;
        }
        else
        {
            result.bLoop = false;
        }
    }

    // frames 배열 파싱
    size_t framesPos = animContent.find(L"\"frames\"");
    if (framesPos != wstring::npos)
    {
        size_t arrayStart = animContent.find(L'[', framesPos);
        size_t arrayEnd = animContent.find(L']', framesPos);

        if (arrayStart != wstring::npos && arrayEnd != wstring::npos)
        {
            wstring framesContent = animContent.substr(arrayStart + 1, arrayEnd - arrayStart - 1);

            // 중첩된 중괄호 처리로 프레임 파싱
            size_t searchPos = 0;
            while (searchPos < framesContent.length())
            {
                size_t frameStart = framesContent.find(L'{', searchPos);
                if (frameStart == wstring::npos) break;

                // 중첩된 중괄호 끝 찾기
                size_t frameEnd = frameStart + 1;
                int braceCount = 1;
                while (frameEnd < framesContent.length() && braceCount > 0)
                {
                    if (framesContent[frameEnd] == L'{') braceCount++;
                    else if (framesContent[frameEnd] == L'}') braceCount--;
                    frameEnd++;
                }

                if (braceCount == 0)
                {
                    tAnimFrame frame = ParseSingleFrame(framesContent, frameStart, frameEnd);
                    result.vecFrames.push_back(frame);
                }

                searchPos = frameEnd;
            }
        }
    }

    return result;
}

tAnimFrame CAnimationDataMgr::ParseSingleFrame(const wstring& _content, size_t _startPos, size_t _endPos)
{
    tAnimFrame result; // vOffset은 {0,0} 기본

    wstring frameContent = _content.substr(_startPos, _endPos - _startPos);

    size_t vltPos = frameContent.find(L"\"vLT\"");
    if (vltPos != wstring::npos) result.vLT = ExtractVec2FromJson(frameContent, vltPos);

    size_t vslicePos = frameContent.find(L"\"vSlice\"");
    if (vslicePos != wstring::npos) result.vSlice = ExtractVec2FromJson(frameContent, vslicePos);

    size_t durationPos = frameContent.find(L"\"fDuration\"");
    if (durationPos != wstring::npos)
    {
        wstring d = ExtractNumberFromJson(frameContent, durationPos);
        result.fDuration = (float)_wtof(d.c_str());
    }

    size_t voffPos = frameContent.find(L"\"vOffset\"");
    if (voffPos != wstring::npos) result.vOffset = ExtractVec2FromJson(frameContent, voffPos);

    return result;
}


// === 파일 처리 내부 구현 ===
wstring CAnimationDataMgr::ReadTextFile(const wstring& _strFilePath)
{
    wstring strFullPath = GetFullPath(_strFilePath);

    HANDLE hFile = CreateFile(strFullPath.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (hFile == INVALID_HANDLE_VALUE)
        return L"";

    DWORD dwFileSize = GetFileSize(hFile, nullptr);
    if (dwFileSize == 0)
    {
        CloseHandle(hFile);
        return L"";
    }

    // UTF-8 데이터 읽기
    vector<char> buffer(dwFileSize + 1);
    DWORD dwBytesRead = 0;
    bool bReadResult = ReadFile(hFile, buffer.data(), dwFileSize, &dwBytesRead, nullptr);
    CloseHandle(hFile);

    if (!bReadResult || dwBytesRead == 0)
        return L"";

    buffer[dwBytesRead] = '\0';

    // UTF-8을 Wide String으로 변환
    int wideSize = MultiByteToWideChar(CP_UTF8, 0, buffer.data(), dwBytesRead, nullptr, 0);
    if (wideSize == 0)
        return L"";

    wstring result(wideSize, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, buffer.data(), dwBytesRead, &result[0], wideSize);

    return result;
}

bool CAnimationDataMgr::WriteTextFile(const wstring& _strFilePath, const wstring& _strContent)
{
    wstring strFullPath = GetFullPath(_strFilePath);

    // Wide String을 UTF-8로 변환
    int utf8Size = WideCharToMultiByte(CP_UTF8, 0, _strContent.c_str(), -1, nullptr, 0, nullptr, nullptr);
    if (utf8Size == 0)
        return false;

    vector<char> utf8Buffer(utf8Size);
    int convertResult = WideCharToMultiByte(CP_UTF8, 0, _strContent.c_str(), -1, utf8Buffer.data(), utf8Size, nullptr, nullptr);
    if (convertResult == 0)
        return false;

    HANDLE hFile = CreateFile(strFullPath.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (hFile == INVALID_HANDLE_VALUE)
        return false;

    DWORD dwBytesWritten = 0;
    bool bWriteResult = WriteFile(hFile, utf8Buffer.data(), utf8Size - 1, &dwBytesWritten, nullptr); // -1로 null terminator 제외
    CloseHandle(hFile);

    bool bSuccess = bWriteResult && (dwBytesWritten == utf8Size - 1);

    return bSuccess;
}

wstring CAnimationDataMgr::GetFullPath(const wstring& _strRelativePath)
{
    // 상대 경로면 애니메이션 디렉토리와 합치기
    if (_strRelativePath.find(L':') == wstring::npos) // 절대 경로가 아니면
    {
        return m_strAnimationDir + _strRelativePath;
    }

    return _strRelativePath;
}

// === 유효성 검사 ===
bool CAnimationDataMgr::ValidateAnimationData(const tAnimationFileData& _data)
{
    // 텍스처 경로 확인
    if (_data.strTexturePath.empty())
    {
        return false;
    }

    // 애니메이션이 하나라도 있는지 확인
    if (_data.mapAnimations.empty())
    {
        return false;
    }

    // 각 애니메이션 유효성 검사
    for (const auto& pair : _data.mapAnimations)
    {
        const tAnimationData& animData = pair.second;

        // 프레임이 있는지 확인
        if (animData.vecFrames.empty())
        {
            return false;
        }

        // 각 프레임 유효성 검사
        for (size_t i = 0; i < animData.vecFrames.size(); ++i)
        {
            if (!ValidateFrameData(animData.vecFrames[i]))
            {
                return false;
            }
        }
    }

    return true;
}

bool CAnimationDataMgr::ValidateFrameData(const tAnimFrame& _frameData)
{
    // 크기가 0보다 큰지 확인
    if (_frameData.vSlice.x <= 0 || _frameData.vSlice.y <= 0)
    {
        return false;
    }

    // 지속시간이 양수인지 확인
    if (_frameData.fDuration <= 0.f)
    {
        return false;
    }

    return true;
}