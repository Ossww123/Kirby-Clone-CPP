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

	// 단항 마이너스 연산자 (벡터 반대 방향)
	Vec2 operator - () const
	{
		return Vec2(-x, -y);
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

	// 내적(Dot Product) 계산
	float Dot(const Vec2& _other) const
	{
		return x * _other.x + y * _other.y;
	}

	// 외적(Cross Product) 계산 (2D에서는 스칼라 값 반환)
	float Cross(const Vec2& _other) const
	{
		return x * _other.y - y * _other.x;
	}

	// 벡터 사이의 각도 계산 (라디안)
	float Angle(const Vec2& _other) const
	{
		float dot = Dot(_other);
		float lenProduct = Length() * _other.Length();

		if (lenProduct == 0.f)
			return 0.f;

		float cosTheta = dot / lenProduct;
		// cos 값을 [-1, 1] 범위로 클램핑
		cosTheta = max(-1.f, min(1.f, cosTheta));

		return acos(cosTheta);
	}

	// 벡터의 제곱 길이 (성능상 이점 - sqrt 연산 생략)
	float LengthSq() const
	{
		return x * x + y * y;
	}

	// 거리 계산 (다른 점까지의 거리)
	float Distance(const Vec2& _other) const
	{
		return (*this - _other).Length();
	}

	// 제곱 거리 계산 (성능상 이점)
	float DistanceSq(const Vec2& _other) const
	{
		return (*this - _other).LengthSq();
	}

	// 정규화된 벡터 반환 (원본 수정 안함)
	Vec2 GetNormalized() const
	{
		Vec2 result = *this;
		result.Normalize();
		return result;
	}

	// 벡터가 영벡터인지 확인
	bool IsZero() const
	{
		return x == 0.f && y == 0.f;
	}

	// 선형 보간 (Linear Interpolation)
	static Vec2 Lerp(const Vec2& _from, const Vec2& _to, float _t)
	{
		return _from + (_to - _from) * _t;
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

// 이벤트 구조체
struct tEvent
{
	EVENT_TYPE  eType;      // 이벤트 타입
	DWORD_PTR   wParam;     // 첫 번째 매개변수
	DWORD_PTR   lParam;     // 두 번째 매개변수

	tEvent()
		: eType(EVENT_TYPE::END)
		, wParam(0)
		, lParam(0)
	{}

	tEvent(EVENT_TYPE _type, DWORD_PTR _wParam, DWORD_PTR _lParam)
		: eType(_type)
		, wParam(_wParam)
		, lParam(_lParam)
	{}
};

// 애니메이션 프레임 정보
struct tAnimFrame
{
	Vec2 vLT;           // 스프라이트 시트에서 좌상단 좌표
	Vec2 vSlice;        // 프레임 크기 (가로, 세로)
	float fDuration;    // 이 프레임의 지속 시간

	tAnimFrame()
		: vLT{}
		, vSlice{}
		, fDuration(0.1f)
	{}

	tAnimFrame(Vec2 _vLT, Vec2 _vSlice, float _fDuration)
		: vLT(_vLT)
		, vSlice(_vSlice)
		, fDuration(_fDuration)
	{}
};

// 레벨 오브젝트 데이터 (타일 시각 타입 정보 추가)
struct tLevelObjectData
{
	GROUP_TYPE  eGroupType;     // 오브젝트 그룹 타입
	Vec2        vPos;           // 위치
	Vec2        vScale;         // 크기
	int         iSubType;       // 서브 타입 (기존 몬스터 종류 구분용)
	int         iTileVisualType; // 타일 시각 타입 (새로 추가)

	tLevelObjectData()
		: eGroupType(GROUP_TYPE::DEFAULT)
		, vPos{}
		, vScale{}
		, iSubType(0)
		, iTileVisualType(0)  // 새로 추가
	{}

	tLevelObjectData(GROUP_TYPE _eType, Vec2 _vPos, Vec2 _vScale, int _iSubType = 0, int _iTileVisualType = 0)
		: eGroupType(_eType)
		, vPos(_vPos)
		, vScale(_vScale)
		, iSubType(_iSubType)
		, iTileVisualType(_iTileVisualType)  // 새로 추가
	{}
};

// 레벨 전체 데이터 (배경 정보 + 경계 정보 추가)
struct tLevelData
{
	wstring                     strLevelName;       // 레벨 이름
	Vec2                        vPlayerSpawn;       // 플레이어 스폰 위치
	vector<tLevelObjectData>    vecObjects;         // 배치된 오브젝트들
	int                         iVersion;           // 파일 버전
	int                         iBackgroundType;    // 배경 타입

	// === 새로 추가: 레벨 경계 정보 ===
	Vec2                        vLevelBoundsMin;    // 레벨 최소 경계 (좌상단)
	Vec2                        vLevelBoundsMax;    // 레벨 최대 경계 (우하단)
	float                       fGameOverY;         // 낙사 지점 Y좌표

	tLevelData()
		: strLevelName{}
		, vPlayerSpawn{}
		, vecObjects{}
		, iVersion(3)  // 버전을 3으로 업데이트 (경계 정보 추가로 인함)
		, iBackgroundType(0)
		, vLevelBoundsMin(Vec2(0.f, -1000.f))        // 기본값: x=0 이상, y=-1000 이상
		, vLevelBoundsMax(Vec2(4000.f, 1280.f))      // 기본값: x=4000 이하, y=1280 이하
		, fGameOverY(1280.f)                         // 기본 낙사 지점
	{}
};

// 타일 정보 구조체
struct tTileInfo
{
	TILE_VISUAL_TYPE eType;
	Vec2 vDefaultSize;      // 기본 크기
	bool bKeepAspectRatio;  // 비율 유지 여부
	bool bDecorative;       // 장식용 여부 (충돌 없음)
	bool bHarmful;          // 데미지 여부
	wstring strTexturePath; // 텍스처 경로

	tTileInfo()
		: eType(TILE_VISUAL_TYPE::GRASS_PLATFORM)
		, vDefaultSize(Vec2(64.f, 64.f))
		, bKeepAspectRatio(true)
		, bDecorative(false)
		, bHarmful(false)
		, strTexturePath(L"")
	{}

	tTileInfo(TILE_VISUAL_TYPE _eType, Vec2 _vSize, bool _bDeco = false, bool _bHarm = false, const wstring& _strPath = L"")
		: eType(_eType)
		, vDefaultSize(_vSize)
		, bKeepAspectRatio(true)
		, bDecorative(_bDeco)
		, bHarmful(_bHarm)
		, strTexturePath(_strPath)
	{}
};