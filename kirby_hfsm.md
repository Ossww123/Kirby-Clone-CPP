```
Root
├─ Overlay(최우선, 제어권 잠금/우선순위)
│  ├─ None
│  ├─ Damaged
│  ├─ Dead
│  ├─ DoorEnter(문 입장 컷신)
│  ├─ Dance(보스 클리어)
│  └─ GameOver
├─ Movement(이동/물리)
│  ├─ Grounded
│  │   ├─ Idle
│  │   ├─ Walk
│  │   ├─ Run
│  │   ├─ Crouch(웅크리기)
│  │   └─ Slide(슬라이딩킥; 짧은 타이머/히트박스)
│  ├─ Airborne
│  │   ├─ Jump
│  │   ├─ Fall
│  │   └─ Inflated(공기 머금기 모드; 떠오르기/날개짓)
│  └─ Ladder(사다리)
└─ Action(전투/상호작용)
   ├─ Neutral(행동 없음)
   ├─ Inhale(빨아들이기; 흡입 박스 이벤트)
   ├─ MouthFull(머금은 상태; Spit/Swallow 대기)
   ├─ SpitObject(빨아들인 물체 뱉기)
   ├─ AirPuff(공기뱉기; Inflated에서만)
   └─ AbilityAtk(카피능력 공격: Fire/Spark/Beam…)
```