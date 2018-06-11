#pragma once
#define INITGUID
#include <d3d12.h>
#include <dinput.h>
#include <memory>

// バージョン
#define VERSION 0x0800

class Window;

class Input
{
	// キーデータ
	struct Key
	{
		// インプット
		LPDIRECTINPUT8			input;
		// インプットデバイス
		LPDIRECTINPUTDEVICE8	dev;
	};

public:
	// コンストラクタ
	Input(std::weak_ptr<Window>winAdr);
	// デストラクタ
	~Input();

	// キー入力
	BOOL InputKey(UINT data);

	// トリガー入力
	BOOL Trigger(UINT data);

private:
	// インプットの生成
	HRESULT CreateInput(void);
	// インプットデバイスの生成
	HRESULT CreateInputDevice(void);


	// インプットデバイスをキーボードにセット
	HRESULT SetInputDevice(void);


	// ウィンドウクラス参照
	std::weak_ptr<Window>win;

	// 参照結果
	HRESULT result;

	// キーデータ
	Key key;

	// キー情報
	BYTE keys[256];

	// 前のキー情報
	BYTE olds[256];
};