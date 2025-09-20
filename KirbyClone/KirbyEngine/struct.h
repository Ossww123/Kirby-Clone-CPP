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


// =============================================================================
// === 입력 시스템 ===
// =============================================================================

// 키 입력 상태 정보
struct tKeyInfo
{
	KEY_STATE   eState;     // 키의 상태
	bool        bPrevPush;  // 이전 프레임에 눌렸는지 여부
};

// =============================================================================
// === 이벤트 시스템 ===
// =============================================================================

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

// =============================================================================
// === 애니메이션 시스템 ===
// =============================================================================

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

// 애니메이션 데이터 (파일 저장용)
struct tAnimationData
{
	wstring strName;                        // 애니메이션 이름
	vector<tAnimFrame> vecFrames;           // 프레임 목록
	bool bLoop;                             // 반복 재생 여부

	tAnimationData()
		: bLoop(true)
	{}

	tAnimationData(const wstring& _strName, bool _bLoop = true)
		: strName(_strName)
		, bLoop(_bLoop)
	{}
};

// 파일 전체 데이터 (한 파일에 여러 애니메이션)
struct tAnimationFileData
{
	wstring strTexturePath;                     // 텍스처 파일 경로
	Vec2 vSpriteSize;                          // 기본 스프라이트 크기
	int iBorder;                               // 테두리 크기
	map<wstring, tAnimationData> mapAnimations; // 애니메이션 맵

	tAnimationFileData()
		: vSpriteSize(32.f, 32.f)
		, iBorder(1)
	{}
};

// =============================================================================
// === 충돌 시스템 ===
// =============================================================================

// 충돌체 속성을 나타내는 구조체
struct tCollisionInfo
{
	COLLISION_TYPE eType;        // 충돌체 타입
	COLORREF displayColor;       // 에디터에서 표시할 색깔
	bool bIsSolid;              // 단단한 충돌 (통과 불가)
	bool bIsHarmful;            // 데미지를 주는가
	bool bIsOneWay;             // 일방통행인가 (위에서만 충돌)
	bool bIsVisible;            // 게임에서 보이는가
	wstring strName;            // 표시명
	wstring strDescription;     // 설명

	tCollisionInfo()
		: eType(COLLISION_TYPE::SOLID_GROUND)
		, displayColor(RGB(0, 0, 255))
		, bIsSolid(true)
		, bIsHarmful(false)
		, bIsOneWay(false)
		, bIsVisible(false)
		, strName(L"Solid Ground")
		, strDescription(L"Basic solid collision")
	{}

	tCollisionInfo(COLLISION_TYPE _type, COLORREF _color, bool _solid, bool _harmful, bool _oneway, bool _visible,
		const wstring& _name, const wstring& _desc)
		: eType(_type), displayColor(_color), bIsSolid(_solid), bIsHarmful(_harmful)
		, bIsOneWay(_oneway), bIsVisible(_visible), strName(_name), strDescription(_desc)
	{}
};

// =============================================================================
// === 타일 시스템 ===
// =============================================================================

// 타일 정보 구조체 (현재는 TRANSPARENT_BLOCK만 사용)
struct tTileInfo
{
	TILE_VISUAL_TYPE eType;      // 타일 시각적 타입
	Vec2 vDefaultSize;           // 기본 크기
	bool bDecorative;            // 장식용 타일인가
	bool bHarmful;               // 위험한 타일인가
	wstring strTexturePath;      // 텍스처 경로

	tTileInfo()
		: eType(TILE_VISUAL_TYPE::TRANSPARENT_BLOCK)
		, vDefaultSize(Vec2(64.f, 64.f))
		, bDecorative(false)
		, bHarmful(false)
		, strTexturePath(L"")
	{}

	tTileInfo(TILE_VISUAL_TYPE _eType, Vec2 _vSize, bool _bDecorative, bool _bHarmful, const wstring& _strPath)
		: eType(_eType)
		, vDefaultSize(_vSize)
		, bDecorative(_bDecorative)
		, bHarmful(_bHarmful)
		, strTexturePath(_strPath)
	{}
};

// =============================================================================
// === 레벨 에디터 시스템 ===
// =============================================================================

// 레벨 객체 데이터 (타일 시각 타입 정보 추가)
struct tLevelObjectData
{
	// 기본 필드들
	GROUP_TYPE eGroupType;
	Vec2 vPos;
	Vec2 vScale;
	int iSubType;
	int iTileVisualType;
	int iCollisionType;
	
	// 몬스터 방향 정보
	float fDirection;
	
	// 트리거 블록 전용 카메라 좌표 (트리거가 아닌 경우 무시됨)
	Vec2 vBossLockPos;

	tLevelObjectData()
		: eGroupType(GROUP_TYPE::END)
		, vPos(Vec2(0.f, 0.f))
		, vScale(Vec2(64.f, 64.f))
		, iSubType(0)
		, iTileVisualType(-1)
		, iCollisionType(-1)
		, fDirection(1.f)
		, vBossLockPos(Vec2(400.f, 300.f))
	{}
};

// 레벨 전체 데이터 (배경 정보 + 경계 정보 추가)
struct tLevelData
{
	// 기본 필드들
	wstring strLevelName;
	int iVersion;
	Vec2 vPlayerSpawn;
	vector<tLevelObjectData> vecObjects;

	// 배경 시스템
	int iBackgroundType;

	// 경계 시스템
	Vec2 vLevelBoundsMin;
	Vec2 vLevelBoundsMax;
	float fGameOverY;

	// 스테이지 이미지 시스템
	wstring strStageImagePath;
	STAGE_IMAGE_TYPE eStageType;
	Vec2 vStageImagePos;

	tLevelData()
		: strLevelName(L"Untitled")
		, iVersion(1)
		, vPlayerSpawn(Vec2(320.f, 320.f))
		, iBackgroundType(0)
		, vLevelBoundsMin(Vec2(0.f, 0.f))
		, vLevelBoundsMax(Vec2(4096.f, 640.f))
		, fGameOverY(736.f)
		, strStageImagePath(L"")
		, eStageType(STAGE_IMAGE_TYPE::STAGE_01)
		, vStageImagePos(Vec2(0.f, 0.f))
	{}
};

// =============================================================================
// === 문 전환 시스템 ===
// =============================================================================

// 레벨 데이터에 문 정보 추가용 확장 구조체
struct tDoorObjectData : public tLevelObjectData
{
	// 문 전용 확장 데이터
	SCENE_TYPE eTargetScene;    // 문의 목표 씬
	Vec2 vTargetPos;            // 문의 목표 위치
	float fInteractionRange;    // 상호작용 범위

	tDoorObjectData() : tLevelObjectData()
		, eTargetScene(SCENE_TYPE::STAGE_01)
		, vTargetPos(Vec2(100.f, 400.f))
		, fInteractionRange(80.f)
	{}
};
