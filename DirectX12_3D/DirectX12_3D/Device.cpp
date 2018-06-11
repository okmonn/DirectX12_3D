#include "Device.h"
#include "Window.h"
#include "d3dx12.h"
#include <d3dcompiler.h>
#include <tchar.h>

#pragma comment (lib,"d3d12.lib")
#pragma comment (lib,"dxgi.lib")
#pragma comment (lib,"d3dcompiler.lib")

//クリアカラーの指定
const FLOAT clearColor[] = { 1.0f,0.0f,0.0f,0.0f };

// 機能レベル一覧
D3D_FEATURE_LEVEL levels[] = 
{
	D3D_FEATURE_LEVEL_12_1,
	D3D_FEATURE_LEVEL_12_0,
	D3D_FEATURE_LEVEL_11_1,
	D3D_FEATURE_LEVEL_11_0,
};

// コンストラクタ
Device::Device(std::weak_ptr<Window> win) : win(win)
{
	//参照結果
	result = S_OK;

	//機能レベル
	level = D3D_FEATURE_LEVEL_12_1;

	//回転角度
	angle = 0.0f;

	//デバイス
	dev = nullptr;
	
	//コマンド
	com = {};

	//スワップチェイン
	swap = {};

	//レンダーターゲット
	render = {};

	//深度ステンシル
	depth = {};

	//フェンス
	fen = {};

	//ルートシグネチャ
	sig = {};

	//パイプライン
	pipe = {};

	//定数バッファ
	con = {};

	//WVP
	wvp = {};

	//ビューポート
	viewPort = {};

	//シザー
	scissor = {};

	// バリア
	barrier = {};


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
	Init();
}

// デストラクタ
Device::~Device()
{
	//定数バッファのアンマップ
	con.resource->Unmap(0, nullptr);

	RELEASE(con.resource);
	RELEASE(con.heap);
	RELEASE(pipe.vertex);
	RELEASE(pipe.pixel);
	RELEASE(pipe.pipeline);
	RELEASE(sig.signature);
	RELEASE(sig.error);
	RELEASE(sig.rootSignature);
	RELEASE(fen.fence);
	RELEASE(depth.resource);
	RELEASE(depth.heap);
	for (UINT i = 0; i < render.resource.size(); ++i)
	{
		RELEASE(render.resource[i]);
	}
	RELEASE(render.heap);
	RELEASE(swap.factory);
	RELEASE(swap.swapChain);
	RELEASE(com.queue);
	RELEASE(com.list);
	RELEASE(com.allocator);
	RELEASE(dev);
}

// 初期化
void Device::Init(void)
{
	SetWorldViewProjection();

	CreateCommand();

	CreateSwapChain();

	CreateRenderTarget();

	CreateDepthView();

	CreateFence();

	CreateRootSigunature();

	ShaderCompile(_T("Shader.hlsl"));

	CreatePipeline();

	CreateConstantView();

	SetViewPort();

	SetScissor();
}

// 描画セット
void Device::Set(void)
{
	/*if (in.lock()->InputKey(DIK_RIGHT) == TRUE)
	{
	//回転
	angle++;
	//行列更新
	wvp.world = DirectX::XMMatrixRotationY(RAD(angle));
	}
	else if (in.lock()->InputKey(DIK_LEFT) == TRUE)
	{
	//回転
	angle--;
	//行列更新
	wvp.world = DirectX::XMMatrixRotationY(RAD(angle));
	}*/
	//回転
	{
		angle++;
		wvp.world = DirectX::XMMatrixRotationY(RAD(angle));

		//行列データ更新
		memcpy(con.data, &wvp, sizeof(WVP));
	}

	//リセット
	{
		//コマンドアロケータのリセット
		com.allocator->Reset();
		//リストのリセット
		com.list->Reset(com.allocator, pipe.pipeline);
	}

	//ルートシグネチャのセット
	{
		com.list->SetGraphicsRootSignature(sig.rootSignature);
	}

	//パイプラインのセット
	{
		com.list->SetPipelineState(pipe.pipeline);
	}

	//定数バッファ
	{
		//定数バッファヒープの先頭ハンドルを取得
		D3D12_GPU_DESCRIPTOR_HANDLE handle = con.heap->GetGPUDescriptorHandleForHeapStart();

		//定数バッファヒープのセット
		com.list->SetDescriptorHeaps(1, &con.heap);

		//定数バッファディスクラプターテーブルのセット
		com.list->SetGraphicsRootDescriptorTable(0, handle);
	}

	//ビューのセット
	{
		com.list->RSSetViewports(1, &viewPort);
	}

	//シザーのセット
	{
		com.list->RSSetScissorRects(1, &scissor);
	}

	//Present ---> RenderTarget
	{
		Barrier(D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_RENDER_TARGET);
	}

	//レンダーターゲット
	{
		//頂点ヒープの先頭ハンドルの取得
		CD3DX12_CPU_DESCRIPTOR_HANDLE handle(render.heap->GetCPUDescriptorHandleForHeapStart(), swap.swapChain->GetCurrentBackBufferIndex(), render.size);

		//レンダーターゲットのセット
		com.list->OMSetRenderTargets(1, &handle, false, &depth.heap->GetCPUDescriptorHandleForHeapStart());

		//レンダーターゲットのクリア
		com.list->ClearRenderTargetView(handle, clearColor, 0, nullptr);
	}

	//深度ステンシル
	{
		//深度ステンシルヒープの先頭ハンドルの取得
		D3D12_CPU_DESCRIPTOR_HANDLE handle = depth.heap->GetCPUDescriptorHandleForHeapStart();

		//深度ステンシルビューのクリア
		com.list->ClearDepthStencilView(handle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
	}
}

// 実行
void Device::Do(void)
{
	// RenderTarget ---> Present
	{
		Barrier(D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PRESENT);
	}

	//コマンドリストの記録終了
	{
		com.list->Close();
	}

	//コマンドリストの実行
	{
		//リストの配列
		ID3D12CommandList *commandList[] = { com.list };
		//配列でない場合：queue->ExecuteCommandLists(1, (ID3D12CommandList*const*)&list);
		com.queue->ExecuteCommandLists(_countof(commandList), commandList);
	}

	//裏、表画面を反転
	{
		swap.swapChain->Present(1, 0);
	}

	Wait();
}

// デバイスの取得
ID3D12Device * Device::GetDevice(void)
{
	return dev;
}

// コマンドリストの取得
ID3D12GraphicsCommandList * Device::GetComList(void)
{
	return com.list;
}

//ワールドビュープロジェクションのセット
void Device::SetWorldViewProjection(void)
{
	//ダミー宣言
	FLOAT pos = 10.0f;
	DirectX::XMMATRIX view   = DirectX::XMMatrixIdentity();
	//カメラの位置
	DirectX::XMVECTOR eye    = { 0, pos,  -15.0f };
	//カメラの焦点
	DirectX::XMVECTOR target = { 0, pos,   0 };
	//カメラの上方向
	DirectX::XMVECTOR upper  = { 0, 1,     0 };

	view = DirectX::XMMatrixLookAtLH(eye, target, upper);

	//ダミー宣言
	DirectX::XMMATRIX projection = DirectX::XMMatrixIdentity();

	projection = DirectX::XMMatrixPerspectiveFovLH(RAD(90), ((static_cast<FLOAT>(WINDOW_X) / static_cast<FLOAT>(WINDOW_Y))), 0.5f, 500.0f);

	//更新
	wvp.world = DirectX::XMMatrixIdentity();
	wvp.viewProjection = view * projection;
}

// デバイスの生成
HRESULT Device::CreateDevice(void)
{
	for (auto& i : levels)
	{
		//デバイス生成
		result = D3D12CreateDevice(nullptr, i, IID_PPV_ARGS(&dev));
		if (result == S_OK)
		{
			level = i;
			break;
		}
	}

	return result;
}

// コマンド周りの生成
HRESULT Device::CreateCommand(void)
{
	result = CreateDevice();
	if (FAILED(result))
	{
		OutputDebugString(_T("\nデバイスの生成：失敗\n"));
		return result;
	}

	//コマンドアロケータの生成
	{
		result = dev->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE::D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&com.allocator));
		if (FAILED(result))
		{
			OutputDebugString(_T("\nコマンドアロケータの生成：失敗\n"));
			return result;
		}
	}

	//コマンドリストの生成
	{
		result = dev->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE::D3D12_COMMAND_LIST_TYPE_DIRECT, com.allocator, nullptr, IID_PPV_ARGS(&com.list));
		if (FAILED(result))
		{
			OutputDebugString(_T("\nコマンドリストの生成：失敗\n"));
			return result;
		}

		//いったん閉じる
		com.list->Close();
	}

	//コマンドキューの生成
	{
		//コマンドキュー設定用構造体
		D3D12_COMMAND_QUEUE_DESC desc = {};
		desc.Flags		= D3D12_COMMAND_QUEUE_FLAGS::D3D12_COMMAND_QUEUE_FLAG_NONE;
		desc.NodeMask	= 0;
		desc.Priority	= D3D12_COMMAND_QUEUE_PRIORITY::D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
		desc.Type		= D3D12_COMMAND_LIST_TYPE::D3D12_COMMAND_LIST_TYPE_DIRECT;

		result = dev->CreateCommandQueue(&desc, IID_PPV_ARGS(&com.queue));
		if (FAILED(result))
		{
			OutputDebugString(_T("\nコマンドキューの生成：失敗"));
			return result;
		}
	}

	return result;
}

// ファクトリーの生成
HRESULT Device::CreateFactory(void)
{
	//ファクトリー生成
	result = CreateDXGIFactory1(IID_PPV_ARGS(&swap.factory));

	return result;
}

// スワップチェインの生成
HRESULT Device::CreateSwapChain(void)
{
	result = CreateFactory();
	if (FAILED(result))
	{
		OutputDebugString(_T("\nファクトリーの生成：失敗\n"));
		return result;
	}

	//スワップチェイン設定用構造体
	DXGI_SWAP_CHAIN_DESC1 desc = {};
	desc.AlphaMode		= DXGI_ALPHA_MODE::DXGI_ALPHA_MODE_UNSPECIFIED;
	desc.BufferCount	= 2;
	desc.BufferUsage	= DXGI_USAGE_RENDER_TARGET_OUTPUT;
	desc.Flags			= 0;
	desc.Format			= DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM;
	desc.Height			= WINDOW_Y;
	desc.SampleDesc		= { 1, 0 };
	desc.Scaling		= DXGI_SCALING::DXGI_SCALING_STRETCH;
	desc.Stereo			= false;
	desc.SwapEffect		= DXGI_SWAP_EFFECT::DXGI_SWAP_EFFECT_FLIP_DISCARD;
	desc.Width			= WINDOW_X;

	//スワップチェイン生成
	result = swap.factory->CreateSwapChainForHwnd(com.queue, win.lock()->GetWindowHandle(), &desc, nullptr, nullptr, (IDXGISwapChain1**)(&swap.swapChain));
	if (FAILED(result))
	{
		OutputDebugString(_T("\nスワップチェインの生成：失敗\n"));
		return result;
	}

	//バックバッファ数保存
	swap.bufferCnt = desc.BufferCount;

	return result;
}

// レンダーターゲット用ヒープの生成
HRESULT Device::CreateRenderHeap(void)
{
	//ヒープ設定用構造体
	D3D12_DESCRIPTOR_HEAP_DESC desc = {};
	desc.Flags				= D3D12_DESCRIPTOR_HEAP_FLAGS::D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	desc.NodeMask			= 0;
	desc.NumDescriptors		= swap.bufferCnt;
	desc.Type				= D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_RTV;

	//ヒープ生成
	result = dev->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&render.heap));
	if (FAILED(result))
	{
		OutputDebugString(_T("\nレンダーターゲット用ヒープの生成：失敗\n"));
		return result;
	}

	//ヒープサイズ設定
	render.size = dev->GetDescriptorHandleIncrementSize(desc.Type);

	return result;
}

// レンダーターゲットの生成
HRESULT Device::CreateRenderTarget(void)
{
	result = CreateRenderHeap();
	if (FAILED(result))
	{
		return result;
	}

	//レンダーターゲット設定用構造体
	D3D12_RENDER_TARGET_VIEW_DESC desc = {};
	desc.Format					= DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM;
	desc.ViewDimension			= D3D12_RTV_DIMENSION::D3D12_RTV_DIMENSION_TEXTURE2D;
	desc.Texture2D.MipSlice		= 0;
	desc.Texture2D.PlaneSlice	= 0;

	//配列のメモリ確保
	render.resource.resize(swap.bufferCnt);

	//先頭ハンドル取得
	D3D12_CPU_DESCRIPTOR_HANDLE handle = render.heap->GetCPUDescriptorHandleForHeapStart();

	for (UINT i = 0; i < render.resource.size(); i++)
	{
		//バッファの取得
		result = swap.swapChain->GetBuffer(i, IID_PPV_ARGS(&render.resource[i]));
		if (FAILED(result))
		{
			OutputDebugString(_T("\nレンダーターゲットの生成：失敗\n"));
			return result;
		}

		//レンダーターゲット生成
		dev->CreateRenderTargetView(render.resource[i], &desc, handle);

		//ハンドルの位置を移動
		handle.ptr += render.size;
	}

	return result;
}

// 深度ステンシル用ヒープの生成
HRESULT Device::CreateDepthHeap(void)
{
	//ヒープ設定用構造体
	D3D12_DESCRIPTOR_HEAP_DESC desc = {};
	desc.Flags				= D3D12_DESCRIPTOR_HEAP_FLAGS::D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	desc.NodeMask			= 0;
	desc.NumDescriptors		= swap.bufferCnt;
	desc.Type				= D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_DSV;

	//深度ステンシル用ヒープ生成
	result = dev->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&depth.heap));
	if (FAILED(result))
	{
		OutputDebugString(_T("\n深度ステンシル用ヒープの生成：失敗\n"));
		return result;
	}

	//ヒープサイズ設定
	depth.size = dev->GetDescriptorHandleIncrementSize(desc.Type);

	return result;
}

// 深度ステンシルの生成
HRESULT Device::CreateDepthStencil(void)
{
	result = CreateDepthHeap();
	if (FAILED(result))
	{
		return result;
	}

	//ヒーププロパティ設定用構造体の設定
	D3D12_HEAP_PROPERTIES prop = {};
	prop.CPUPageProperty		= D3D12_CPU_PAGE_PROPERTY::D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	prop.CreationNodeMask		= 1;
	prop.MemoryPoolPreference	= D3D12_MEMORY_POOL::D3D12_MEMORY_POOL_UNKNOWN;
	prop.Type					= D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_DEFAULT;
	prop.VisibleNodeMask		= 1;

	//リソース設定用構造体の設定
	D3D12_RESOURCE_DESC desc = {};
	desc.Dimension				= D3D12_RESOURCE_DIMENSION::D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	desc.Alignment				= 0;
	desc.Width					= WINDOW_X;
	desc.Height					= WINDOW_Y;
	desc.DepthOrArraySize		= 1;
	desc.MipLevels				= 0;
	desc.Format					= DXGI_FORMAT::DXGI_FORMAT_D32_FLOAT;
	desc.SampleDesc.Count		= 1;
	desc.SampleDesc.Quality		= 0;
	desc.Flags					= D3D12_RESOURCE_FLAGS::D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
	desc.Layout					= D3D12_TEXTURE_LAYOUT::D3D12_TEXTURE_LAYOUT_UNKNOWN;

	//クリア値設定用構造体の設定
	D3D12_CLEAR_VALUE clear = {};
	clear.Format				= DXGI_FORMAT::DXGI_FORMAT_D32_FLOAT;
	clear.DepthStencil.Depth	= 1.0f;
	clear.DepthStencil.Stencil	= 0;

	//深度ステンシル用リソース生成
	result = dev->CreateCommittedResource(&prop, D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE, &desc, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_DEPTH_WRITE, &clear, IID_PPV_ARGS(&depth.resource));
	if (FAILED(result))
	{
		OutputDebugString(_T("\n深度ステンシル用リソースの生成：失敗\n"));
		return result;
	}

	return result;
}

// 深度ステンシルビューの生成
HRESULT Device::CreateDepthView(void)
{
	result = CreateDepthStencil();
	if (FAILED(result))
	{
		return result;
	}

	//深度ステンシルビュー設定用構造体の設定
	D3D12_DEPTH_STENCIL_VIEW_DESC desc = {};
	desc.Format			= DXGI_FORMAT::DXGI_FORMAT_D32_FLOAT;
	desc.ViewDimension	= D3D12_DSV_DIMENSION::D3D12_DSV_DIMENSION_TEXTURE2D;
	desc.Flags			= D3D12_DSV_FLAGS::D3D12_DSV_FLAG_NONE;

	//深度ステンシルビュー生成
	dev->CreateDepthStencilView(depth.resource, &desc, depth.heap->GetCPUDescriptorHandleForHeapStart());

	return result;
}

// フェンスの生成
HRESULT Device::CreateFence(void)
{
	if (dev == nullptr)
	{
		OutputDebugString(_T("\nデバイスが生成されていません\n"));
		return S_FALSE;
	}

	//フェンス生成
	{
		result = dev->CreateFence(fen.fenceCnt, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fen.fence));
		if (FAILED(result))
		{
			OutputDebugString(_T("\nフェンスの生成：失敗\n"));
			return result;
		}

		//フェンス値更新
		fen.fenceCnt = 1;
	}

	//フェンスイベント生成
	{
		fen.fenceEvent = CreateEventEx(nullptr, FALSE, FALSE, EVENT_ALL_ACCESS);
		if (fen.fenceEvent == nullptr)
		{
			OutputDebugString(_T("\nフェンスイベントの生成：失敗\n"));
			return S_FALSE;
		}
	}

	return result;
}

// シグネチャのシリアライズ
HRESULT Device::Serialize(void)
{
	//ディスクリプタレンジの設定
	D3D12_DESCRIPTOR_RANGE range[3];
	SecureZeroMemory(&range, sizeof(range));

	//ルートパラメータの設定
	D3D12_ROOT_PARAMETER param[3];
	SecureZeroMemory(&param, sizeof(param));

	//定数バッファ用
	range[0].RangeType								= D3D12_DESCRIPTOR_RANGE_TYPE::D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
	range[0].NumDescriptors							= 1;
	range[0].BaseShaderRegister						= 0;
	range[0].RegisterSpace							= 0;
	range[0].OffsetInDescriptorsFromTableStart		= D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	param[0].ParameterType							= D3D12_ROOT_PARAMETER_TYPE::D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	param[0].ShaderVisibility						= D3D12_SHADER_VISIBILITY::D3D12_SHADER_VISIBILITY_ALL;
	param[0].DescriptorTable.NumDescriptorRanges	= 1;
	param[0].DescriptorTable.pDescriptorRanges		= &range[0];

	//テクスチャ用
	range[1].RangeType								= D3D12_DESCRIPTOR_RANGE_TYPE::D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	range[1].NumDescriptors							= 1;
	range[1].BaseShaderRegister						= 0;
	range[1].RegisterSpace							= 0;
	range[1].OffsetInDescriptorsFromTableStart		= D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	param[1].ParameterType							= D3D12_ROOT_PARAMETER_TYPE::D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	param[1].ShaderVisibility						= D3D12_SHADER_VISIBILITY::D3D12_SHADER_VISIBILITY_PIXEL;
	param[1].DescriptorTable.NumDescriptorRanges	= 1;
	param[1].DescriptorTable.pDescriptorRanges		= &range[1];

	//マテリアル用
	range[2].RangeType								= D3D12_DESCRIPTOR_RANGE_TYPE::D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
	range[2].NumDescriptors							= 1;
	range[2].BaseShaderRegister						= 1;
	range[2].RegisterSpace							= 0;
	range[2].OffsetInDescriptorsFromTableStart		= D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	param[2].ParameterType							= D3D12_ROOT_PARAMETER_TYPE::D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	param[2].ShaderVisibility						= D3D12_SHADER_VISIBILITY::D3D12_SHADER_VISIBILITY_ALL;
	param[2].DescriptorTable.NumDescriptorRanges	= 1;
	param[2].DescriptorTable.pDescriptorRanges		= &range[2];

	//静的サンプラーの設定
	D3D12_STATIC_SAMPLER_DESC sampler = {};
	sampler.Filter									= D3D12_FILTER::D3D12_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR;
	sampler.AddressU								= D3D12_TEXTURE_ADDRESS_MODE::D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	sampler.AddressV								= D3D12_TEXTURE_ADDRESS_MODE::D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	sampler.AddressW								= D3D12_TEXTURE_ADDRESS_MODE::D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	sampler.MipLODBias								= 0;
	sampler.MaxAnisotropy							= 0;
	sampler.ComparisonFunc							= D3D12_COMPARISON_FUNC::D3D12_COMPARISON_FUNC_NEVER;
	sampler.BorderColor								= D3D12_STATIC_BORDER_COLOR::D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
	sampler.MinLOD									= 0.0f;
	sampler.MaxLOD									= D3D12_FLOAT32_MAX;
	sampler.ShaderRegister							= 0;
	sampler.RegisterSpace							= 0;
	sampler.ShaderVisibility						= D3D12_SHADER_VISIBILITY::D3D12_SHADER_VISIBILITY_ALL;

	//ルートシグネチャ設定用構造体の設定
	D3D12_ROOT_SIGNATURE_DESC desc = {};
	desc.NumParameters								= _countof(param);
	desc.pParameters								= param;
	desc.NumStaticSamplers							= 1;
	desc.pStaticSamplers							= &sampler;
	desc.Flags										= D3D12_ROOT_SIGNATURE_FLAGS::D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

	//ルートシグネチャのシリアライズ化
	{
		result = D3D12SerializeRootSignature(&desc, D3D_ROOT_SIGNATURE_VERSION::D3D_ROOT_SIGNATURE_VERSION_1, &sig.signature, &sig.error);
		if (FAILED(result))
		{
			OutputDebugString(_T("\nシリアライズ化：失敗\n"));
			return result;
		}
	}

	return result;
}

// ルートシグネチャの生成
HRESULT Device::CreateRootSigunature(void)
{
	result = Serialize();
	if (FAILED(result))
	{
		return result;
	}
	
	//ルートシグネチャ生成
	result = dev->CreateRootSignature(0, sig.signature->GetBufferPointer(), sig.signature->GetBufferSize(), IID_PPV_ARGS(&sig.rootSignature));
	if (FAILED(result))
	{
		OutputDebugString(_T("\nルートシグネチャの生成：失敗\n"));
		return result;
	}

	return result;
}

// シェーダのコンパイル
HRESULT Device::ShaderCompile(LPCWSTR fileName)
{
	//頂点シェーダのコンパイル
	{
		result = D3DCompileFromFile(fileName, nullptr, nullptr, "BasicVS", "vs_5_0", D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION, 0, &pipe.vertex, &sig.error);
		if (FAILED(result))
		{
			OutputDebugString(_T("\n頂点シェーダコンパイル：失敗\n"));
			return result;
		}
	}

	//ピクセルシェーダのコンパイル
	{
		result = D3DCompileFromFile(fileName, nullptr, nullptr, "BasicPS", "ps_5_0", D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION, 0, &pipe.pixel, &sig.error);
		if (FAILED(result))
		{
			OutputDebugString(_T("\nピクセルシェーダコンパイル：失敗\n"));
			return result;
		}
	}

	return result;
}

// パイプラインの生成
HRESULT Device::CreatePipeline(void)
{
	//頂点レイアウト設定用構造体の設定
	D3D12_INPUT_ELEMENT_DESC input[] =
	{
		{ "POSITION", 0, DXGI_FORMAT::DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "NORMAL",   0, DXGI_FORMAT::DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT::DXGI_FORMAT_R32G32_FLOAT,    0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	};

	//ラスタライザーステート設定用構造体の設定
	D3D12_RASTERIZER_DESC rasterizer = {};
	rasterizer.FillMode						= D3D12_FILL_MODE::D3D12_FILL_MODE_SOLID;
	rasterizer.CullMode						= D3D12_CULL_MODE::D3D12_CULL_MODE_NONE;
	rasterizer.FrontCounterClockwise		= FALSE;
	rasterizer.DepthBias					= D3D12_DEFAULT_DEPTH_BIAS;
	rasterizer.DepthBiasClamp				= D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
	rasterizer.SlopeScaledDepthBias			= D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
	rasterizer.DepthClipEnable				= TRUE;
	rasterizer.MultisampleEnable			= FALSE;
	rasterizer.AntialiasedLineEnable		= FALSE;
	rasterizer.ForcedSampleCount			= 0;
	rasterizer.ConservativeRaster			= D3D12_CONSERVATIVE_RASTERIZATION_MODE::D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;

	//レンダーターゲットブレンド設定用構造体
	D3D12_RENDER_TARGET_BLEND_DESC renderBlend = {};
	renderBlend.BlendEnable					= FALSE;
	renderBlend.BlendOp						= D3D12_BLEND_OP::D3D12_BLEND_OP_ADD;
	renderBlend.BlendOpAlpha				= D3D12_BLEND_OP::D3D12_BLEND_OP_ADD;
	renderBlend.DestBlend					= D3D12_BLEND::D3D12_BLEND_ZERO;
	renderBlend.DestBlendAlpha				= D3D12_BLEND::D3D12_BLEND_ZERO;
	renderBlend.LogicOp						= D3D12_LOGIC_OP::D3D12_LOGIC_OP_NOOP;
	renderBlend.LogicOpEnable				= FALSE;
	renderBlend.RenderTargetWriteMask		= D3D12_COLOR_WRITE_ENABLE::D3D12_COLOR_WRITE_ENABLE_ALL;
	renderBlend.SrcBlend					= D3D12_BLEND::D3D12_BLEND_ONE;
	renderBlend.SrcBlendAlpha				= D3D12_BLEND::D3D12_BLEND_ONE;

	//ブレンドステート設定用構造体
	D3D12_BLEND_DESC descBS = {};
	descBS.AlphaToCoverageEnable			= TRUE;
	descBS.IndependentBlendEnable			= FALSE;
	for (UINT i = 0; i < swap.bufferCnt; i++)
	{
		descBS.RenderTarget[i]				= renderBlend;
	}

	//パイプラインステート設定用構造体
	D3D12_GRAPHICS_PIPELINE_STATE_DESC desc = {};
	desc.InputLayout						= { input, _countof(input) };
	desc.PrimitiveTopologyType				= D3D12_PRIMITIVE_TOPOLOGY_TYPE::D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	desc.pRootSignature						= sig.rootSignature;
	desc.VS									= CD3DX12_SHADER_BYTECODE(pipe.vertex);
	desc.PS									= CD3DX12_SHADER_BYTECODE(pipe.pixel);
	desc.RasterizerState					= rasterizer;
	desc.BlendState							= descBS;
	desc.DepthStencilState.DepthEnable		= true;
	desc.DepthStencilState.DepthWriteMask	= D3D12_DEPTH_WRITE_MASK::D3D12_DEPTH_WRITE_MASK_ALL;
	desc.DepthStencilState.DepthFunc		= D3D12_COMPARISON_FUNC::D3D12_COMPARISON_FUNC_LESS;
	desc.DepthStencilState.StencilEnable	= FALSE;
	desc.SampleMask							= UINT_MAX;
	desc.NumRenderTargets					= 1;
	desc.RTVFormats[0]						= DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM;
	desc.DSVFormat							= DXGI_FORMAT::DXGI_FORMAT_D32_FLOAT;
	desc.SampleDesc.Count					= 1;

	//パイプライン生成
	{
		result = dev->CreateGraphicsPipelineState(&desc, IID_PPV_ARGS(&pipe.pipeline));
		if (FAILED(result))
		{
			OutputDebugString(_T("\nパイプラインの生成：失敗\n"));
			return result;
		}
	}

	return result;
}

// 定数バッファ用ヒープの生成
HRESULT Device::CreateConstantHeap(void)
{
	//定数バッファ設定用構造体の設定
	D3D12_DESCRIPTOR_HEAP_DESC desc = {};
	desc.NumDescriptors		= 2;
	desc.Flags				= D3D12_DESCRIPTOR_HEAP_FLAGS::D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	desc.Type				= D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;

	//ヒープ生成
	{
		result = dev->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&con.heap));
		if (FAILED(result))
		{
			OutputDebugString(_T("\n定数バッファ用ヒープの生成：失敗\n"));
			return result;
		}

		//ヒープサイズ設定
		con.size = dev->GetDescriptorHandleIncrementSize(desc.Type);
	}

	return result;
}

// 定数バッファの生成
HRESULT Device::CreateConstant(void)
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
	desc.Width					= ((sizeof(WVP) + 0xff) &~0xff);
	desc.Height					= 1;
	desc.DepthOrArraySize		= 1;
	desc.MipLevels				= 1;
	desc.Format					= DXGI_FORMAT::DXGI_FORMAT_UNKNOWN;
	desc.SampleDesc.Count		= 1;
	desc.Flags					= D3D12_RESOURCE_FLAGS::D3D12_RESOURCE_FLAG_NONE;
	desc.Layout					= D3D12_TEXTURE_LAYOUT::D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

	//リソース生成
	{
		result = dev->CreateCommittedResource(&prop, D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE, &desc, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&con.resource));
		if (FAILED(result))
		{
			OutputDebugString(_T("\n定数バッファ用リソースの生成：失敗\n"));
			return result;
		}
	}

	return result;
}

// 定数バッファビューの生成
HRESULT Device::CreateConstantView(void)
{
	result = CreateConstant();
	if (FAILED(result))
	{
		return result;
	}

	//定数バッファビュー設定用構造体の設定
	D3D12_CONSTANT_BUFFER_VIEW_DESC desc = {};
	desc.BufferLocation		= con.resource->GetGPUVirtualAddress();
	desc.SizeInBytes		= (sizeof(WVP) + 0xff) &~0xff;

	{
		//定数バッファビュー生成
		dev->CreateConstantBufferView(&desc, con.heap->GetCPUDescriptorHandleForHeapStart());
	}

	//送信範囲
	D3D12_RANGE range = { 0, 0 };

	//マッピング
	{
		result = con.resource->Map(0, &range, (void**)(&con.data));
		if (FAILED(result))
		{
			OutputDebugString(_T("\n定数バッファ用リソースのマッピング：失敗\n"));
			return result;
		}

		//コピー
		memcpy(con.data, &wvp, sizeof(DirectX::XMMATRIX));
	}

	return result;
}

// ビューポートのセット
void Device::SetViewPort(void)
{
	viewPort.TopLeftX	= 0;
	viewPort.TopLeftY	= 0;
	viewPort.Width		= WINDOW_X;
	viewPort.Height		= WINDOW_Y;
	viewPort.MinDepth	= 0;
	viewPort.MaxDepth	= 1;
}

// シザーのセット
void Device::SetScissor(void)
{
	scissor.left	= 0;
	scissor.right	= WINDOW_X;
	scissor.top		= 0;
	scissor.bottom	= WINDOW_Y;
}

// バリアの更新
void Device::Barrier(D3D12_RESOURCE_STATES befor, D3D12_RESOURCE_STATES affter)
{
	barrier.Type					= D3D12_RESOURCE_BARRIER_TYPE::D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrier.Flags					= D3D12_RESOURCE_BARRIER_FLAGS::D3D12_RESOURCE_BARRIER_FLAG_NONE;
	barrier.Transition.pResource	= render.resource[swap.swapChain->GetCurrentBackBufferIndex()];
	barrier.Transition.StateBefore	= befor;
	barrier.Transition.StateAfter	= affter;
	barrier.Transition.Subresource	= D3D12_RESOURCE_BARRIER_FLAGS::D3D12_RESOURCE_BARRIER_FLAG_NONE;

	//バリア設置
	{
		com.list->ResourceBarrier(1, &barrier);
	}
}

// 待機処理
void Device::Wait(void)
{
	//フェンス値更新
	fen.fenceCnt++;

	//フェンス値を変更
	{
		result = com.queue->Signal(fen.fence, fen.fenceCnt);
		if (FAILED(result))
		{
			OutputDebugString(_T("\nフェンス値の更新：失敗\n"));
			return;
		}
	}

	//完了を待機(ポーリング)
	while (fen.fence->GetCompletedValue() != fen.fenceCnt)
	{
		//フェンスイベントのセット
		result = fen.fence->SetEventOnCompletion(fen.fenceCnt, fen.fenceEvent);
		if (FAILED(result))
		{
			OutputDebugString(_T("\nフェンスイベントのセット：失敗\n"));
			return;
		}

		//フェンスイベントの待機
		WaitForSingleObject(fen.fenceEvent, INFINITE);
	}
}