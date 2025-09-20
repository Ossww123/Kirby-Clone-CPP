#include "gamePCH.h"
#include "CSpecialObject.h"
#include "CTimeMgr.h"
#include "CCamera.h"

CSpecialObject::CSpecialObject()
    : m_eSpecialType(OBJECT_TYPE::OBJECT_DOOR)
    , m_bIsActive(true)
    , m_bIsInteractable(true)
    , m_fInteractionRange(80.f)
{
    // 기본 오브젝트 타입 설정
    SetType(OBJECT_TYPE::OBJECT_DOOR);
}

CSpecialObject::~CSpecialObject()
{
}

void CSpecialObject::Update()
{
}

void CSpecialObject::Render(HDC _dc)
{
}
