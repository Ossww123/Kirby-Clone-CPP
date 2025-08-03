#pragma once
#include "CObject.h"

class CDoor : public CObject
{
private:
    SCENE_TYPE  m_eTargetScene;         // 이동할 씬
    Vec2        m_vTargetPosition;      // 이동할 위치
    wstring     m_strTargetDoorID;      // 연결된 문의 ID
    wstring     m_strDoorID;            // 이 문의 고유 ID

    bool        m_bPlayerNearby;        // 플레이어가 근처에 있는지
    bool        m_bIsLocked;            // 문이 잠겨있는지
    float       m_fInteractionRange;   // 상호작용 범위

    // 시각적 효과
    float       m_fAnimTimer;           // 애니메이션 타이머
    bool        m_bShowPrompt;          // "↑ 키를 눌러 입장" 프롬프트 표시

public:
    virtual void Update() override;
    virtual void Render(HDC _dc) override;
    virtual void OnCollisionEnter(CCollider* _pOther) override;
    virtual void OnCollision(CCollider* _pOther) override;
    virtual void OnCollisionExit(CCollider* _pOther) override;

private:
    void CheckPlayerInteraction();      // 플레이어 상호작용 체크
    void ProcessDoorEnter();           // 문 입장 처리
    void RenderPrompt(HDC _dc);        // 프롬프트 렌더링
    void RenderDoorEffect(HDC _dc);    // 문 시각 효과 렌더링

public:
    // Getter/Setter
    void SetTargetScene(SCENE_TYPE _eScene) { m_eTargetScene = _eScene; }
    void SetTargetPosition(Vec2 _vPos) { m_vTargetPosition = _vPos; }
    void SetTargetDoorID(const wstring& _strID) { m_strTargetDoorID = _strID; }
    void SetDoorID(const wstring& _strID) { m_strDoorID = _strID; }
    void SetLocked(bool _bLocked) { m_bIsLocked = _bLocked; }

    SCENE_TYPE GetTargetScene() const { return m_eTargetScene; }
    Vec2 GetTargetPosition() const { return m_vTargetPosition; }
    const wstring& GetTargetDoorID() const { return m_strTargetDoorID; }
    const wstring& GetDoorID() const { return m_strDoorID; }
    bool IsLocked() const { return m_bIsLocked; }

    // 레벨 에디터용 함수
    void SetEditorData(SCENE_TYPE _eScene, Vec2 _vPos, const wstring& _strTargetID);

public:
    CDoor();
    virtual ~CDoor();
};