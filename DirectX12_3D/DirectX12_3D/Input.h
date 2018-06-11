#pragma once
#define INITGUID
#include <d3d12.h>
#include <dinput.h>
#include <memory>

// �o�[�W����
#define VERSION 0x0800

class Window;

class Input
{
	// �L�[�f�[�^
	struct Key
	{
		// �C���v�b�g
		LPDIRECTINPUT8			input;
		// �C���v�b�g�f�o�C�X
		LPDIRECTINPUTDEVICE8	dev;
	};

public:
	// �R���X�g���N�^
	Input(std::weak_ptr<Window>winAdr);
	// �f�X�g���N�^
	~Input();

	// �L�[����
	BOOL InputKey(UINT data);

	// �g���K�[����
	BOOL Trigger(UINT data);

private:
	// �C���v�b�g�̐���
	HRESULT CreateInput(void);
	// �C���v�b�g�f�o�C�X�̐���
	HRESULT CreateInputDevice(void);


	// �C���v�b�g�f�o�C�X���L�[�{�[�h�ɃZ�b�g
	HRESULT SetInputDevice(void);


	// �E�B���h�E�N���X�Q��
	std::weak_ptr<Window>win;

	// �Q�ƌ���
	HRESULT result;

	// �L�[�f�[�^
	Key key;

	// �L�[���
	BYTE keys[256];

	// �O�̃L�[���
	BYTE olds[256];
};