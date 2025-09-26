#include "gamePCH.h"
#include "CProjectile.h"
#include "CTimeMgr.h"
#include "CCollider.h"
#include "CCore.h"
#include "CEventMgr.h"
#include "CAnimator.h"
#include "CAnimationDataMgr.h"
#include "CCamera.h"
#include "CSceneMgr.h"
#include "CScene.h"

CProjectile::CProjectile()
    : m_eProjectileType(PROJECTILE_TYPE::KIRBY_AIR_PUFF)
    , m_vDirection(Vec2(1.f, 0.f))
    , m_fSpeed(300.f)
    , m_fInitialSpeed(300.f)
    , m_fDeceleration(0.f)
    , m_fDamage(1.f)
    , m_fAccTime(0.f)
    , m_fMaxLifeTime(3.f)
    , m_eOwnerType(GROUP_TYPE::DEFAULT)
    , m_bIsRotatingBeam(false)
    , m_vRotationCenter(Vec2(0.f, 0.f))
    , m_fRotationRadius(0.f)
    , m_fStartAngle(0.f)
    , m_fEndAngle(0.f)
    , m_fRotationDuration(0.f)
    , m_fRotationTimer(0.f)
{
    SetType(OBJECT_TYPE::PLAYER);
    InitializeByType();
    CreateAnimation();
}

CProjectile::CProjectile(PROJECTILE_TYPE _eType)
    : m_eProjectileType(_eType)
    , m_vDirection(Vec2(1.f, 0.f))
    , m_fSpeed(300.f)
    , m_fInitialSpeed(300.f)
    , m_fDeceleration(0.f)
    , m_fDamage(1.f)
    , m_fAccTime(0.f)
    , m_fMaxLifeTime(3.f)
    , m_eOwnerType(GROUP_TYPE::DEFAULT)
    , m_bIsRotatingBeam(false)
    , m_vRotationCenter(Vec2(0.f, 0.f))
    , m_fRotationRadius(0.f)
    , m_fStartAngle(0.f)
    , m_fEndAngle(0.f)
    , m_fRotationDuration(0.f)
    , m_fRotationTimer(0.f)
{
    SetType(OBJECT_TYPE::PLAYER);
    InitializeByType();
    CreateAnimation();
}

CProjectile::~CProjectile()
{
}

void CProjectile::Update()
{
    
    // 애니메이터 업데이트 (애니메이션 프레임 진행)
    CAnimator* pAnimator = GetAnimator();
    if (pAnimator)
    {
        pAnimator->Update();
    }
    
    UpdateMovement();
    UpdateLifeTime();
    CheckBounds();
}

void CProjectile::Render(HDC _dc)
{
    // KIRBY_SLIDE_KICK, KIRBY_BEAM, KIRBY_ELECTRIC_FIELD, MONSTER_BEAM은 완전히 투명 (아무것도 렌더링하지 않음)
    if (m_eProjectileType == PROJECTILE_TYPE::KIRBY_SLIDE_KICK || 
        m_eProjectileType == PROJECTILE_TYPE::KIRBY_BEAM ||
        m_eProjectileType == PROJECTILE_TYPE::KIRBY_ELECTRIC_FIELD ||
        m_eProjectileType == PROJECTILE_TYPE::MONSTER_BEAM)
        return;
        
    CObject::Render(_dc);
        
    CAnimator* pAnimator = GetAnimator();
    if (!pAnimator || !pAnimator->GetCurAnim())
    {
        // 카메라 변환된 위치 계산
        Vec2 vRenderPos = CCamera::GetInst()->GetRenderPos(GetPos());
        Vec2 vScale = GetScale();
        
        // 투사체 타입별로 다른 색상으로 렌더링
        COLORREF color = RGB(255, 255, 255); // 기본 흰색
        
        switch (m_eProjectileType)
        {
        case PROJECTILE_TYPE::KIRBY_AIR_PUFF:
            color = RGB(200, 200, 255); // 연한 파랑
            break;
        case PROJECTILE_TYPE::KIRBY_STAR:
            color = RGB(255, 255, 0);   // 노랑
            break;
        case PROJECTILE_TYPE::KIRBY_FIRE:
            color = RGB(255, 100, 0);   // 주황
            break;
        case PROJECTILE_TYPE::BOSS_AIR_PUFF:
            color = RGB(100, 255, 100); // 연한 초록
            break;
        default:
            color = RGB(255, 255, 255); // 흰색
            break;
        }
        
        // 브러시 생성 및 사각형 그리기
        HBRUSH hBrush = CreateSolidBrush(color);
        HBRUSH hOldBrush = (HBRUSH)SelectObject(_dc, hBrush);
        
        Rectangle(_dc, 
            (int)(vRenderPos.x - vScale.x * 0.5f),
            (int)(vRenderPos.y - vScale.y * 0.5f),
            (int)(vRenderPos.x + vScale.x * 0.5f),
            (int)(vRenderPos.y + vScale.y * 0.5f));
        
        SelectObject(_dc, hOldBrush);
        DeleteObject(hBrush);
    }
}

void CProjectile::OnCollisionEnter(CCollider* _pOther)
{
    //CObject* pOtherObj = _pOther->GetOwner();
    //
    //// 다른 오브젝트의 그룹 타입은 씬 시스템에서 관리되므로
    //// 오브젝트 타입으로 판단
    //OBJECT_TYPE eOtherType = pOtherObj->GetType();
    //

    //// 같은 소유자와는 충돌하지 않음 (플레이어 투사체는 플레이어와 충돌 안함)
    //if ((m_eOwnerType == GROUP_TYPE::PLAYER && eOtherType == OBJECT_TYPE::PLAYER) ||
    //    (m_eOwnerType == GROUP_TYPE::MONSTER && 
    //     (eOtherType >= OBJECT_TYPE::MONSTER_WADDLE_DEE && eOtherType <= OBJECT_TYPE::MONSTER_WHISPY_WOODS)))
    //    return;

    //// 타일과 충돌 시 투사체 소멸
    //if (eOtherType >= OBJECT_TYPE::TILE_GROUND && eOtherType <= OBJECT_TYPE::TILE_INVISIBLE)
    //{
    //    // 투사체 히트 이벤트 발생
    //    tEvent event = {};
    //    event.eType = EVENT_TYPE::PROJECTILE_HIT;
    //    event.wParam = (DWORD_PTR)this;
    //    event.lParam = (DWORD_PTR)pOtherObj;
    //    CEventMgr::GetInst()->AddEvent(event);

    //    // 투사체 삭제
    //    SetDead();
    //    return;
    //}

    //// 몬스터와 충돌 시 (플레이어 투사체인 경우)
    //if (m_eOwnerType == GROUP_TYPE::PLAYER && 
    //    (eOtherType >= OBJECT_TYPE::MONSTER_WADDLE_DEE && eOtherType <= OBJECT_TYPE::MONSTER_WHISPY_WOODS))
    //{
    //    
    //    // 몬스터에게 데미지 이벤트 발생 (투사체 위치 정보도 함께 전달)
    //    tEvent event = {};
    //    event.eType = EVENT_TYPE::MONSTER_DAMAGE;
    //    event.wParam = (DWORD_PTR)pOtherObj;
    //    event.lParam = (DWORD_PTR)this;  // 투사체 객체 전달 (위치 정보 포함)
    //    CEventMgr::GetInst()->AddEvent(event);

    //    // 슬라이딩킥 투사체인 경우 플레이어 반동 이벤트 추가 생성
    //    if (m_eProjectileType == PROJECTILE_TYPE::KIRBY_SLIDE_KICK)
    //    {
    //        tEvent recoilEvent = {};
    //        recoilEvent.eType = EVENT_TYPE::PLAYER_SLIDE_KICK_RECOIL;
    //        recoilEvent.wParam = (DWORD_PTR)this;  // 투사체 정보 (방향 등)
    //        recoilEvent.lParam = (DWORD_PTR)pOtherObj;  // 충돌한 몬스터
    //        CEventMgr::GetInst()->AddEvent(recoilEvent);
    //    }

    //    // 투사체 히트 이벤트 발생
    //    tEvent hitEvent = {};
    //    hitEvent.eType = EVENT_TYPE::PROJECTILE_HIT;
    //    hitEvent.wParam = (DWORD_PTR)this;
    //    hitEvent.lParam = (DWORD_PTR)pOtherObj;
    //    CEventMgr::GetInst()->AddEvent(hitEvent);

    //    // 투사체 삭제
    //    SetDead();
    //    return;
    //}

    //// 플레이어와 충돌 시 (몬스터 투사체인 경우)
    //if (m_eOwnerType == GROUP_TYPE::MONSTER && eOtherType == OBJECT_TYPE::PLAYER)
    //{
    //    // 플레이어에게 데미지 이벤트 발생
    //    Vec2 knockbackDir = m_vDirection; // 투사체 방향으로 넉백
    //    tEvent event = {};
    //    event.eType = EVENT_TYPE::PLAYER_DAMAGE;
    //    event.wParam = (DWORD_PTR)pOtherObj;
    //    event.lParam = (DWORD_PTR)new Vec2(knockbackDir); // 동적 할당으로 변경
    //    CEventMgr::GetInst()->AddEvent(event);

    //    // 투사체 히트 이벤트 발생
    //    tEvent hitEvent = {};
    //    hitEvent.eType = EVENT_TYPE::PROJECTILE_HIT;
    //    hitEvent.wParam = (DWORD_PTR)this;
    //    hitEvent.lParam = (DWORD_PTR)pOtherObj;
    //    CEventMgr::GetInst()->AddEvent(hitEvent);

    //    // 투사체 삭제
    //    SetDead();
    //    return;
    //}
}

void CProjectile::UpdateMovement()
{
    // 회전 빔인 경우 별도 처리
    if (m_bIsRotatingBeam)
    {
        UpdateRotatingBeam();
        return;
    }
    
    // 기존 직선 이동 로직
    float fDT = CTimeMgr::GetInst()->GetfDT();
    
    // 감속 적용 (KIRBY_AIR_PUFF만 감속)
    if (m_eProjectileType == PROJECTILE_TYPE::KIRBY_AIR_PUFF && m_fDeceleration > 0.f)
    {
        // 현재 속도에서 감속도만큼 감소 (최소 0까지)
        m_fSpeed = max(0.f, m_fSpeed - m_fDeceleration * fDT);
    }
    
    // 현재 위치에 방향 * 속도 * 델타타임을 더해서 이동
    Vec2 vPos = GetPos();
    vPos += m_vDirection * m_fSpeed * fDT;
    SetPos(vPos);
}

void CProjectile::UpdateLifeTime()
{
    // 이미 삭제 예정이면 더 이상 처리하지 않음
    if (!IsAlive()) return;
    
    // 회전 빔인 경우 자체적으로 수명 관리하므로 건너뛰기
    if (m_bIsRotatingBeam) return;
    
    float fDT = CTimeMgr::GetInst()->GetfDT();
    m_fAccTime += fDT;

    // 생존 시간 초과 시 삭제 (한 번만 호출)
    if (m_fAccTime >= m_fMaxLifeTime)
    {
        SetDead();
    }
}

void CProjectile::CheckBounds()
{
    // 카메라 위치를 고려한 경계 체크
    Vec2 vResolution = CCore::GetInst()->GetResolution();
    Vec2 vPos = GetPos();
    Vec2 vScale = GetScale();
    Vec2 vCameraPos = CCamera::GetInst()->GetLookAt();

    // 카메라 중심을 기준으로 한 화면 경계 계산
    Vec2 vCameraLeftTop = vCameraPos - vResolution * 0.5f;
    Vec2 vCameraRightBottom = vCameraPos + vResolution * 0.5f;

    // 투사체가 카메라 뷰 영역을 벗어나면 삭제 (여유 공간 추가)
    float fMargin = 100.f; // 화면 밖 100픽셀까지 여유
    if (vPos.x + vScale.x * 0.5f < vCameraLeftTop.x - fMargin ||
        vPos.x - vScale.x * 0.5f > vCameraRightBottom.x + fMargin ||
        vPos.y + vScale.y * 0.5f < vCameraLeftTop.y - fMargin ||
        vPos.y - vScale.y * 0.5f > vCameraRightBottom.y + fMargin)
    {
        SetDead();
    }
}

// === 회전 빔 관련 메서드들 ===
void CProjectile::SetRotationData(Vec2 _vCenter, float _fRadius, float _fStartAngle, float _fEndAngle, float _fDuration)
{
    m_bIsRotatingBeam = true;
    m_vRotationCenter = _vCenter;
    m_fRotationRadius = _fRadius;
    m_fStartAngle = _fStartAngle;
    m_fEndAngle = _fEndAngle;
    m_fRotationDuration = _fDuration;
    m_fRotationTimer = 0.f;
    
    // 시작 위치로 설정
    Vec2 startPos = _vCenter + Vec2(
        cos(_fStartAngle) * _fRadius,
        sin(_fStartAngle) * _fRadius
    );
    SetPos(startPos);
    
}

void CProjectile::UpdateRotatingBeam()
{
    float fDT = CTimeMgr::GetInst()->GetfDT();
    m_fRotationTimer += fDT;
    
    // 회전 진행도 계산 (0.0 ~ 1.0)
    float rotationProgress = m_fRotationTimer / m_fRotationDuration;
    
    if (rotationProgress >= 1.0f)
    {
        // 회전 완료, 투사체 소멸
        SetDead();
        return;
    }
    
    // 현재 각도 계산 (선형 보간)
    float currentAngle = m_fStartAngle + (m_fEndAngle - m_fStartAngle) * rotationProgress;
    
    // 새 위치 계산
    Vec2 newPos = m_vRotationCenter + Vec2(
        cos(currentAngle) * m_fRotationRadius,
        sin(currentAngle) * m_fRotationRadius
    );
    
    SetPos(newPos);
    
}

void CProjectile::InitializeByType()
{
    switch (m_eProjectileType)
    {
    // === 커비 투사체들 ===
    case PROJECTILE_TYPE::KIRBY_AIR_PUFF:
        SetScale(Vec2(64.f, 32.f));  // JSON 파일의 spriteSize와 동일하게 수정
        m_fSpeed = 750.f;            // 초기 속도 (매우 빠르게 시작)
        m_fInitialSpeed = 750.f;     // 초기 속도 저장
        m_fDeceleration = 1400.f;      // 감속도 (초당 400픽셀/초씩 감소)
        m_fDamage = 1.f;
        m_fMaxLifeTime = 0.5f;        // 0.5초로 설정
        CreateCollider();
        GetCollider()->SetScale(Vec2(24.f, 24.f));
        CreateAnimator();             // 애니메이터 생성
        break;

    case PROJECTILE_TYPE::KIRBY_SLIDE_KICK:
        SetScale(Vec2(32.f, 32.f));  // 커비 발끝 크기
        m_fSpeed = 320.f;            // 슬라이드 속도와 동일
        m_fDamage = 1.f;             // 기본 데미지
        m_fMaxLifeTime = 0.8f;       // 슬라이드 지속시간과 동일
        CreateCollider();
        GetCollider()->SetScale(Vec2(30.f, 30.f));
        // 투명 투사체이므로 애니메이터는 생성하지 않음
        break;

    case PROJECTILE_TYPE::KIRBY_STAR:
        SetScale(Vec2(24.f, 24.f));
        m_fSpeed = 500.f;
        m_fDamage = 2.f;
        m_fMaxLifeTime = 3.f;
        CreateCollider();
        GetCollider()->SetScale(Vec2(20.f, 20.f));
        CreateAnimator();
        break;

    case PROJECTILE_TYPE::KIRBY_STAR_ENHANCED:
        SetScale(Vec2(32.f, 32.f));
        m_fSpeed = 550.f;
        m_fDamage = 4.f;  // 강화된 데미지
        m_fMaxLifeTime = 4.f;
        CreateCollider();
        GetCollider()->SetScale(Vec2(28.f, 28.f));
        CreateAnimator();
        break;

    case PROJECTILE_TYPE::KIRBY_FIRE:
        SetScale(Vec2(128.f, 128.f));  // 2x2 타일 크기
        m_fSpeed = 480.f;              // 1.5타일(96픽셀)을 0.2초에 이동
        m_fDamage = 3.f;
        m_fMaxLifeTime = 0.2f;         // 1.5타일 이동 후 삭제
        CreateCollider();
        GetCollider()->SetScale(Vec2(120.f, 120.f));  // 충돌체는 약간 작게
        CreateAnimator();
        break;

    case PROJECTILE_TYPE::KIRBY_BEAM:
        {
            SetScale(Vec2(20.f, 8.f));  // 빔 형태 (가로로 긴)
            m_fSpeed = 700.f;  // 빠른 속도
            m_fDamage = 2.f;
            m_fMaxLifeTime = 1.5f;
            CreateCollider();
            GetCollider()->SetScale(Vec2(18.f, 6.f));
            CreateAnimator();
            
            break;
        }

    case PROJECTILE_TYPE::KIRBY_ELECTRIC_FIELD:
        SetScale(Vec2(192.f, 192.f));  // 3x3 타일 크기 (64*3=192)
        m_fSpeed = 0.f;                // 이동하지 않음
        m_fDamage = 2.5f;
        m_fMaxLifeTime = 999.f;        // 매우 긴 수명 (수동으로 삭제)
        CreateCollider();
        GetCollider()->SetScale(Vec2(180.f, 180.f));  // 충돌체는 약간 작게
        CreateAnimator();
        break;

    // === 몬스터 투사체들 ===
    case PROJECTILE_TYPE::BOSS_AIR_PUFF:
        SetScale(Vec2(48.f, 48.f));  // 보스 투사체는 큼
        m_fSpeed = 300.f;
        m_fDamage = 1.f;
        m_fMaxLifeTime = 4.f;
        CreateCollider();
        GetCollider()->SetScale(Vec2(40.f, 40.f));
        CreateAnimator();
        break;

    case PROJECTILE_TYPE::MONSTER_FIREBALL:
        SetScale(Vec2(64.f, 64.f));  // 커비 파이어와 비슷한 크기
        m_fSpeed = 480.f;            // 커비 파이어와 동일한 속도
        m_fDamage = 1.f;
        m_fMaxLifeTime = 2.f;        // 2초 유지
        CreateCollider();
        GetCollider()->SetScale(Vec2(60.f, 60.f));
        CreateAnimator();
        break;

    case PROJECTILE_TYPE::MONSTER_ELECTRIC:
        SetScale(Vec2(96.f, 96.f));  // 커비 전기장과 비슷한 크기  
        m_fSpeed = 0.f;              // 커비 전기장처럼 이동하지 않음
        m_fDamage = 1.f;
        m_fMaxLifeTime = 2.f;        // 2초 유지
        CreateCollider();
        GetCollider()->SetScale(Vec2(90.f, 90.f));
        CreateAnimator();
        break;

    case PROJECTILE_TYPE::MONSTER_BEAM:
        SetScale(Vec2(16.f, 6.f));   // 웨이들두 빔 (가로로 긴)
        m_fSpeed = 600.f;
        m_fDamage = 1.f;
        m_fMaxLifeTime = 1.5f;
        CreateCollider();
        GetCollider()->SetScale(Vec2(14.f, 4.f));
        CreateAnimator();
        break;

    default:
        break;
    }
}

void CProjectile::CreateAnimation()
{
    // 애니메이터가 있는지 확인
    CAnimator* pAnimator = GetAnimator();
    if (!pAnimator)
        return;

    // 투사체 애니메이션 파일 로드 (플레이어와 동일한 방식)
    wstring animationFilePath = L"projectile_animations.json";
    CAnimationDataMgr::GetInst()->LoadAnimationsIntoAnimator(pAnimator, animationFilePath);
    
    // 투사체 타입에 맞는 애니메이션 설정
    SetAnimationByType();
}

void CProjectile::SetAnimationByType()
{
    CAnimator* pAnimator = GetAnimator();
    if (!pAnimator)
        return;

    // 투사체 타입별로 적절한 애니메이션 재생
    switch (m_eProjectileType)
    {
    case PROJECTILE_TYPE::KIRBY_AIR_PUFF:
        pAnimator->Play(L"KIRBY_AIR_PUFF", true);  // 반복 재생
        break;

    case PROJECTILE_TYPE::KIRBY_STAR:
        pAnimator->Play(L"KIRBY_STAR", true);
        break;

    case PROJECTILE_TYPE::KIRBY_STAR_ENHANCED:
        pAnimator->Play(L"KIRBY_STAR_ENHANCED", true);
        break;

    case PROJECTILE_TYPE::KIRBY_FIRE:
        pAnimator->Play(L"FIRE", true);
        // 왼쪽 방향이면 스프라이트 뒤집기
        if (m_vDirection.x < 0)
            pAnimator->SetFlipX(true);
        break;

    case PROJECTILE_TYPE::KIRBY_BEAM:
        pAnimator->Play(L"KIRBY_BEAM", true);
        break;

    case PROJECTILE_TYPE::KIRBY_ELECTRIC_FIELD:
        pAnimator->Play(L"KIRBY_ELECTRIC_FIELD", true);
        break;

    case PROJECTILE_TYPE::BOSS_AIR_PUFF:
        pAnimator->Play(L"BOSS_AIR_PUFF", true);
        break;

    case PROJECTILE_TYPE::MONSTER_FIREBALL:
        pAnimator->Play(L"FIRE", true);
        // 왼쪽 방향이면 스프라이트 뒤집기
        if (m_vDirection.x < 0)
            pAnimator->SetFlipX(true);
        break;

    case PROJECTILE_TYPE::MONSTER_ELECTRIC:
        pAnimator->Play(L"MONSTER_SPARK", true);
        break;

    case PROJECTILE_TYPE::MONSTER_BEAM:
        pAnimator->Play(L"MONSTER_BEAM", true);
        break;

    default:
        // 기본 애니메이션이 없으면 첫 번째 애니메이션 재생
        pAnimator->Play(L"KIRBY_AIR_PUFF", true);
        break;
    }
}