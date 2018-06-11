#include "PMD.h"
#include "d3dx12.h"
#include <sstream>
#include <tchar.h>

// コンストラクタ
PMD::PMD(std::weak_ptr<Device>dev, std::weak_ptr<Texture>tex) : dev(dev), tex(tex)
{
	//参照結果
	result = S_OK;

	//PMD
	pmd = {};

	//マテリアル
	mat = {};
}

// デストラクタ
PMD::~PMD()
{
	pmd.con.resource->Unmap(0, nullptr);

	RELEASE(pmd.icon.resource);
	RELEASE(pmd.vcon.resource);
	RELEASE(pmd.con.resource);
	RELEASE(pmd.con.heap);
}

// 文字列の検索
std::string PMD::FindString(const std::string path, const CHAR find, INT offset, bool start)
{
	std::string tmp;

	if (start == false)
	{
		auto pos = path.find_last_of(find) + offset;

		tmp = path.substr(0, pos);
	}
	else
	{
		auto pos = path.find_first_of(find) + offset;

		tmp = path.substr(pos, path.size());
	}

	return tmp;
}

// フォルダーとの連結
std::string PMD::FolderPath(std::string path, const char* textureName)
{
	//ダミー宣言
	int pathIndex1 = path.rfind('/');
	int pathIndex2 = path.rfind('\\');
	int pathIndex = max(pathIndex1, pathIndex2);

	std::string folderPath = path.substr(0, pathIndex);
	folderPath += "/";
	folderPath += textureName;

	return folderPath;
}

// 読み込み
HRESULT PMD::Load(std::string fileName)
{
	//ファイル
	FILE *file;

	if ((fopen_s(&file, fileName.c_str(), "rb")) != 0)
	{
		//エラーナンバー確認
		auto a = (fopen_s(&file, fileName.c_str(), "rb"));
		std::stringstream s;
		s << a;
		OutputDebugString(_T("\nファイルを開けませんでした：失敗\n"));
		OutputDebugStringA(s.str().c_str());
		return S_FALSE;
	}

	//ヘッダーの読み込み
	{
		fread(&pmd.header, sizeof(pmd.header), 1, file);
	}

	//頂点データの読み込み
	{
		//頂点データ配列のメモリサイズ確保
		pmd.vertex.resize(pmd.header.vertexNum);

		for (auto& v : pmd.vertex)
		{
			fread(&v.pos,        sizeof(v.pos),        1, file);
			fread(&v.normal,     sizeof(v.normal),     1, file);
			fread(&v.uv,         sizeof(v.uv),         1, file);
			fread(&v.bornNum,    sizeof(v.bornNum),    1, file);
			fread(&v.bornWeight, sizeof(v.bornWeight), 1, file);
			fread(&v.edge,       sizeof(v.edge),       1, file);
		}
	}

	//インデックスデータの読み込み
	{
		//インデックス数格納用
		UINT indexNum = 0;
		//インデックス数の読み込み
		fread(&indexNum, sizeof(UINT), 1, file);

		//インデックスデータ配列のメモリサイズ確保
		pmd.index.resize(indexNum);

		for (auto& i : pmd.index)
		{
			fread(&i, sizeof(USHORT), 1, file);
		}
	}
	
	//マテリアルデータの読み込み
	{
		//マテリアル数格納用
		UINT materialNum = 0;
		//マテリアル数の読み込み
		fread(&materialNum, sizeof(UINT), 1, file);

		//マテリアルデータ配列のメモリサイズ確保
		pmd.material.resize(materialNum);

		fread(&pmd.material[0], sizeof(MaterialData), materialNum, file);
	}

	//ボーンデータの読み込み
	{
		//ボーン数格納用
		UINT bornNum = 0;
		//ボーン数の読み込み
		fread(&bornNum, sizeof(USHORT), 1, file);

		//ボーンデータ配列のメモリサイズ確保
		pmd.born.resize(bornNum);

		for (auto& b : pmd.born)
		{
			fread(&b.name,                 sizeof(b.name),				   1, file);
			fread(&b.parent_born_index,    sizeof(b.parent_born_index),	   1, file);
			fread(&b.child_born_index,     sizeof(b.child_born_index),	   1, file);
			fread(&b.type,                 sizeof(b.type),				   1, file);
			fread(&b.ik_parent_born_index, sizeof(b.ik_parent_born_index), 1, file);
			fread(&b.pos,				   sizeof(b.pos),				   1, file);
		}
	}

	//ファイルを閉じる
	fclose(file);

	result = LoadTexture(FindString(fileName, '/'));
	result = CreateVertexIndex();
	result = CreateConstantView();

	return result;
}

// テクスチャの読み込み
HRESULT PMD::LoadTexture(std::string fileName)
{
	for (UINT i = 0; i < pmd.material.size(); ++i)
	{
		if (pmd.material[i].textureFilePath[0] != '\0')
		{
			tex.lock()->LoadWIC(&pmd.id[i], tex.lock()->ChangeUnicode(FolderPath(fileName, pmd.material[i].textureFilePath).c_str()));
			if (FAILED(result))
			{
				OutputDebugString(_T("\nPMDのテクスチャ読み込み：失敗\n"));
				return result;
			}
		}
	}
	return result;
}

// 頂点バッファの生成
HRESULT PMD::CreateVertexBuffer(void)
{
	//ヒープ設定用構造体の設定
	D3D12_HEAP_PROPERTIES prop = {};
	prop.Type					= D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_UPLOAD;
	prop.CPUPageProperty		= D3D12_CPU_PAGE_PROPERTY::D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	prop.MemoryPoolPreference	= D3D12_MEMORY_POOL::D3D12_MEMORY_POOL_UNKNOWN;
	prop.CreationNodeMask		= 1;
	prop.VisibleNodeMask		= 1;

	//リソース設定用構造体の設定
	D3D12_RESOURCE_DESC desc = {};
	desc.Dimension				= D3D12_RESOURCE_DIMENSION::D3D12_RESOURCE_DIMENSION_BUFFER;
	desc.Width					= (sizeof(VertexData) * pmd.vertex.size());
	desc.Height					= 1;
	desc.DepthOrArraySize		= 1;
	desc.MipLevels				= 1;
	desc.Format					= DXGI_FORMAT::DXGI_FORMAT_UNKNOWN;
	desc.SampleDesc.Count		= 1;
	desc.Flags					= D3D12_RESOURCE_FLAGS::D3D12_RESOURCE_FLAG_NONE;
	desc.Layout					= D3D12_TEXTURE_LAYOUT::D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

	//リソース生成
	{
		result = dev.lock()->GetDevice()->CreateCommittedResource(&prop, D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE, &desc, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&pmd.vcon.resource));
		if (FAILED(result))
		{
			OutputDebugString(_T("\nPMDの頂点バッファ用リソースの生成：失敗\n"));
			return result;
		}
	}

	//送信範囲
	D3D12_RANGE range = { 0,0 };

	//マッピング
	{
		result = pmd.vcon.resource->Map(0, &range, reinterpret_cast<void**>(&pmd.vcon.data));
		if (FAILED(result))
		{
			OutputDebugString(_T("\nPMDの頂点バッファ用リソースのマッピング：失敗\n"));
			return result;
		}

		//頂点データのコピー
		memcpy(pmd.vcon.data, &pmd.vertex[0], (sizeof(VertexData) * pmd.vertex.size()));
	}

	//アンマッピング
	pmd.vcon.resource->Unmap(0, nullptr);

	//頂点バッファ設定用構造体の設定
	{
		pmd.vcon.view.BufferLocation	= pmd.vcon.resource->GetGPUVirtualAddress();
		pmd.vcon.view.SizeInBytes		= sizeof(VertexData) * pmd.vertex.size();
		pmd.vcon.view.StrideInBytes		= sizeof(VertexData);
	}

	return result;
}

// 頂点インデックスの生成
HRESULT PMD::CreateVertexIndex(void)
{
	result = CreateVertexBuffer();
	if (FAILED(result))
	{
		return result;
	}

	//ヒープ設定用構造体の設定
	D3D12_HEAP_PROPERTIES prop = {};
	prop.Type					= D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_UPLOAD;
	prop.CPUPageProperty		= D3D12_CPU_PAGE_PROPERTY::D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	prop.MemoryPoolPreference	= D3D12_MEMORY_POOL::D3D12_MEMORY_POOL_UNKNOWN;
	prop.CreationNodeMask		= 1;
	prop.VisibleNodeMask		= 1;

	//リソース設定用構造体の設定
	D3D12_RESOURCE_DESC desc = {};
	desc.Dimension				= D3D12_RESOURCE_DIMENSION::D3D12_RESOURCE_DIMENSION_BUFFER;
	desc.Width					= (sizeof(USHORT) * pmd.index.size());
	desc.Height					= 1;
	desc.DepthOrArraySize		= 1;
	desc.MipLevels				= 1;
	desc.Format					= DXGI_FORMAT::DXGI_FORMAT_UNKNOWN;
	desc.SampleDesc.Count		= 1;
	desc.Flags					= D3D12_RESOURCE_FLAGS::D3D12_RESOURCE_FLAG_NONE;
	desc.Layout					= D3D12_TEXTURE_LAYOUT::D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

	//リソース生成
	{
		result = dev.lock()->GetDevice()->CreateCommittedResource(&prop, D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE, &desc, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&pmd.icon.resource));
		if (FAILED(result))
		{
			OutputDebugString(_T("\nPMDの頂点インデックス用リソースの生成：失敗\n"));
			return result;
		}
	}

	//送信範囲
	D3D12_RANGE range = { 0,0 };

	//マッピング
	{
		result = pmd.icon.resource->Map(0, &range, reinterpret_cast<void**>(&pmd.icon.data));
		if (FAILED(result))
		{
			OutputDebugString(_T("\nPMDの頂点インデックスのマッピング：失敗\n"));
			return result;
		}
	}

	//コピー
	memcpy(pmd.icon.data, &pmd.index[0], sizeof(USHORT) * pmd.index.size());

	//アンマッピング
	pmd.icon.resource->Unmap(0, nullptr);

	//頂点インデックスビュー設定用構造体の設定
	{
		pmd.icon.view.BufferLocation	= pmd.icon.resource->GetGPUVirtualAddress();
		pmd.icon.view.SizeInBytes		= sizeof(USHORT) * pmd.index.size();
		pmd.icon.view.Format			= DXGI_FORMAT::DXGI_FORMAT_R16_UINT;
	}

	return result;
}

// 定数バッファ用ヒープの生成
HRESULT PMD::CreateConstantHeap(void)
{
	//定数バッファ設定用構造体の設定
	D3D12_DESCRIPTOR_HEAP_DESC desc = {};
	desc.NumDescriptors		= 2 + pmd.material.size();
	desc.Flags				= D3D12_DESCRIPTOR_HEAP_FLAGS::D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	desc.Type				= D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;

	//ヒープ生成
	{
		result = dev.lock()->GetDevice()->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&pmd.con.heap));
		if (FAILED(result))
		{
			OutputDebugString(_T("\nPMDの定数バッファ用ヒープの生成：失敗\n"));
			return S_FALSE;
		}

		//ヒープサイズを取得
		pmd.con.size = dev.lock()->GetDevice()->GetDescriptorHandleIncrementSize(desc.Type);
	}

	return result;
}

// 定数バッファ用リソースの生成
HRESULT PMD::CreateConstant(void)
{
	result = CreateConstantHeap();
	if (FAILED(result))
	{
		return result;
	}

	//ヒープ設定用構造体の設定
	D3D12_HEAP_PROPERTIES prop = {};
	prop.Type					= D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_UPLOAD;
	prop.CPUPageProperty		= D3D12_CPU_PAGE_PROPERTY::D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	prop.MemoryPoolPreference	= D3D12_MEMORY_POOL::D3D12_MEMORY_POOL_UNKNOWN;
	prop.CreationNodeMask		= 1;
	prop.VisibleNodeMask		= 1;

	//リソース設定用構造体の設定
	D3D12_RESOURCE_DESC desc = {};
	desc.Dimension				= D3D12_RESOURCE_DIMENSION::D3D12_RESOURCE_DIMENSION_BUFFER;
	desc.Width					= ((sizeof(Mat) + 0xff) &~0xff) * ((sizeof(Mat) + 0xff) &~0xff);
	desc.Height					= 1;
	desc.DepthOrArraySize		= 1;
	desc.MipLevels				= 1;
	desc.Format					= DXGI_FORMAT::DXGI_FORMAT_UNKNOWN;
	desc.SampleDesc.Count		= 1;
	desc.Flags					= D3D12_RESOURCE_FLAGS::D3D12_RESOURCE_FLAG_NONE;
	desc.Layout					= D3D12_TEXTURE_LAYOUT::D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

	//リソース生成
	{
		result = dev.lock()->GetDevice()->CreateCommittedResource(&prop, D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE, &desc, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&pmd.con.resource));
		if (FAILED(result))
		{
			OutputDebugString(_T("\nPMDの定数バッファ用リソースの生成：失敗\n"));
			return result;
		}
	}

	return result;
}

// 定数バッファビューの生成
HRESULT PMD::CreateConstantView(void)
{
	result = CreateConstant();
	if (FAILED(result))
	{
		return result;
	}

	//定数バッファ７ビュー設定用構造体の設定
	D3D12_CONSTANT_BUFFER_VIEW_DESC desc = {};
	desc.SizeInBytes = (sizeof(Mat) + 0xff) &~0xff;

	//GPUアドレス
	D3D12_GPU_VIRTUAL_ADDRESS address = pmd.con.resource->GetGPUVirtualAddress();
	//ディスクリプターのCPUハンドル
	D3D12_CPU_DESCRIPTOR_HANDLE handle = pmd.con.heap->GetCPUDescriptorHandleForHeapStart();

	for (UINT i = 0; i < pmd.material.size(); ++i)
	{
		desc.BufferLocation = address;

		//定数バッファビュー生成
		dev.lock()->GetDevice()->CreateConstantBufferView(&desc, handle);

		address += desc.SizeInBytes;

		handle.ptr += pmd.con.size;
	}

	//送信範囲
	D3D12_RANGE range = { 0, 0 };

	//マッピング
	{
		result = pmd.con.resource->Map(0, &range, (void**)(&pmd.con.data));
		if (FAILED(result))
		{
			OutputDebugString(_T("\nPMDの定数バッファ用リソースのマッピング：失敗\n"));
			return result;
		}

		//コピー
		memcpy(pmd.con.data, &mat, sizeof(DirectX::XMMATRIX));
	}

	return result;
}

// 描画
void PMD::Draw(void)
{
	//セット
	{
		//頂点バッファビューのセット
		dev.lock()->GetComList()->IASetVertexBuffers(0, 1, &pmd.vcon.view);

		//頂点インデックスビューのセット
		dev.lock()->GetComList()->IASetIndexBuffer(&pmd.icon.view);

		dev.lock()->GetComList()->IASetPrimitiveTopology(D3D12_PRIMITIVE_TOPOLOGY::D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	}

	//オフセット
	UINT offset = 0;

	//送信用データ
	UINT8* data = pmd.con.data;

	//ヒープの先頭ハンドル
	D3D12_GPU_DESCRIPTOR_HANDLE handle = pmd.con.heap->GetGPUDescriptorHandleForHeapStart();

	for (UINT i = 0; i < pmd.material.size(); ++i)
	{
		mat.diffuse = pmd.material[i].diffuse;

		mat.texFlag = (pmd.material[i].textureFilePath[0] != '\0');
		if (mat.texFlag == TRUE)
		{
			tex.lock()->SetDrawWIC(&pmd.id[i]);
		}
		else
		{
			for (auto itr = pmd.id.begin(); itr != pmd.id.end(); ++itr)
			{
				tex.lock()->SetDescriptorWIC(&itr->second);
				break;
			}
		}

		//定数ヒープのセット
		dev.lock()->GetComList()->SetDescriptorHeaps(1, &pmd.con.heap);

		//ディスクリプターテーブルのセット
		dev.lock()->GetComList()->SetGraphicsRootDescriptorTable(2, handle);

		//コピー
		memcpy(data, &mat, sizeof(Mat));

		//描画
		dev.lock()->GetComList()->DrawIndexedInstanced(pmd.material[i].indexNum, 1, offset, 0, 0);

		//ハンドル更新
		handle.ptr += pmd.con.size;

		//データ更新
		data = (UINT8*)(((sizeof(Mat) + 0xff) &~0xff) + (CHAR*)(data));

		//オフセット更新
		offset +=pmd.material[i].indexNum;
	}
}