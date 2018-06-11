// ------------------------------------------------ --------------------------------------
//ファイル：WICTextureLoader12.h
//
// WICイメージを読み込んでDirect3Dランタイムテクスチャを作成する機能
//（可能であればミップマップを自動生成する）
//
//注：アプリケーションがすでにCoInitializeExを呼び出していると仮定します。
//
//これらの関数は、単純な2Dテクスチャとして作成された画像に便利です。 ためにDDSTextureLoaderは優れた軽量ランタイムローダーです。
//フル装備のDDSファイルリーダー、ライター、およびテクスチャ処理パイプラインについては、
// 'Texconv'サンプルと 'DirectXTex'ライブラリ。
//
// Copyright（c）Microsoft Corporation。 全著作権所有。
// MITライセンスでライセンスされています。
//
// http://go.microsoft.com/fwlink/?LinkId=248926
// http://go.microsoft.com/fwlink/?LinkID=615561
// ------------------------------------------------ --------------------------------------

#pragma once

#include <d3d12.h>
#include <stdint.h>
#include <memory>


namespace DirectX
{
    enum WIC_LOADER_FLAGS
    {
        WIC_LOADER_DEFAULT = 0,
        WIC_LOADER_FORCE_SRGB = 0x1,
        WIC_LOADER_IGNORE_SRGB = 0x2,
        WIC_LOADER_MIP_AUTOGEN = 0x4,
        WIC_LOADER_MIP_RESERVE = 0x8,
    };

	//標準バージョン
    HRESULT __cdecl LoadWICTextureFromMemory(
        _In_ ID3D12Device* d3dDevice,
        _In_reads_bytes_(wicDataSize) const uint8_t* wicData,
        size_t wicDataSize,
        _Outptr_ ID3D12Resource** texture,
        std::unique_ptr<uint8_t[]>& decodedData,
        D3D12_SUBRESOURCE_DATA& subresource,
        size_t maxsize = 0);

    HRESULT __cdecl LoadWICTextureFromFile(
        _In_ ID3D12Device* d3dDevice,
        _In_z_ const wchar_t* szFileName,
        _Outptr_ ID3D12Resource** texture,
        std::unique_ptr<uint8_t[]>& decodedData,
        D3D12_SUBRESOURCE_DATA& subresource,
        size_t maxsize = 0);

	//拡張版
    HRESULT __cdecl LoadWICTextureFromMemoryEx(
        _In_ ID3D12Device* d3dDevice,
        _In_reads_bytes_(wicDataSize) const uint8_t* wicData,
        size_t wicDataSize,
        size_t maxsize,
        D3D12_RESOURCE_FLAGS resFlags,
        unsigned int loadFlags,
        _Outptr_ ID3D12Resource** texture,
        std::unique_ptr<uint8_t[]>& decodedData,
        D3D12_SUBRESOURCE_DATA& subresource);

    HRESULT __cdecl LoadWICTextureFromFileEx(
        _In_ ID3D12Device* d3dDevice,
        _In_z_ const wchar_t* szFileName,
        size_t maxsize,
        D3D12_RESOURCE_FLAGS resFlags,
        unsigned int loadFlags,
        _Outptr_ ID3D12Resource** texture,
        std::unique_ptr<uint8_t[]>& decodedData,
        D3D12_SUBRESOURCE_DATA& subresource);
}