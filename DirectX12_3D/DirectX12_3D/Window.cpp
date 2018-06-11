#include "Window.h"
#include "Typedef.h"
#include <tchar.h>

// �R���X�g���N�^
Window::Window()
{
	//�E�B���h�E�n���h��
	windowHandle = nullptr;

	//�E�B���h�E�ݒ�p�\����
	window = {};

	//�E�B���h�E�T�C�Y�ݒ�p�\����
	rect = {};


	//�֐��Ăяo��
	CreateWnd();
}

// �f�X�g���N�^
Window::~Window()
{
	//�E�B���h�E�N���X�̏���,�������̉��
	UnregisterClass(window.lpszClassName, window.hInstance);
}

// �E�B���h�E�v���V�[�W��
LRESULT Window::WindowProcedure(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
	//�E�B���h�E���j�����ꂽ�Ƃ�
	if (msg == WM_DESTROY)
	{
		//OS�ɑ΂��ăA�v���P�[�V�����I����`����
		PostQuitMessage(0);
		return 0;
	}

	//�K��̏������s��
	return DefWindowProc(hwnd, msg, wparam, lparam);
}

// �E�B���h�E�̐���
void Window::CreateWnd(void)
{
	//�E�B���h�E�ݒ�p�\���̂̐ݒ�
	{
		window.cbClsExtra		= 0;
		window.cbSize			= sizeof(WNDCLASSEX);
		window.cbWndExtra		= 0;
		window.hbrBackground	= CreateSolidBrush(0x000000);
		window.hCursor			= LoadCursor(NULL, IDC_ARROW);
		window.hIcon			= LoadCursor(NULL, IDI_APPLICATION);
		window.hIconSm			= LoadIcon(NULL, IDI_APPLICATION);
		window.hInstance		= GetModuleHandle(0);
		window.lpfnWndProc		= (WNDPROC)WindowProcedure;
		window.lpszClassName	= _T("DirectX12");
		window.lpszMenuName		= _T("DirectX12");
		window.style			= CS_HREDRAW;
	}

	//�E�B���h�E�̓o�^
	RegisterClassEx(&window);

	//�E�B���h�E�T�C�Y�ݒ�p�\���̂̐ݒ�
	{
		rect.bottom		= WINDOW_Y;
		rect.left		= 0;
		rect.right		= WINDOW_X;
		rect.top		= 0;
	}

	//�T�C�Y�̕␳
	AdjustWindowRect(&rect, WS_OVERLAPPEDWINDOW, false);

	//�E�B���h�E����
	windowHandle = CreateWindow(window.lpszClassName, _T("DirectX12"), WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, (rect.right - rect.left), (rect.bottom - rect.top), nullptr, nullptr, window.hInstance, nullptr);
}

// �E�B���h�E�n���h���̎擾
HWND Window::GetWindowHandle(void)
{
	return windowHandle;
}
