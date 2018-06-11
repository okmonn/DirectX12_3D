#pragma once
#include <Windows.h>

class Window
{
public:
	// コンストラクタ
	Window();
	// デストラクタ
	~Window();

	// ウィンドウプロシージャ
	static LRESULT WindowProcedure(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);

	// ウィンドウハンドルの取得
	HWND GetWindowHandle(void);

private:
	// ウィンドウの生成
	void CreateWnd(void);


	// ウィンドウハンドル
	HWND windowHandle;

	// ウィンドウの設定用構造体
	WNDCLASSEX window;

	// ウィンドウサイズの設定用構造体
	RECT rect;
};

