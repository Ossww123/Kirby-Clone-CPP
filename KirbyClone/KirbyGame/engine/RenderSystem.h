#pragma once
#include <memory>
#include <utility>
#include "engine/IRenderer.h"          // Color, IRenderer
#include "engine/D3D11SpriteBatch.h"   // SpriteBatch
#include "engine/D3D11DebugDraw.h"     // Debug draw
#include "engine/Camera.h"             // Camera
#include "engine/Texture.h"            // Tex2D

namespace engine {

    struct RenderConfig {
        bool  usePixelSnap = true;           // (참고) 카메라에서 처리 중
        float zoom = 1.0f;           // 카메라 줌 (월드→스크린 변환에 반영)
        bool  usePointSamplerForPixels = true; // (확장용) 픽셀아트 시 포인트 샘플러
    };

    // 고수준 렌더 오케스트라: 프레임 라이프사이클/카메라 변환/상태 캐시/그리기 파사드
    class RenderSystem {
    public:
        RenderSystem ( ) = default;
        ~RenderSystem ( ) = default;

        // D3D11Renderer를 전달 (IRenderer 파생)
        bool Init ( IRenderer* renderer );
        void OnResize ( int w , int h );

        // 프레임 라이프사이클
        void Begin ( const Color& clear );
        void End ( );

        // 상태/정책
        void SetConfig ( const RenderConfig& cfg );
        const RenderConfig& GetConfig ( ) const { return m_cfg; }

        void SetBlendMode ( uint8_t mode );   // 0: alpha, 1: additive, ... (확장용)
        void SetSamplerMode ( uint8_t mode ); // 0: linear, 1: point, ...  (확장용)

        // 카메라
        void SetCamera ( const Camera* cam ) { m_cam = cam; }
        const Camera* GetCamera ( ) const { return m_cam; }

        // 월드→스크린 변환(카메라 오프셋 + 줌)
        std::pair<float , float> ToScreen ( float wx , float wy ) const;
        float Zoom ( ) const { return m_cfg.zoom; }

        // 그리기 파사드 (필요시 계속 추가)
        void DrawSprite ( const Tex2D& tex ,
                        float wx , float wy , float w , float h ,
                        const RECT* src = nullptr ,
                        uint32_t rgba = 0xFFFFFFFF ,
                        float rotation = 0.f ,
                        float originX = 0.f , float originY = 0.f ,
                        float z = 0.f , uint8_t sortBlend = 0 );

        // 하위 시스템 접근(월드 렌더 등의 직접 호출용)
        D3D11SpriteBatch& Batch ( ) { return *m_batch; }
        D3D11DebugDraw& Debug ( ) { return *m_dbg; }

        // 렌더러 크기 질의
        int BackbufferWidth ( )  const;
        int BackbufferHeight ( ) const;

    private:
        IRenderer* m_renderer{};                            // 비소유
        std::unique_ptr<D3D11SpriteBatch> m_batch;          // 커맨드 수집/정렬/드로우
        std::unique_ptr<D3D11DebugDraw>   m_dbg;            // 디버그 프리미티브

        const Camera* m_cam{};                               // 비소유
        RenderConfig  m_cfg{};

        // 경량 상태 캐시(블렌드/샘플러 모드 키) - 확장 시 SpriteBatch 정렬키로 연결
        uint8_t m_curBlend = 0xFF;
        uint8_t m_curSampler = 0xFF;
    };

} // namespace engine
