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