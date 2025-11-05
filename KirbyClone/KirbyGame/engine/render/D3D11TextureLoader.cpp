#include "engine/TextureLoader.h"
#include "engine/Texture.h"

#include <wrl/client.h>
#include <wincodec.h>
#include <vector>

#pragma comment(lib, "windowscodecs.lib")

namespace engine {

    bool CreateSolidTexture1x1 ( ID3D11Device* dev , unsigned int rgba , Tex2D* out ) {
        if ( !dev || !out ) return false;

        D3D11_TEXTURE2D_DESC td{};
        td.Width = 1; td.Height = 1; td.MipLevels = 1; td.ArraySize = 1;
        td.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        td.SampleDesc.Count = 1;
        td.Usage = D3D11_USAGE_IMMUTABLE;
        td.BindFlags = D3D11_BIND_SHADER_RESOURCE;

        D3D11_SUBRESOURCE_DATA srd{};
        srd.pSysMem = &rgba;
        srd.SysMemPitch = 4;

        Microsoft::WRL::ComPtr<ID3D11Texture2D> tex;
        if ( FAILED ( dev->CreateTexture2D ( &td , &srd , &tex ) ) ) return false;

        D3D11_SHADER_RESOURCE_VIEW_DESC sd{};
        sd.Format = td.Format;
        sd.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
        sd.Texture2D.MostDetailedMip = 0;
        sd.Texture2D.MipLevels = 1;

        Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> srv;
        if ( FAILED ( dev->CreateShaderResourceView ( tex.Get ( ) , &sd , &srv ) ) ) return false;

        out->srv = srv;
        out->width = 1;
        out->height = 1;
        return true;
    }

    bool LoadTextureWIC ( ID3D11Device* device , const wchar_t* path , Tex2D* out ) {
        if ( !device || !path || !out ) return false;

        using Microsoft::WRL::ComPtr;

        // WIC 팩토리
        ComPtr<IWICImagingFactory> factory;
        HRESULT hr = CoCreateInstance (
            CLSID_WICImagingFactory , nullptr , CLSCTX_INPROC_SERVER ,
            IID_PPV_ARGS ( &factory )
        );
        if ( FAILED ( hr ) ) return false;

        // 디코더/프레임
        ComPtr<IWICBitmapDecoder> decoder;
        hr = factory->CreateDecoderFromFilename (
            path , nullptr , GENERIC_READ , WICDecodeMetadataCacheOnDemand , &decoder
        );
        if ( FAILED ( hr ) ) return false;

        ComPtr<IWICBitmapFrameDecode> frame;
        if ( FAILED ( decoder->GetFrame ( 0 , &frame ) ) ) return false;

        // 32bpp RGBA로 변환 (비 premultiplied → 우리 블렌딩과 일치)
        ComPtr<IWICFormatConverter> conv;
        if ( FAILED ( factory->CreateFormatConverter ( &conv ) ) ) return false;
        hr = conv->Initialize (
            frame.Get ( ) , GUID_WICPixelFormat32bppRGBA ,
            WICBitmapDitherTypeNone , nullptr , 0.0 , WICBitmapPaletteTypeCustom
        );
        if ( FAILED ( hr ) ) return false;

        UINT w = 0 , h = 0;
        conv->GetSize ( &w , &h );
        std::vector<uint8_t> pixels ( static_cast< size_t >( w ) * h * 4 );
        hr = conv->CopyPixels ( nullptr , w * 4 , static_cast< UINT >( pixels.size ( ) ) , pixels.data ( ) );
        if ( FAILED ( hr ) ) return false;

        // D3D11 텍스처/SRV
        D3D11_TEXTURE2D_DESC td{};
        td.Width = w; td.Height = h;
        td.MipLevels = 1; td.ArraySize = 1;
        td.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        td.SampleDesc.Count = 1;
        td.Usage = D3D11_USAGE_IMMUTABLE;
        td.BindFlags = D3D11_BIND_SHADER_RESOURCE;

        D3D11_SUBRESOURCE_DATA srd{};
        srd.pSysMem = pixels.data ( );
        srd.SysMemPitch = w * 4;

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
