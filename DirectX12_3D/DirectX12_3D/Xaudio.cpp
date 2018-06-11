#include "Xaudio.h"
#include "Typedef.h"
#include <d3d12.h>
#include <tchar.h>

#pragma comment(lib, "xaudio2.lib")
#pragma comment(lib, "d3d12.lib")

// �R���X�g���N�^
Xaudio::Xaudio()
{
	CoInitializeEx(nullptr, COINIT_MULTITHREADED);

	result = S_OK;
	data = {};


	//�G���[���o�͂ɕ\��������
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

// �f�X�g���N�^
Xaudio::~Xaudio()
{
	data.voice->DestroyVoice();
	RELEASE(data.audio);
	CoUninitialize();
}

// WAVE�̓ǂݍ���
HRESULT Xaudio::LoadWAV(USHORT * index, std::string fileName)
{
	return E_NOTIMPL;
}

// XAudio2�̐���
HRESULT Xaudio::CreateXaudio2(void)
{
	if (data.audio == nullptr)
	{
		result = XAudio2Create(&data.audio, 0);
		if (FAILED(result))
		{
			OutputDebugString(_T("\nXAudio2�̐����F���s\n"));
			return result;
		}
	}

	return result;
}

// �}�X�^�����O�{�C�X�̐���
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
		OutputDebugString(_T("�}�X�^�����O�{�C�X�̐����F���s\n"));
		return result;
	}

	return result;
}
