#pragma once
#include "Texture.h"

class PMD
{
	// 頂点バッファ
	struct VertexBuffer
	{
		//リソース
		ID3D12Resource*				resource;
		//頂点バッファビュー
		D3D12_VERTEX_BUFFER_VIEW	view;
		//送信データ
		UINT*						data;
	};

	// 頂点インデックス
	struct VertexIndex
	{
		//リソース
		ID3D12Resource*			resource;
		//送信データ
		UINT*					data;
		//頂点インデックスビュー
		D3D12_INDEX_BUFFER_VIEW	view;
	};

	// 定数バッファ
	struct Constant
	{
		//ヒープ
		ID3D12DescriptorHeap*	heap;
		//リソース
		ID3D12Resource*			resource;
		//送信データ
		UINT8*					data;
		//サイズ
		UINT					size;
	};

	// マテリアル
	struct Mat
	{
		//基本色
		DirectX::XMFLOAT3 diffuse;
		//テクスチャフラグ
		BOOL texFlag;
	};

#pragma pack(1)
	// ヘッダー
	struct Header
	{
		//タイプ
		UCHAR	type[3];
		//バージョン
		FLOAT	ver;
		//名前
		UCHAR	name[20];
		//コメント
		UCHAR	comment[256];
		//頂点数
		UINT	vertexNum;
	};
#pragma pack()

	// 頂点
	struct VertexData
	{
		//座標
		DirectX::XMFLOAT3	pos;
		//法線
		DirectX::XMFLOAT3	normal;
		//uv
		DirectX::XMFLOAT2	uv;
		//ボーン番号
		USHORT				bornNum[2];
		//ウェイト
		UCHAR				bornWeight;
		//輪郭線フラグ
		UCHAR				edge;
	};

#pragma pack(1)
	// マテリアル
	struct MaterialData
	{
		//基本色
		DirectX::XMFLOAT3	diffuse;
		//透明度
		FLOAT				alpha;
		//反射強度
		FLOAT				specularity;
		//反射色
		DirectX::XMFLOAT3	specula;
		//環境色
		DirectX::XMFLOAT3	mirror;
		//トゥーン番号
		UCHAR				toonIndex;
		//輪郭線フラグ
		UCHAR				edge;
		//インデックス数
		UINT				indexNum;
		//テクスチャパス
		CHAR				textureFilePath[20];
	};
#pragma pack()

	// ボーン
	struct BornData
	{
		//名前
		CHAR				name[20];
		//親ボーン番号
		WORD				parent_born_index;
		//子ボーン番号
		WORD				child_born_index;
		//タイプ
		BYTE				type;
		//IK親ボーン番号
		WORD				ik_parent_born_index;
		//座標
		DirectX::XMFLOAT3	pos;
	};

	// ボーン座標
	struct Pos
	{
		//先頭座標
		DirectX::XMFLOAT3	head;
		//末尾座標
		DirectX::XMFLOAT3	tail;
	};

	// ボーンノード
	struct Node
	{
		std::vector<USHORT>	index;
	};

	// PMD
	struct Pmd
	{
		Header							header;
		std::vector<VertexData>			vertex;
		std::vector<USHORT>				index;
		std::vector<MaterialData>		material;
		std::vector<BornData>			born;
		std::vector<Pos>				pos;
		std::vector<Node>				node;
		std::map<std::string, UINT>		bornName;
		std::vector<DirectX::XMMATRIX>	bornMatrix;

		VertexBuffer					vcon;
		VertexIndex						icon;
		Constant						con;
		std::map<USHORT, USHORT>		id;
	};

public:
	// コンストラクタ
	PMD(std::weak_ptr<Device>dev, std::weak_ptr<Texture>tex);
	// デストラクタ
	~PMD();

	// 文字列の検索
	std::string FindString(const std::string path, const CHAR find, INT offset = 1, bool start = false);

	// フォルダーとの連結
	std::string FolderPath(std::string path, const char* textureName);

	// 読み込み
	HRESULT Load(std::string fileName);

	// テクスチャの読み込み
	HRESULT LoadTexture(std::string fileName);

	// 描画
	void Draw(void);

private:
	// 頂点バッファの生成
	HRESULT CreateVertexBuffer(void);
	// 頂点インデックスの生成
	HRESULT CreateVertexIndex(void);


	// 定数バッファ用ヒープの生成
	HRESULT CreateConstantHeap(void);
	// 定数バッファ用リソースの生成
	HRESULT CreateConstant(void);
	// 定数バッファビューの生成
	HRESULT CreateConstantView(void);


	// デバイス
	std::weak_ptr<Device>dev;

	// テクスチャ
	std::weak_ptr<Texture>tex;

	// 参照結果
	HRESULT result;

	// PMD
	Pmd pmd;

	// マテリアル
	Mat mat;
};