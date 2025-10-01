#pragma once
#include <wrl/client.h>

// 전방 선언만으로 충분 (ComPtr은 인터페이스 전방 선언과 함께 사용 가능)
struct ID3D11ShaderResourceView;

namespace engine {

    // D3D11용 간단 텍스처 핸들
    struct Tex2D {
        Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> srv;
        int width = 0;
        int height = 0;
    };

} // namespace engine
