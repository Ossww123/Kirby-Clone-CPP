#pragma once
#include <vector>
#include <cstdint>
#include <memory>
#include <wrl/client.h>
#include <d3d11.h>
#include <d3dcompiler.h>
#pragma comment(lib, "d3dcompiler.lib")

#include "engine/Texture.h" // Tex2D (srv: ComPtr)

namespace engine {

    // 간단 프리셋
    enum class BlendMode : uint8_t { Alpha = 0 , Additive = 1 };
    enum class SamplerMode : uint8_t { Linear = 0 , Point = 1 };

    // 정점 포맷
    struct SpriteVertex {
        float x , y;     // screen space
        float u , v;
        uint32_t rgba;  // 0xAARRGGBB
    };

    // 수집 커맨드(정렬 키 포함)
    struct SpriteItem {
        uint64_t sortKeyHi = 0;    // [blend:8 | sampler:8 | z:16 | pad:32]
        uint64_t seq = 0;          // 제출 순서(안정 타이브레이커)
        const Tex2D* tex = nullptr;
        RECT   src{ 0,0,0,0 };
        float  x = 0 , y = 0 , w = 0 , h = 0;
        float  rotation = 0 , originX = 0 , originY = 0;
        uint32_t rgba = 0xFFFFFFFF;
    };

    class D3D11SpriteBatch {
    public:
        D3D11SpriteBatch ( ) = default;
        ~D3D11SpriteBatch ( );

        bool Initialize ( ID3D11Device* dev , ID3D11DeviceContext* ctx , int screenW , int screenH );
        void OnResize ( int w , int h ) { SetProjection ( w , h ); }

        // 수집 시작/끝
        void Begin ( );              // 파이프라인(VS/PS/IL/CB/샘플러/블렌드/래스터) 바인드 포함
        void End ( );                // 수집 정렬/그룹화/버퍼업로드/드로우

        // 기존 시그니처 유지
        void Draw ( const Tex2D& tex ,
                  float x , float y , float w , float h ,
                  const RECT* srcPixels = nullptr ,
                  uint32_t tintRGBA = 0xFFFFFFFF ,
                  float rotation = 0.f , float originX = 0.f , float originY = 0.f );

        // v2: z/블렌드/샘플러 지정
        void Draw ( const Tex2D& tex ,
                  float x , float y , float w , float h ,
                  const RECT* srcPixels ,
                  uint32_t tintRGBA ,
                  float rotation ,
                  float originX , float originY ,
                  int16_t zSort ,
                  BlendMode blend = BlendMode::Alpha ,
                  SamplerMode sampler = SamplerMode::Point );

        // 기본 프리셋
        void SetDefaultBlend ( BlendMode m ) { m_defaultBlend = m; }
        void SetDefaultSampler ( SamplerMode m ) { m_defaultSampler = m; }

    private:
        // 파이프라인/상수
        bool CreatePipeline ( );
        void CreateStates ( );
        void SetProjection ( int w , int h );

        // 내부 도우미
        void ensureVB ( size_t vertices );
        void ensureIB ( size_t indices );
        void flushBatches ( );

        void applyBlend ( BlendMode );
        void applySampler ( SamplerMode );
        void applyTexture ( const Tex2D* );

        static UINT D3DCompileFlagsRowMajor ( );

    private:
        // 디바이스/컨텍스트
        ID3D11Device* m_dev = nullptr;
        ID3D11DeviceContext* m_ctx = nullptr;

        // 셰이더/IL/상수
        Microsoft::WRL::ComPtr<ID3D11VertexShader> m_vs;
        Microsoft::WRL::ComPtr<ID3D11PixelShader>  m_ps;
        Microsoft::WRL::ComPtr<ID3D11InputLayout>  m_layout;
        Microsoft::WRL::ComPtr<ID3D11Buffer>       m_cbProj; // 4x4

        // 상태
        Microsoft::WRL::ComPtr<ID3D11BlendState>   m_blendAlpha;
        Microsoft::WRL::ComPtr<ID3D11BlendState>   m_blendAdd;
        Microsoft::WRL::ComPtr<ID3D11SamplerState> m_sampLinear;
        Microsoft::WRL::ComPtr<ID3D11SamplerState> m_sampPoint;
        Microsoft::WRL::ComPtr<ID3D11RasterizerState> m_rsCullNone;

        // 버퍼
        Microsoft::WRL::ComPtr<ID3D11Buffer> m_vb; // DYNAMIC
        Microsoft::WRL::ComPtr<ID3D11Buffer> m_ib; // DYNAMIC
        size_t m_vbCapacity = 0; // vertices
        size_t m_ibCapacity = 0; // indices

        // 수집/작업 버퍼
        std::vector<SpriteItem>    m_items;
        std::vector<SpriteVertex>  m_vertices;
        std::vector<uint16_t>      m_indices;

        // 상태 캐시
        const Tex2D* m_boundTex = nullptr;
        BlendMode    m_boundBlend = static_cast< BlendMode >( 0xFF );
        SamplerMode  m_boundSampler = static_cast< SamplerMode >( 0xFF );

        // 기본 모드
        BlendMode    m_defaultBlend = BlendMode::Alpha;
        SamplerMode  m_defaultSampler = SamplerMode::Point;

        bool m_inBegin = false;
        uint64_t m_seq = 0;
    };

} // namespace engine
