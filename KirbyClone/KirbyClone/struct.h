#pragma once

struct Vec2
{
	float x;
	float y;

public:
	Vec2()
		: x(0.f)
		, y(0.f)
	{}

	Vec2(float _x, float _y)
		: x(_x)
		, y(_y)
	{}

	Vec2(int _x, int _y)
		: x((float)_x)
		, y((float)_y)
	{}
};


struct tKeyInfo
{
	KEY_STATE   eState;     // 키의 상태
	bool        bPrevPush;  // 이전 프레임에 눌렸는지 여부
};