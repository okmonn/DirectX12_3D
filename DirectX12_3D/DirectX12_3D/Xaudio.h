#pragma once
#include <xaudio2.h>
#include <string>

class Xaudio
{
	// データ
	struct Data
	{
		//XAudio2
		IXAudio2*				audio;
		//マスタリング
		IXAudio2MasteringVoice*	voice;
	};

public:
	// コンストラクタ
	Xaudio();
	// デストラクタ
	~Xaudio();

	// WAVEの読み込み
	HRESULT LoadWAV(USHORT* index, std::string fileName);

private:
	// XAudio2の生成
	HRESULT CreateXaudio2(void);

	// マスタリングボイスの生成
	HRESULT CreateMasteringVoice(void);


	// 参照結果
	HRESULT result;

	// データ
	Data data;

};

