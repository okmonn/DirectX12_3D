#pragma once
#include <DirectXMath.h>
#include <string>

// ウィンドウサイズ
#define WINDOW_X 640
#define WINDOW_Y 480

// 円周率
#define PI ((FLOAT)3.14159265359f)
// ラジアン変換
#define RAD(X) (X) * (PI / 180.0f)

// 解放処理マクロ
#ifndef _RELEASE
#define RELEASE(descriptor)      { if (descriptor != nullptr) { (descriptor)->Release(); (descriptor) = nullptr; } }
#endif

// 空間行列
struct WVP
{
	//ワールド
	DirectX::XMMATRIX world;
	//ビュープロジェクション
	DirectX::XMMATRIX viewProjection;
};

//頂点
struct Vertex
{
	//座標
	DirectX::XMFLOAT3 pos;
	//uv
	DirectX::XMFLOAT2 uv;
};

//二次元座標
template<typename T>
struct Vector2
{
	T x;
	T y;

	Vector2() : x(0), y(0)
	{}
	Vector2(T x, T y) : x(x), y(y)
	{}

	void operator=(const T a)
	{
		x = a;
		y = a;
	}
};