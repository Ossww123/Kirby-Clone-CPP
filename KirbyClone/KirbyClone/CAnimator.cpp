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

void CAnimator::Update()
{
    if (nullptr == m_pCurAnim)
        return;

    // 현재 애니메이션 업데이트
    m_pCurAnim->Update();

    // 애니메이션 완료 시 처리
    if (m_pCurAnim->IsFinish() && !m_bRepeat)
    {
        m_pCurAnim = nullptr;
    }
}

void CAnimator::Render(HDC _dc)
{
    if (nullptr == m_pCurAnim || nullptr == m_pOwner)
        return;

    // 오브젝트 위치 가져오기
    Vec2 vPos = m_pOwner->GetPos();

    // 스케일 팩터 적용
    float fScale = CCore::GetPixelScale();

    // 스케일된 렌더링 수행
    m_pCurAnim->RenderScaled(_dc, vPos, fScale);
}

void CAnimator::CreateAnimation(const wstring& _strName, CTexture* _pTex,
    Vec2 _vLT, Vec2 _vSliceSize, Vec2 _vStep,
    float _fDuration, int _iFrameCount, bool _bLoop)
{
    // 유효성 검사
    if (_strName.empty() || nullptr == _pTex)
        return;

    // 중복 애니메이션 체크
    CAnimation* pExistingAnim = FindAnimation(_strName);
    if (nullptr != pExistingAnim)
        return;

    // 새 애니메이션 생성
    CAnimation* pAnim = new CAnimation;
    pAnim->SetName(_strName);
    pAnim->SetTexture(_pTex);
    pAnim->Create(_pTex, _vLT, _vSliceSize, _vStep, _fDuration, _iFrameCount, _bLoop);

    // 맵에 추가
    m_mapAnim.insert(make_pair(_strName, pAnim));
}

void CAnimator::AddCustomAnimation(const wstring& _strName, CAnimation* _pAnim)
{
    // 유효성 검사
    if (_strName.empty() || nullptr == _pAnim)
        return;

    // 기존 애니메이션이 있다면 교체
    auto iter = m_mapAnim.find(_strName);
    if (iter != m_mapAnim.end())
    {
        delete iter->second;
        iter->second = _pAnim;
    }
    else
    {
        // 새 애니메이션 추가
        m_mapAnim.insert(make_pair(_strName, _pAnim));
    }
}

void CAnimator::LoadAnimation(const wstring& _strRelativePath)
{
    // TODO: 파일에서 애니메이션 정보 로드
    // 현재 미구현 상태
}

void CAnimator::SaveAnimation(const wstring& _strRelativePath)
{
    // TODO: 애니메이션 정보를 파일로 저장  
    // 현재 미구현 상태
}

void CAnimator::Play(const wstring& _strName, bool _bRepeat)
{
    // 애니메이션 찾기
    CAnimation* pTargetAnim = FindAnimation(_strName);
    if (nullptr == pTargetAnim)
        return;

    // 애니메이션 재생 시작
    m_pCurAnim = pTargetAnim;
    m_bRepeat = _bRepeat;
    m_pCurAnim->Reset();
}

void CAnimator::RenderScaled(HDC _dc, float _fScale)
{
    if (nullptr == m_pCurAnim || nullptr == m_pOwner)
        return;

    // 오브젝트 위치 가져오기
    Vec2 vPos = m_pOwner->GetPos();

    // 스케일된 렌더링 수행
    m_pCurAnim->RenderScaled(_dc, vPos, _fScale);
}

CAnimation* CAnimator::FindAnimation(const wstring& _strName)
{
    auto iter = m_mapAnim.find(_strName);

    if (iter == m_mapAnim.end())
        return nullptr;

    return iter->second;
}