#pragma once
#include <DirectXMath.h>
#include <string>

// �E�B���h�E�T�C�Y
#define WINDOW_X 640
#define WINDOW_Y 480

// �~����
#define PI ((FLOAT)3.14159265359f)
// ���W�A���ϊ�
#define RAD(X) (X) * (PI / 180.0f)

// ��������}�N��
#ifndef _RELEASE
#define RELEASE(descriptor)      { if (descriptor != nullptr) { (descriptor)->Release(); (descriptor) = nullptr; } }
#endif

// ��ԍs��
struct WVP
{
	//���[���h
	DirectX::XMMATRIX world;
	//�r���[�v���W�F�N�V����
	DirectX::XMMATRIX viewProjection;
};

//���_
struct Vertex
{
	//���W
	DirectX::XMFLOAT3 pos;
	//uv
	DirectX::XMFLOAT2 uv;
};

//�񎟌����W
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