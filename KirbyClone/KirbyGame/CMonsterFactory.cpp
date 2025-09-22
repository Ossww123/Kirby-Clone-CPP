#include "gamePCH.h"
#include "CMonsterFactory.h"

// 몬스터 클래스들 include
#include "CWaddleDee.h"
#include "CWaddleDoo.h"
#include "CBrontoBurt.h"
#include "CGordo.h"
#include "CHotHead.h"
#include "CSparky.h"
#include "CWhispyWoods.h"
#include "CApple.h"

CObject* CMonsterFactory::CreateMonster(OBJECT_TYPE _eMonsterType, Vec2 _vPos)
{
    CObject* pMonster = nullptr;

    // 각 몬스터별 세부 생성 함수 호출
    switch (_eMonsterType)
    {
    case OBJECT_TYPE::MONSTER_WADDLE_DEE:
        pMonster = CreateWaddleDee(_vPos);
        break;
    case OBJECT_TYPE::MONSTER_WADDLE_DOO:
        pMonster = CreateWaddleDoo(_vPos);
        break;
    case OBJECT_TYPE::MONSTER_BRONTO_BURT:
        pMonster = CreateBrontoBurt(_vPos);
        break;
    case OBJECT_TYPE::MONSTER_GORDOS:
        pMonster = CreateGordo(_vPos);
        break;
    case OBJECT_TYPE::MONSTER_HOT_HEAD:
        pMonster = CreateHotHead(_vPos);
        break;
    case OBJECT_TYPE::MONSTER_SPARKY:
        pMonster = CreateSparky(_vPos);
        break;
    case OBJECT_TYPE::MONSTER_WHISPY_WOODS:
        pMonster = CreateWhispyWoods(_vPos);
        break;
    default:
        return nullptr;
    }

    return pMonster;
}

CWaddleDee* CMonsterFactory::CreateWaddleDee(Vec2 _vPos)
{
    CWaddleDee* pWaddleDee = new CWaddleDee;
    pWaddleDee->SetPos(_vPos);
    pWaddleDee->SetScale(Vec2(64.f, 64.f));

    // 웨이들 디 전용 설정
    // (생성자에서 대부분 처리되므로 추가 설정은 최소화)

    return pWaddleDee;
}

CWaddleDoo* CMonsterFactory::CreateWaddleDoo(Vec2 _vPos)
{
    CWaddleDoo* pWaddleDoo = new CWaddleDoo;
    pWaddleDoo->SetPos(_vPos);
    pWaddleDoo->SetScale(Vec2(64.f, 64.f));

    // 웨이들 두 전용 설정
    // pWaddleDoo->SetAttackRange(150.f);  // 향후 공격 범위 설정 추가

    return pWaddleDoo;
}

CBrontoBurt* CMonsterFactory::CreateBrontoBurt(Vec2 _vPos)
{
    CBrontoBurt* pBrontoBurt = new CBrontoBurt;
    pBrontoBurt->SetPos(_vPos);
    pBrontoBurt->SetScale(Vec2(72.f, 64.f));  // 조금 더 큰 크기

    // 브론토 버트 전용 설정
    // pBrontoBurt->SetFlightHeight(_vPos.y);  // 향후 비행 높이 설정 추가

    return pBrontoBurt;
}

CGordo* CMonsterFactory::CreateGordo(Vec2 _vPos)
{
    CGordo* pGordo = new CGordo;
    pGordo->SetPos(_vPos);
    pGordo->SetScale(Vec2(80.f, 80.f));  // 더 큰 크기

    // 고르도 전용 설정
    // pGordo->SetMoveDirection(GORDO_MOVE_TYPE::HORIZONTAL);  // 이동 방향 설정 추가

    return pGordo;
}

CHotHead* CMonsterFactory::CreateHotHead(Vec2 _vPos)
{
    CHotHead* pHotHead = new CHotHead;
    pHotHead->SetPos(_vPos);
    pHotHead->SetScale(Vec2(64.f, 64.f));

    // 핫 헤드 전용 설정
    // pHotHead->SetFireRange(120.f);  // 화염 공격 범위 설정 추가

    return pHotHead;
}

CSparky* CMonsterFactory::CreateSparky(Vec2 _vPos)
{
    CSparky* pSparky = new CSparky;
    pSparky->SetPos(_vPos);
    pSparky->SetScale(Vec2(64.f, 64.f));

    // 스파키 전용 설정
    // pSparky->SetElectricRange(100.f);  // 전기 공격 범위 설정 추가

    return pSparky;
}

CWhispyWoods* CMonsterFactory::CreateWhispyWoods(Vec2 _vPos)
{
    CWhispyWoods* pWhispyWoods = new CWhispyWoods;
    pWhispyWoods->SetPos(_vPos);
    pWhispyWoods->SetScale(Vec2(128.f, 160.f));  // 큰 보스 크기

    // 위스피 우즈 전용 설정
    // pWhispyWoods->SetBossHP(1000);      // 보스 체력 설정 추가
    // pWhispyWoods->SetBossPhase(BOSS_PHASE::INTRO);  // 초기 페이즈 설정

    return pWhispyWoods;
}

CApple* CMonsterFactory::CreateApple(Vec2 _vPos)
{
    CApple* pApple = new CApple;
    pApple->SetPos(_vPos);
    pApple->SetScale(Vec2(32.f, 32.f));  // 작은 사과 크기

    // 사과 전용 설정
    pApple->SetGravity(true);        // 중력 적용
    pApple->SetLifetime(10.f);       // 10초 생존시간

    return pApple;
}

const wchar_t* CMonsterFactory::GetMonsterTypeName(OBJECT_TYPE _eType)
{
    switch (_eType)
    {
    case OBJECT_TYPE::MONSTER_WADDLE_DEE: return L"WaddleDee";
    case OBJECT_TYPE::MONSTER_WADDLE_DOO: return L"WaddleDoo";
    case OBJECT_TYPE::MONSTER_BRONTO_BURT: return L"BrontoBurt";
    case OBJECT_TYPE::MONSTER_GORDOS: return L"Gordo";
    case OBJECT_TYPE::MONSTER_HOT_HEAD: return L"HotHead";
    case OBJECT_TYPE::MONSTER_SPARKY: return L"Sparky";
    case OBJECT_TYPE::MONSTER_WHISPY_WOODS: return L"WhispyWoods";
    default: return L"Unknown Monster";
    }
}

GROUP_TYPE CMonsterFactory::GetMonsterGroup(OBJECT_TYPE _eType)
{
    return GROUP_TYPE::MONSTER;
}

Vec2 CMonsterFactory::GetMonsterDefaultScale(OBJECT_TYPE _eType)
{
    switch (_eType)
    {
    case OBJECT_TYPE::MONSTER_WADDLE_DEE: return Vec2(64.f, 64.f);
    case OBJECT_TYPE::MONSTER_WADDLE_DOO: return Vec2(64.f, 64.f);
    case OBJECT_TYPE::MONSTER_BRONTO_BURT: return Vec2(72.f, 64.f);
    case OBJECT_TYPE::MONSTER_GORDOS: return Vec2(80.f, 80.f);
    case OBJECT_TYPE::MONSTER_HOT_HEAD: return Vec2(64.f, 64.f);
    case OBJECT_TYPE::MONSTER_SPARKY: return Vec2(64.f, 64.f);
    case OBJECT_TYPE::MONSTER_WHISPY_WOODS: return Vec2(128.f, 160.f);
    default: return Vec2(64.f, 64.f);
    }
}

bool CMonsterFactory::IsMonsterType(OBJECT_TYPE _eType)
{
    return _eType >= OBJECT_TYPE::MONSTER_WADDLE_DEE && _eType <= OBJECT_TYPE::MONSTER_WHISPY_WOODS;
}

void CMonsterFactory::SetupMonsterAI(CObject* _pMonster, OBJECT_TYPE _eType)
{
    // 몬스터별 AI 설정 (향후 확장)
    // 현재는 생성자에서 처리되므로 추가 설정 없음
}
