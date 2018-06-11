#include "Input.h"
#include "Window.h"
#include "KeyTbl.h"
#include "Typedef.h"
#include <tchar.h>

#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dinput8.lib")

// コンストラクタ
Input::Input(std::weak_ptr<Window>winAdr) : win(winAdr)
{
	//参照結果
	result = S_OK;

	// キーデータ
	key = {};

	//キー情報配列の初期化
	memset(&keys, 0, sizeof(keys));

	//前のキー情報配列の初期化
	memset(&olds, 0, sizeof(olds));


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


	//関数呼び出し
	result = SetInputDevice();
}

// デストラクタ
Input::~Input()
{
	RELEASE(key.input);
	RELEASE(key.dev);
}

// インプットの生成
HRESULT Input::CreateInput(void)
{
	//インプット生成
	result = DirectInput8Create(GetModuleHandle(0), VERSION, IID_IDirectInput8, (void**)(&key.input), NULL);
	if (FAILED(result))
	{
		OutputDebugString(_T("\nインプットの生成：失敗\n"));
		return result;
	}

	return result;
}

// インプットデバイスの生成
HRESULT Input::CreateInputDevice(void)
{
	result = CreateInput();
	if (FAILED(result))
	{
		return result;
	}

	//インプットデバイス生成
	result = key.input->CreateDevice(GUID_SysKeyboard, &key.dev, NULL);
	if (FAILED(result))
	{
		OutputDebugString(_T("\nインプットデバイスの生成：失敗\n"));
		return result;
	}

	return result;
}

// インプットデバイスをキーボードにセット
HRESULT Input::SetInputDevice(void)
{
	result = CreateInputDevice();
	if (FAILED(result))
	{
		return result;
	}

	//キーボードにセット
	{
		result = key.dev->SetDataFormat(&keybord);
		if (FAILED(result))
		{
			OutputDebugString(_T("\nインプットデバイスのキーボードセット：失敗\n"));
			return result;
		}
	}

	//協調レベルをセット
	{
		result = key.dev->SetCooperativeLevel(win.lock()->GetWindowHandle(), DISCL_NONEXCLUSIVE | DISCL_BACKGROUND);
		if (FAILED(result))
		{
			OutputDebugString(_T("\n協調レベルのセット：失敗\n"));
			return result;
		}
	}

	//入力デバイスへのアクセス権利を取得
	key.dev->Acquire();

	return result;
}

// キー入力
BOOL Input::InputKey(UINT data)
{
	//ダミー宣言
	BOOL flag = FALSE;

	//キー情報を取得
	key.dev->GetDeviceState(sizeof(keys), &keys);

	if (keys[data] & 0x80)
	{
		flag = TRUE;
	}

	olds[data] = keys[data];

	return flag;
}

// トリガー入力
BOOL Input::Trigger(UINT data)
{
	//ダミー宣言
	BOOL flag = FALSE;

	//キー情報を取得
	key.dev->GetDeviceState(sizeof(keys), &keys);

	if ((keys[data] & 0x80) && !(olds[data] & 0x80))
	{
		flag = TRUE;
	}

	olds[data] = keys[data];

	return flag;
}
