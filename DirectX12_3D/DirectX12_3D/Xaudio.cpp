#include "Xaudio.h"
#include "Typedef.h"
#include <d3d12.h>
#include <tchar.h>

#pragma comment(lib, "xaudio2.lib")
#pragma comment(lib, "d3d12.lib")

// コンストラクタ
Xaudio::Xaudio()
{
	CoInitializeEx(nullptr, COINIT_MULTITHREADED);

	result = S_OK;
	data = {};


	//エラーを出力に表示させる
#ifdef _DEBUG
	ID3D12Debug *debug = nullptr;
	result = D3D12GetDebugInterface(IID_PPV_ARGS(&debug));
	if (FAILED(result))
		int i = 0;
	debug->EnableDebugLayer();
	debug->Release();
	debug = nullptr;
#endif


	CreateMasteringVoice();
}

// デストラクタ
Xaudio::~Xaudio()
{
	data.voice->DestroyVoice();
	RELEASE(data.audio);
	CoUninitialize();
}

// WAVEの読み込み
HRESULT Xaudio::LoadWAV(USHORT * index, std::string fileName)
{
	return E_NOTIMPL;
}

// XAudio2の生成
HRESULT Xaudio::CreateXaudio2(void)
{
	if (data.audio == nullptr)
	{
		result = XAudio2Create(&data.audio, 0);
		if (FAILED(result))
		{
			OutputDebugString(_T("\nXAudio2の生成：失敗\n"));
			return result;
		}
	}

	return result;
}

// マスタリングボイスの生成
HRESULT Xaudio::CreateMasteringVoice(void)
{
	result = CreateXaudio2();
	if (FAILED(result))
	{
		return result;
	}

	result = data.audio->CreateMasteringVoice(&data.voice);
	if (FAILED(result))
	{
		OutputDebugString(_T("マスタリングボイスの生成：失敗\n"));
		return result;
	}

	return result;
}
