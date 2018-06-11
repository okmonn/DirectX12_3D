#pragma once
#include <xaudio2.h>
#include <string>

class Xaudio
{
	// �f�[�^
	struct Data
	{
		//XAudio2
		IXAudio2*				audio;
		//�}�X�^�����O
		IXAudio2MasteringVoice*	voice;
	};

public:
	// �R���X�g���N�^
	Xaudio();
	// �f�X�g���N�^
	~Xaudio();

	// WAVE�̓ǂݍ���
	HRESULT LoadWAV(USHORT* index, std::string fileName);

private:
	// XAudio2�̐���
	HRESULT CreateXaudio2(void);

	// �}�X�^�����O�{�C�X�̐���
	HRESULT CreateMasteringVoice(void);


	// �Q�ƌ���
	HRESULT result;

	// �f�[�^
	Data data;

};

