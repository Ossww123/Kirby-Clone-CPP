// engine/render/D3D11TextureLoader.cpp
//
// Responsibility: Create Tex2D from WIC file or a 1x1 solid color on D3D11.
// Non-Goals: Streaming, DDS/KTX loading, mipmap generation.
// Call-Context: Main thread; requires COM initialized by caller (CoInitializeEx).
//

#ifndef NOMINMAX
#define NOMINMAX
#endif

#include "engine/render/TextureLoader.h"
#include "engine/render/Texture.h"

#include <d3d11.h>
#include <wrl/client.h>
#include <wincodec.h>
#include <vector>
#include <cstdint>

#pragma comment(lib, "windowscodecs.lib")

namespace engine {

    using Microsoft::WRL::ComPtr;

    // Clear output on failure paths
    static inline void ResetOutTex ( Tex2D* out ) {
        if ( !out ) return;
        out->srv.Reset ( );
        out->width = 0;
        out->height = 0;
    }

    bool CreateSolidTexture1x1 ( ID3D11Device* dev , unsigned int rgba , Tex2D* out )
    {
        if ( !dev || !out ) return false;
        ResetOutTex ( out );

        D3D11_TEXTURE2D_DESC td{};
        td.Width = 1; td.Height = 1;
        td.MipLevels = 1; td.ArraySize = 1;
        td.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        td.SampleDesc.Count = 1;
        td.Usage = D3D11_USAGE_IMMUTABLE;
        td.BindFlags = D3D11_BIND_SHADER_RESOURCE;

        D3D11_SUBRESOURCE_DATA srd{};
        srd.pSysMem = &rgba;
        srd.SysMemPitch = 4;

        ComPtr<ID3D11Texture2D> tex;
        if ( FAILED ( dev->CreateTexture2D ( &td , &srd , &tex ) ) ) return false;

        D3D11_SHADER_RESOURCE_VIEW_DESC sd{};
        sd.Format = td.Format;
        sd.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
        sd.Texture2D.MostDetailedMip = 0;
        sd.Texture2D.MipLevels = 1;

        ComPtr<ID3D11ShaderResourceView> srv;
        if ( FAILED ( dev->CreateShaderResourceView ( tex.Get ( ) , &sd , &srv ) ) ) return false;

        out->srv = srv;
        out->width = 1;
        out->height = 1;
        return true;
    }

    bool LoadTextureWIC ( ID3D11Device* device , const wchar_t* path , Tex2D* out )
    {
        if ( !device || !path || !out ) return false;
        ResetOutTex ( out );

        // WIC factory
        ComPtr<IWICImagingFactory> factory;
        HRESULT hr = CoCreateInstance (
            CLSID_WICImagingFactory , nullptr , CLSCTX_INPROC_SERVER ,
            IID_PPV_ARGS ( &factory )
        );
        if ( FAILED ( hr ) ) return false;

        // Decoder / frame
        ComPtr<IWICBitmapDecoder> decoder;
        hr = factory->CreateDecoderFromFilename (
            path , nullptr , GENERIC_READ , WICDecodeMetadataCacheOnDemand , &decoder
        );
        if ( FAILED ( hr ) ) return false;

        ComPtr<IWICBitmapFrameDecode> frame;
        if ( FAILED ( decoder->GetFrame ( 0 , &frame ) ) ) return false;

        // Convert to 32bpp RGBA (non-premultiplied) for our blend state
        ComPtr<IWICFormatConverter> conv;
        if ( FAILED ( factory->CreateFormatConverter ( &conv ) ) ) return false;

        hr = conv->Initialize (
            frame.Get ( ) , GUID_WICPixelFormat32bppRGBA ,
            WICBitmapDitherTypeNone , nullptr , 0.0 , WICBitmapPaletteTypeCustom
        );
        if ( FAILED ( hr ) ) return false;

        UINT w = 0 , h = 0;
        conv->GetSize ( &w , &h );
        if ( w == 0 || h == 0 ) return false;

        std::vector<std::uint8_t> pixels ( static_cast< size_t >( w ) * h * 4 );
        hr = conv->CopyPixels ( nullptr ,
                              static_cast< UINT >( w * 4 ) ,
                              static_cast< UINT >( pixels.size ( ) ) ,
                              pixels.data ( ) );
        if ( FAILED ( hr ) ) return false;

        // D3D11 texture + SRV
        D3D11_TEXTURE2D_DESC td{};
        td.Width = w; td.Height = h;
        td.MipLevels = 1; td.ArraySize = 1;
        td.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        td.SampleDesc.Count = 1;
        td.Usage = D3D11_USAGE_IMMUTABLE;
        td.BindFlags = D3D11_BIND_SHADER_RESOURCE;

        D3D11_SUBRESOURCE_DATA srd{};
        srd.pSysMem = pixels.data ( );
        srd.SysMemPitch = static_cast< UINT >( w * 4 );

        ComPtr<ID3D11Texture2D> tex;
        if ( FAILED ( device->CreateTexture2D ( &td , &srd , &tex ) ) ) return false;

        D3D11_SHADER_RESOURCE_VIEW_DESC sd{};
        sd.Format = td.Format;
        sd.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
        sd.Texture2D.MostDetailedMip = 0;
        sd.Texture2D.MipLevels = 1;

        ComPtr<ID3D11ShaderResourceView> srv;
        if ( FAILED ( device->CreateShaderResourceView ( tex.Get ( ) , &sd , &srv ) ) ) return false;

        out->srv = srv;
        out->width = static_cast< int >( w );
        out->height = static_cast< int >( h );
        return true;
    }

} // namespace engine
