#include "pch.h"
#include "CAnimator.h"
#include "CAnimation.h"
#include "CObject.h"
#include "CCore.h"  // 새로 추가: GetPixelScale() 사용

CAnimator::CAnimator()
    : m_pOwner(nullptr)
    , m_pCurAnim(nullptr)
    , m_bRepeat(false)
{
}

CAnimator::~CAnimator()
{
    // 맵이 유효한지 먼저 확인
    if (!m_mapAnim.empty())
    {
        // 안전한 방법으로 삭제
        for (map<wstring, CAnimation*>::iterator iter = m_mapAnim.begin();
            iter != m_mapAnim.end(); )
        {
            if (iter->second != nullptr)
            {
                delete iter->second;
                iter->second = nullptr;
            }
            iter = m_mapAnim.erase(iter);  // erase 후 다음 iterator 반환
        }
    }
}

void CAnimator::CreateAnimation(const wstring& _strName, CTexture* _pTex, Vec2 _vLT, Vec2 _vSliceSize, Vec2 _vStep, float _fDuration, int _iFrameCount, bool _bLoop)
{
    CAnimation* pAnim = FindAnimation(_strName);
    if (nullptr != pAnim)
        return; // 이미 존재하는 애니메이션

    pAnim = new CAnimation;
    pAnim->SetName(_strName);
    pAnim->SetTexture(_pTex);
    pAnim->Create(_pTex, _vLT, _vSliceSize, _vStep, _fDuration, _iFrameCount, _bLoop);

    m_mapAnim.insert(make_pair(_strName, pAnim));
}

void CAnimator::LoadAnimation(const wstring& _strRelativePath)
{
    // TODO: 파일에서 애니메이션 정보 로드
}

void CAnimator::SaveAnimation(const wstring& _strRelativePath)
{
    // TODO: 애니메이션 정보를 파일로 저장
}

void CAnimator::Play(const wstring& _strName, bool _bRepeat)
{
    m_pCurAnim = FindAnimation(_strName);
    m_bRepeat = _bRepeat;

    if (nullptr != m_pCurAnim)
    {
        m_pCurAnim->Reset();
    }
}

void CAnimator::Update()
{
    if (nullptr != m_pCurAnim)
    {
        m_pCurAnim->Update();

        // 애니메이션이 끝나고 반복이 아니라면
        if (m_pCurAnim->IsFinish() && !m_bRepeat)
        {
            m_pCurAnim = nullptr;
        }
    }
}

void CAnimator::Render(HDC _dc)
{
    if (nullptr != m_pCurAnim && nullptr != m_pOwner)
    {
        Vec2 vPos = m_pOwner->GetPos();
        float fScale = CCore::GetPixelScale(); // 4배 스케일 적용

        // 스케일이 적용된 렌더링 호출
        m_pCurAnim->RenderScaled(_dc, vPos, fScale);
    }
}

CAnimation* CAnimator::FindAnimation(const wstring& _strName)
{
    auto iter = m_mapAnim.find(_strName);

    if (iter == m_mapAnim.end())
    {
        return nullptr;
    }

    return iter->second;
}

void CAnimator::AddCustomAnimation(const wstring& _strName, CAnimation* _pAnim)
{
    // 이미 같은 이름의 애니메이션이 있다면 삭제
    auto iter = m_mapAnim.find(_strName);
    if (iter != m_mapAnim.end())
    {
        delete iter->second;
        m_mapAnim.erase(iter);
    }

    // 새 애니메이션 추가
    m_mapAnim.insert(make_pair(_strName, _pAnim));
}

void CAnimator::RenderScaled(HDC _dc, float _fScale)
{
    if (nullptr != m_pCurAnim && nullptr != m_pOwner)
    {
        Vec2 vPos = m_pOwner->GetPos();  // m_pOwner는 CAnimator의 멤버
        m_pCurAnim->RenderScaled(_dc, vPos, _fScale);
    }
}
