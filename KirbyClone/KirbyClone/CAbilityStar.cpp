#include "pch.h"
#include "CAbilityStar.h"
#include "CTimeMgr.h"
#include "CCollider.h"
#include "CEventMgr.h"
#include "CCore.h"
#include "CAnimator.h"
#include "CAnimationDataMgr.h"
#include "CTile.h"
#include "CCamera.h"

CAbilityStar::CAbilityStar()
    : CObject(OBJECT_TYPE::ITEM_ABILITY_STAR)
    , m_eAbility(COPY_ABILITY::NONE)
    , m_fLifeTime(0.f)
    , m_fMaxLifeTime(10.f)          // 10초 후 자동 소멸
    , m_vVelocity(Vec2(0.f, 0.f))
    , m_vInitialVelocity(Vec2(320.f, -800.f))  // X: 10타일/2초, Y: 4타일 최고점
    , m_fGravity(800.f)             // 중력 가속도 (Y 최고점 공식에 맞춤)
    , m_fBounceFactorX(0.7f)        // X축 반발 계수
    , m_fBounceFactorY(0.5f)        // Y축 반발 계수
    , m_fFriction(0.95f)            // 마찰 계수
    , m_bOnGround(false)
    , m_fBlinkTimer(0.f)
    , m_bVisible(true)
{
    // 기본 크기 설정 (능력별 크기)
    SetScale(Vec2(32.f, 32.f));
    
    // 충돌체 생성
    CreateCollider();
    GetCollider()->SetScale(Vec2(28.f, 28.f));
    
    // 애니메이터 생성 및 애니메이션 로드
    CreateAnimator();
    CAnimationDataMgr::GetInst()->LoadAnimationsIntoAnimator(GetAnimator(), L"projectile_animations.json");
    GetAnimator()->Play(L"KIRBY_STAR", true);
}

CAbilityStar::CAbilityStar(COPY_ABILITY _eAbility)
    : CObject(OBJECT_TYPE::ITEM_ABILITY_STAR)
    , m_eAbility(_eAbility)
    , m_fLifeTime(0.f)
    , m_fMaxLifeTime(10.f)
    , m_vVelocity(Vec2(0.f, 0.f))
    , m_vInitialVelocity(Vec2(320.f, -800.f))  // X: 10타일/2초, Y: 4타일 최고점
    , m_fGravity(800.f)
    , m_fBounceFactorX(0.7f)
    , m_fBounceFactorY(0.5f)
    , m_fFriction(0.95f)
    , m_bOnGround(false)
    , m_fBlinkTimer(0.f)
    , m_bVisible(true)
{
    // 기본 크기 설정
    SetScale(Vec2(32.f, 32.f));
    
    // 충돌체 생성
    CreateCollider();
    GetCollider()->SetScale(Vec2(28.f, 28.f));
    
    // 애니메이터 생성 및 애니메이션 로드
    CreateAnimator();
    CAnimationDataMgr::GetInst()->LoadAnimationsIntoAnimator(GetAnimator(), L"projectile_animations.json");
    GetAnimator()->Play(L"KIRBY_STAR", true);
}

CAbilityStar::~CAbilityStar()
{
}

void CAbilityStar::Update()
{
    float fDT = CTimeMgr::GetInst()->GetfDT();
    
    // 생명주기 관리
    m_fLifeTime += fDT;
    
    // 사라지기 2초 전부터 깜빡임
    if (m_fLifeTime >= m_fMaxLifeTime - 2.f)
    {
        m_fBlinkTimer += fDT;
        if (m_fBlinkTimer >= 0.2f)  // 0.2초마다 깜빡임
        {
            m_bVisible = !m_bVisible;
            m_fBlinkTimer = 0.f;
        }
    }
    
    // 수명 다하면 삭제
    if (m_fLifeTime >= m_fMaxLifeTime)
    {
        SetDead();
        return;
    }
    
    // 물리 시뮬레이션
    UpdatePhysics();
    
    // === 디버깅: 위치와 속도 출력 ===
    static float debugTimer = 0.f;
    debugTimer += fDT;
    if (debugTimer >= 0.5f)  // 0.5초마다 출력
    {
        Vec2 vPos = GetPos();
        wchar_t debugText[256];
        swprintf_s(debugText, L"AbilityStar - Pos: (%.1f, %.1f), Vel: (%.1f, %.1f), OnGround: %s\n", 
                   vPos.x, vPos.y, m_vVelocity.x, m_vVelocity.y, m_bOnGround ? L"true" : L"false");
        OutputDebugString(debugText);
        debugTimer = 0.f;
    }
    
    // 땅 충돌 체크
    CheckGroundCollision();
    
    // 애니메이터 업데이트
    if (GetAnimator())
    {
        GetAnimator()->Update();
    }
}

void CAbilityStar::Render(HDC _dc)
{
    // 깜빡임 상태가 아니거나 보이는 상태일 때만 렌더링
    if (m_bVisible)
    {
        // 애니메이터가 있으면 애니메이터로 렌더링, 없으면 기본 렌더링
        if (GetAnimator())
        {
            GetAnimator()->Render(_dc);
        }
        else
        {
            CObject::Render(_dc);
        }
    }
}

void CAbilityStar::OnCollisionEnter(CCollider* _pOther)
{
    // 플레이어와 충돌 시 빨아들이기 시스템에서 처리
    // 여기서는 별도 처리하지 않음 (빨아들이기 시스템에서 감지)
}

void CAbilityStar::OnCollision(CCollider* _pOther)
{
    CObject* pOtherObj = _pOther->GetOwner();
    if (!pOtherObj)
        return;

    // 타일과의 충돌 처리
    if (pOtherObj->GetType() == OBJECT_TYPE::TILE_GROUND )
    {
        HandleTileCollision(pOtherObj);
    }
}

void CAbilityStar::UpdatePhysics()
{
    float fDT = CTimeMgr::GetInst()->GetfDT();
    
    // 중력 적용 (Y축 속도 증가)
    if (!m_bOnGround)
    {
        m_vVelocity.y += m_fGravity * fDT;
    }
    
    // X축 속력은 일정하게 유지 (마찰 없음)
    
    // 위치 업데이트
    Vec2 vPos = GetPos();
    vPos += m_vVelocity * fDT;
    SetPos(vPos);
}

void CAbilityStar::CheckGroundCollision()
{
    // 범위 제한 없음 - 수명이 다하거나 플레이어가 먹을 때까지 유지
}

void CAbilityStar::ApplyBounce()
{
    // Y축 바운스 (위쪽으로 튕김)
    m_vVelocity.y = -m_vVelocity.y * m_fBounceFactorY;
    
    // 바운스가 너무 작으면 멈춤
    if (abs(m_vVelocity.y) < 50.f)
    {
        m_vVelocity.y = 0.f;
        m_bOnGround = true;
    }
}

void CAbilityStar::HandleTileCollision(CObject* _pTile)
{
    if (!_pTile || !GetCollider() || !_pTile->GetCollider())
        return;

    CTile* pTile = dynamic_cast<CTile*>(_pTile);
    if (!pTile || !pTile->IsSolid())
        return;

    Vec2 vMyPos = GetPos();
    Vec2 vTilePos = _pTile->GetPos();
    Vec2 vMyScale = GetCollider()->GetScale();
    Vec2 vTileScale = _pTile->GetCollider()->GetScale();

    // 충돌 깊이 계산
    float fOverlapX = (vMyScale.x + vTileScale.x) / 2.f - abs(vMyPos.x - vTilePos.x);
    float fOverlapY = (vMyScale.y + vTileScale.y) / 2.f - abs(vMyPos.y - vTilePos.y);

    if (fOverlapX > 0.f && fOverlapY > 0.f)
    {
        // 더 작은 겹침을 우선으로 분리
        if (fOverlapX < fOverlapY)
        {
            // X축 분리 (좌우 벽 충돌)
            if (vMyPos.x < vTilePos.x)
            {
                // 왼쪽에서 충돌 - 오른쪽으로 밀어냄
                vMyPos.x = vTilePos.x - (vMyScale.x + vTileScale.x) / 2.f;
            }
            else
            {
                // 오른쪽에서 충돌 - 왼쪽으로 밀어냄
                vMyPos.x = vTilePos.x + (vMyScale.x + vTileScale.x) / 2.f;
            }
            SetPos(vMyPos);
            
            // X축 속도 반전 (벽 바운스)
            m_vVelocity.x = -m_vVelocity.x * m_fBounceFactorX;
        }
        else
        {
            // Y축 분리 (바닥/천장 충돌)
            if (vMyPos.y < vTilePos.y)
            {
                // 위에서 충돌 - 아래로 밀어냄
                vMyPos.y = vTilePos.y - (vMyScale.y + vTileScale.y) / 2.f;
            }
            else
            {
                // 아래에서 충돌 - 위로 밀어냄 (바닥 충돌)
                vMyPos.y = vTilePos.y + (vMyScale.y + vTileScale.y) / 2.f;
                m_bOnGround = true;
            }
            SetPos(vMyPos);
            
            // Y축 속도 초기화 (사인파 궤도 유지)
            if (m_vVelocity.y > 0) // 아래로 떨어지고 있었다면 (바닥 충돌)
            {
                // 초기 Y속도로 재설정하여 일정한 포물선 유지
                m_vVelocity.y = m_vInitialVelocity.y;
                m_bOnGround = false; // 다시 점프 시작
            }
            else // 위로 올라가고 있었다면 (천장 충돌)
            {
                m_vVelocity.y = -m_vVelocity.y * m_fBounceFactorY;
            }
        }
    }
}