#pragma once

#define SINGLE(type) public:\
						 static type* GetInst()\
						 {\
							 static type mgr;\
							 return &mgr;\
						 }\
					 private:\
						 type();\
						 ~type();

/* ------------- */
/* 키 매니저 관련 */
/* ------------- */
#define KEY_TAP(key)    CKeyMgr::GetInst()->IsKeyTap(key)
#define KEY_HOLD(key)   CKeyMgr::GetInst()->IsKeyHold(key)
#define KEY_AWAY(key)   CKeyMgr::GetInst()->IsKeyAway(key)

/* --------------- */
/* 이벤트 관련 매크로 */
/* --------------- */
#define CREATE_OBJECT(obj, group) \
    do { \
        tEvent event(EVENT_TYPE::CREATE_OBJECT, (DWORD_PTR)group, (DWORD_PTR)obj); \
        CEventMgr::GetInst()->AddEvent(event); \
    } while(0)

#define DELETE_OBJECT(obj) \
    do { \
        tEvent event(EVENT_TYPE::DELETE_OBJECT, 0, (DWORD_PTR)obj); \
        CEventMgr::GetInst()->AddEvent(event); \
    } while(0)

#define CHANGE_SCENE(scene) \
    do { \
        tEvent event(EVENT_TYPE::SCENE_CHANGE, 0, (DWORD_PTR)scene); \
        CEventMgr::GetInst()->AddEvent(event); \
    } while(0)