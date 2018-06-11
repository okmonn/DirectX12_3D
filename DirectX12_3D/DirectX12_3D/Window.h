#pragma once
#include <Windows.h>

class Window
{
public:
	// �R���X�g���N�^
	Window();
	// �f�X�g���N�^
	~Window();

	// �E�B���h�E�v���V�[�W��
	static LRESULT WindowProcedure(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);

	// �E�B���h�E�n���h���̎擾
	HWND GetWindowHandle(void);

private:
	// �E�B���h�E�̐���
	void CreateWnd(void);


	// �E�B���h�E�n���h��
	HWND windowHandle;

	// �E�B���h�E�̐ݒ�p�\����
	WNDCLASSEX window;

	// �E�B���h�E�T�C�Y�̐ݒ�p�\����
	RECT rect;
};

