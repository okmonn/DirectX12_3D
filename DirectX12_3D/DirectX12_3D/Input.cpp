#include "Input.h"
#include "Window.h"
#include "KeyTbl.h"
#include "Typedef.h"
#include <tchar.h>

#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dinput8.lib")

// �R���X�g���N�^
Input::Input(std::weak_ptr<Window>winAdr) : win(winAdr)
{
	//�Q�ƌ���
	result = S_OK;

	// �L�[�f�[�^
	key = {};

	//�L�[���z��̏�����
	memset(&keys, 0, sizeof(keys));

	//�O�̃L�[���z��̏�����
	memset(&olds, 0, sizeof(olds));


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


	//�֐��Ăяo��
	result = SetInputDevice();
}

// �f�X�g���N�^
Input::~Input()
{
	RELEASE(key.input);
	RELEASE(key.dev);
}

// �C���v�b�g�̐���
HRESULT Input::CreateInput(void)
{
	//�C���v�b�g����
	result = DirectInput8Create(GetModuleHandle(0), VERSION, IID_IDirectInput8, (void**)(&key.input), NULL);
	if (FAILED(result))
	{
		OutputDebugString(_T("\n�C���v�b�g�̐����F���s\n"));
		return result;
	}

	return result;
}

// �C���v�b�g�f�o�C�X�̐���
HRESULT Input::CreateInputDevice(void)
{
	result = CreateInput();
	if (FAILED(result))
	{
		return result;
	}

	//�C���v�b�g�f�o�C�X����
	result = key.input->CreateDevice(GUID_SysKeyboard, &key.dev, NULL);
	if (FAILED(result))
	{
		OutputDebugString(_T("\n�C���v�b�g�f�o�C�X�̐����F���s\n"));
		return result;
	}

	return result;
}

// �C���v�b�g�f�o�C�X���L�[�{�[�h�ɃZ�b�g
HRESULT Input::SetInputDevice(void)
{
	result = CreateInputDevice();
	if (FAILED(result))
	{
		return result;
	}

	//�L�[�{�[�h�ɃZ�b�g
	{
		result = key.dev->SetDataFormat(&keybord);
		if (FAILED(result))
		{
			OutputDebugString(_T("\n�C���v�b�g�f�o�C�X�̃L�[�{�[�h�Z�b�g�F���s\n"));
			return result;
		}
	}

	//�������x�����Z�b�g
	{
		result = key.dev->SetCooperativeLevel(win.lock()->GetWindowHandle(), DISCL_NONEXCLUSIVE | DISCL_BACKGROUND);
		if (FAILED(result))
		{
			OutputDebugString(_T("\n�������x���̃Z�b�g�F���s\n"));
			return result;
		}
	}

	//���̓f�o�C�X�ւ̃A�N�Z�X�������擾
	key.dev->Acquire();

	return result;
}

// �L�[����
BOOL Input::InputKey(UINT data)
{
	//�_�~�[�錾
	BOOL flag = FALSE;

	//�L�[�����擾
	key.dev->GetDeviceState(sizeof(keys), &keys);

	if (keys[data] & 0x80)
	{
		flag = TRUE;
	}

	olds[data] = keys[data];

	return flag;
}

// �g���K�[����
BOOL Input::Trigger(UINT data)
{
	//�_�~�[�錾
	BOOL flag = FALSE;

	//�L�[�����擾
	key.dev->GetDeviceState(sizeof(keys), &keys);

	if ((keys[data] & 0x80) && !(olds[data] & 0x80))
	{
		flag = TRUE;
	}

	olds[data] = keys[data];

	return flag;
}
