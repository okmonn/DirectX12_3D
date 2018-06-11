#pragma once
#include "Device.h"

class Texture
{
	// サイズ
	struct Size
	{
		LONG	width;
		LONG	height;
	};

	// 頂点データ
	struct VertexData
	{
		//頂点データ
		Vertex						vertex[6];
		//リソース
		ID3D12Resource*				resource;
		//送信データ
		UCHAR*						data;
		// 頂点バッファビュー
		D3D12_VERTEX_BUFFER_VIEW	view;
	};
	
	// BMPデータ
	struct BMP
	{
		//画像サイズ
		Size					size;
		//bmpデータ
		std::vector<UCHAR>		data;
		//ヒープ
		ID3D12DescriptorHeap*	heap;
		//リソース
		ID3D12Resource*			resource;
		//頂点データ
		VertexData				vertex;
	};

	// WICデータ
	struct WIC
	{
		//ヒープ
		ID3D12DescriptorHeap*		heap;
		//リソース
		ID3D12Resource*				resource;
		//デコード
		std::unique_ptr<uint8_t[]>	decode;
		//サブ
		D3D12_SUBRESOURCE_DATA		sub;
		//頂点データ
		VertexData					vertex;
	};

public:
	// コンストラクタ
	Texture(std::weak_ptr<Device>dev);
	// デストラクタ
	~Texture();

	// ユニコード変換
	std::wstring ChangeUnicode(const CHAR * str);

	// 読み込み
	HRESULT LoadBMP(USHORT* index, std::string fileName);
	// WIC読み込み
	HRESULT LoadWIC(USHORT* index, std::wstring fileName);

	// ディスクリプターのセット
	void SetDescriptorBMP(USHORT* index);
	// ディスクリプターのセット
	void SetDescriptorWIC(USHORT* index);

	// 描画準備
	void SetDrawBMP(USHORT* index);
	// 描画準備
	void SetDrawWIC(USHORT* index);
	
	// 描画
	void DrawBMP(USHORT* index, Vector2<FLOAT>pos, Vector2<FLOAT>size);
	// 描画
	void DrawWIC(USHORT* index, Vector2<FLOAT>pos, Vector2<FLOAT>size);

	// 分割描画
	void DrawRect(USHORT* index, Vector2<FLOAT>pos, Vector2<FLOAT>size, Vector2<FLOAT>rect, Vector2<FLOAT>rSize, bool turn = false);
	// 分割描画
	void DrawRectWIC(USHORT* index, Vector2<FLOAT>pos, Vector2<FLOAT>size, Vector2<FLOAT>rect, Vector2<FLOAT>rSize, bool turn = false);

private:
	// 定数バッファ用のヒープの生成	
	HRESULT CreateConstantHeap(USHORT* index, std::string fileName);
	// 定数バッファの生成
	HRESULT CreateConstant(USHORT* index, std::string fileName);
	// シェーダリソースビューの生成
	HRESULT CreateShaderResourceView(USHORT* index, std::string fileName);


	// 定数バッファ用ヒープの生成
	HRESULT CreateConstantHeapWIC(USHORT* index);
	// シェーダリソースビューの生成
	HRESULT CreateShaderResourceViewWIC(USHORT* index);


	// 頂点リソースの生成
	HRESULT CreateVertex(USHORT* index);

	
	// 頂点リソースの生成
	HRESULT CreateVertexWIC(USHORT* index);


	// デバイス
	std::weak_ptr<Device>dev;

	// 参照結果
	HRESULT result;

	// BMPデータの起源
	std::map<std::string, BMP>origin;

	// BMPデータ
	std::map<USHORT*, BMP>bmp;

	// WICデータ
	std::map<USHORT*, WIC>wic;
};

