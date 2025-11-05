#pragma once
#include <d3d11.h>
#include "engine/Texture.h"

namespace engine {
	// 파일에서 로드(WIC)
	bool LoadTextureWIC ( ID3D11Device* device , const wchar_t* path , Tex2D* out );

	// 옵션: 1x1 단색 텍스처 유틸(파일 없이 파이프라인만 테스트할 때 유용)
	bool CreateSolidTexture1x1 ( ID3D11Device* device , unsigned int rgba /*0xAARRGGBB*/ , Tex2D* out );
}
