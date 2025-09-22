#include "gamePCH.h"
#include "CPathMgr.h"
#include "CCore.h"

CPathMgr::CPathMgr()
    : m_strContentPath{}
    , m_strRelativePath{}
{
}

CPathMgr::~CPathMgr()
{
}

void CPathMgr::init()
{
    // 실행 파일 경로를 얻어온다
    wchar_t szBuffer[255] = {};
    GetCurrentDirectory(255, szBuffer);

    // 실행 파일이 있는 경로에서 상위로 이동하여 Content 폴더를 찾는다
    m_strContentPath = szBuffer;

    // KirbyGame 프로젝트 폴더에서 상위로 이동하여 루트 폴더로 가기
    size_t iFind = m_strContentPath.rfind(L"\\KirbyGame");
    if (iFind != wstring::npos)
    {
        m_strContentPath = m_strContentPath.substr(0, iFind);
    }

    // Content 폴더 경로 추가
    m_strContentPath += L"\\bin\\content\\";

    // 상대 경로 저장 (절대 경로에서 Content 경로를 뺀 나머지)
    m_strRelativePath = m_strContentPath;
}

wstring CPathMgr::GetRelativePath(const wstring& _strFilePath)
{
    // 파일의 절대 경로에서 Content 경로 부분을 제거하여 상대 경로만 반환
    size_t iFind = _strFilePath.find(m_strContentPath);
    if (iFind != wstring::npos)
    {
        return _strFilePath.substr(m_strContentPath.length());
    }

    return _strFilePath;
}