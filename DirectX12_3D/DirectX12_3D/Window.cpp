#include "Window.h"
#include "Typedef.h"
#include <tchar.h>

// コンストラクタ
Window::Window()
{
	//ウィンドウハンドル
	windowHandle = nullptr;

	//ウィンドウ設定用構造体
	window = {};

	//ウィンドウサイズ設定用構造体
	rect = {};


	//関数呼び出し
	CreateWnd();
}

// デストラクタ
Window::~Window()
{
	//ウィンドウクラスの消去,メモリの解放
	UnregisterClass(window.lpszClassName, window.hInstance);
}

// ウィンドウプロシージャ
LRESULT Window::WindowProcedure(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
	//ウィンドウが破棄されたとき
	if (msg == WM_DESTROY)
	{
		//OSに対してアプリケーション終了を伝える
		PostQuitMessage(0);
		return 0;
	}

	//規定の処理を行う
	return DefWindowProc(hwnd, msg, wparam, lparam);
}

// ウィンドウの生成
void Window::CreateWnd(void)
{
	//ウィンドウ設定用構造体の設定
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

	//ウィンドウの登録
	RegisterClassEx(&window);

	//ウィンドウサイズ設定用構造体の設定
	{
		rect.bottom		= WINDOW_Y;
		rect.left		= 0;
		rect.right		= WINDOW_X;
		rect.top		= 0;
	}

	//サイズの補正
	AdjustWindowRect(&rect, WS_OVERLAPPEDWINDOW, false);

	//ウィンドウ生成
	windowHandle = CreateWindow(window.lpszClassName, _T("DirectX12"), WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, (rect.right - rect.left), (rect.bottom - rect.top), nullptr, nullptr, window.hInstance, nullptr);
}

// ウィンドウハンドルの取得
HWND Window::GetWindowHandle(void)
{
	return windowHandle;
}
