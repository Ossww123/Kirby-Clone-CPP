#pragma once

class CCollider;

union COLLIDER_ID
{
    struct
    {
        UINT iLeft_id;
        UINT iRight_id;
    };
    ULONGLONG ID;
};

class CCollisionMgr
{
    SINGLE(CCollisionMgr);

private:
    // 충돌 그룹 간의 충돌 체크 여부
    UINT m_arrCheck[(UINT)GROUP_TYPE::END];

    // 이전 프레임 충돌 정보
    map<ULONGLONG, bool> m_mapColInfo;

public:
    void init();
    void update();

    void CheckGroup(GROUP_TYPE _eLeft, GROUP_TYPE _eRight);
    void UnCheckGroup(GROUP_TYPE _eLeft, GROUP_TYPE _eRight);
    void Reset() { memset(m_arrCheck, 0, sizeof(UINT) * (UINT)GROUP_TYPE::END); }

private:
    void CollisionGroupUpdate(GROUP_TYPE _eLeft, GROUP_TYPE _eRight);
    bool IsCollision(CCollider* _pLeftCol, CCollider* _pRightCol);
};