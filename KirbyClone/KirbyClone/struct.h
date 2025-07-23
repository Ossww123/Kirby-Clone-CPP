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

	// 복사 생성자
	Vec2(const Vec2& _other)
		: x(_other.x)
		, y(_other.y)
	{}

	// 연산자 오버로딩
	Vec2 operator + (const Vec2& _other) const
	{
		return Vec2(x + _other.x, y + _other.y);
	}

	Vec2 operator - (const Vec2& _other) const
	{
		return Vec2(x - _other.x, y - _other.y);
	}

	Vec2 operator * (float _f) const
	{
		return Vec2(x * _f, y * _f);
	}

	Vec2 operator / (float _f) const
	{
		return Vec2(x / _f, y / _f);
	}

	Vec2& operator += (const Vec2& _other)
	{
		x += _other.x;
		y += _other.y;
		return *this;
	}

	Vec2& operator -= (const Vec2& _other)
	{
		x -= _other.x;
		y -= _other.y;
		return *this;
	}

	Vec2& operator *= (float _f)
	{
		x *= _f;
		y *= _f;
		return *this;
	}

	Vec2& operator /= (float _f)
	{
		x /= _f;
		y /= _f;
		return *this;
	}

	// 대입 연산자
	Vec2& operator = (const Vec2& _other)
	{
		x = _other.x;
		y = _other.y;
		return *this;
	}

	// 비교 연산자
	bool operator == (const Vec2& _other) const
	{
		return x == _other.x && y == _other.y;
	}

	bool operator != (const Vec2& _other) const
	{
		return !(*this == _other);
	}

	// 벡터의 길이
	float Length() const
	{
		return sqrt(x * x + y * y);
	}

	// 정규화 (단위 벡터로 만들기)
	Vec2& Normalize()
	{
		float len = Length();
		if (len != 0.f)
		{
			x /= len;
			y /= len;
		}
		return *this;
	}
};


struct tKeyInfo
{
	KEY_STATE   eState;     // 키의 상태
	bool        bPrevPush;  // 이전 프레임에 눌렸는지 여부
};