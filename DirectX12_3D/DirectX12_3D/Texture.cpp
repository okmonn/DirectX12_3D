#include "Texture.h"
#include "WICTextureLoader12.h"
#include <sstream>
#include <tchar.h>

#pragma comment (lib,"d3d12.lib")

// �R���X�g���N�^
Texture::Texture(std::weak_ptr<Device>dev) : dev(dev)
{
	//WIC�̏�������
	CoInitialize(nullptr);

	//�Q�ƌ���
	result = S_OK;

	//�N��
	origin.clear();

	//BMP�f�[�^
	bmp.clear();

	//WIC�f�[�^
	wic.clear();


	//�G���[���o�͂ɕ\��������
#ifdef _DEBUG
	ID3D12Debug *debug = nullptr;
	result = D3D12GetDebugInterface(IID_PPV_ARGS(&debug));
	if (FAILED(result))
		int i = 0;
	debug->EnableDebugLayer();
	debug->Release();
	debug = nullptr;
#endif
}

// �f�X�g���N�^
Texture::~Texture()
{
	for (auto itr = bmp.begin(); itr != bmp.end(); ++itr)
	{
		RELEASE(itr->second.vertex.resource);
		RELEASE(itr->second.resource);
		RELEASE(itr->second.heap);
	}
	for (auto itr = origin.begin(); itr != origin.end(); ++itr)
	{
		RELEASE(itr->second.resource);
		RELEASE(itr->second.heap);
	}
	for (auto itr = wic.begin(); itr != wic.end(); ++itr)
	{
		itr->second.decode.release();
		RELEASE(itr->second.vertex.resource);
		RELEASE(itr->second.resource);
		RELEASE(itr->second.heap);
	}
}

// ���j�R�[�h�ϊ�
std::wstring Texture::ChangeUnicode(const CHAR * str)
{
	//�������̎擾
	auto byteSize = MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED | MB_ERR_INVALID_CHARS, str, -1, nullptr, 0);

	std::wstring wstr;
	wstr.resize(byteSize);

	//�ϊ�
	byteSize = MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED | MB_ERR_INVALID_CHARS, str, -1, &wstr[0], byteSize);

	return wstr;
}

// �ǂݍ���
HRESULT Texture::LoadBMP(USHORT* index, std::string fileName)
{
	for (auto itr = origin.begin(); itr != origin.end(); ++itr)
	{
		if (itr->first == fileName)
		{
			bmp[index] = itr->second;
			
			return result;
			break;
		}
	}

	//BMP�w�b�_�[�\����
	BITMAPINFOHEADER header = {};

	//BMP�t�@�C���w�b�_�[
	BITMAPFILEHEADER fileheader = {};

	//�t�@�C��
	FILE* file;

	//�t�@�C���J�炭
	if ((fopen_s(&file, fileName.c_str(), "rb")) != 0)
	{
		//�G���[�i���o�[�m�F
		auto a = (fopen_s(&file, fileName.c_str(), "rb"));
		std::stringstream s;
		s << a;
		OutputDebugString(_T("\n�t�@�C�����J���܂���ł����F���s\n"));
		OutputDebugStringA(s.str().c_str());
		return S_FALSE;
	}

	//BMP�t�@�C���w�b�_�[�ǂݍ���
	fread(&fileheader, sizeof(fileheader), 1, file);

	//BMP�w�b�_�[�ǂݍ���
	fread(&header, sizeof(header), 1, file);

	//�摜�̕��ƍ����̕ۑ�
	origin[fileName].size = { header.biWidth, header.biHeight };

	if (header.biBitCount == 24)
	{
		//�f�[�^�T�C�Y���̃������m��(�r�b�g�̐[����24bit�̏ꍇ)
		origin[fileName].data.resize(header.biWidth * header.biHeight * 4);

		for (int line = header.biHeight - 1; line >= 0; --line)
		{
			for (int count = 0; count < header.biWidth * 4; count += 4)
			{
				//��ԍ��̔z��ԍ�
				UINT address = line * header.biWidth * 4;
				origin[fileName].data[address + count] = 0;
				fread(&origin[fileName].data[address + count + 1], sizeof(UCHAR), 3, file);
			}
		}
	}
	else if (header.biBitCount == 32)
	{
		//�f�[�^�T�C�Y���̃������m��(�r�b�g�̐[����32bit�̏ꍇ)
		origin[fileName].data.resize(header.biSizeImage);

		for (int line = header.biHeight - 1; line >= 0; --line)
		{
			for (int count = 0; count < header.biWidth * 4; count += 4)
			{
				//��ԍ��̔z��ԍ�
				UINT address = line * header.biWidth * 4;
				fread(&origin[fileName].data[address + count], sizeof(UCHAR), 4, file);
			}
		}
	}

	//�t�@�C�������
	fclose(file);

	result = CreateShaderResourceView(index, fileName);
	result = CreateVertex(index);

	bmp[index] = origin[fileName];

	return result;
}

// WIC�ǂݍ���
HRESULT Texture::LoadWIC(USHORT* index, std::wstring fileName)
{
	result = DirectX::LoadWICTextureFromFile(dev.lock()->GetDevice(), fileName.c_str(), &wic[index].resource, wic[index].decode, wic[index].sub);
	if (FAILED(result))
	{
		OutputDebugString(_T("\nWIC�e�N�X�`���̓ǂݍ��݁F���s\n"));
		return result;
	}

	result = CreateShaderResourceViewWIC(index);
	result = CreateVertexWIC(index);

	return result;
}

// �f�B�X�N���v�^�[�̃Z�b�g
void Texture::SetDescriptorBMP(USHORT * index)
{
	//�q�[�v�̐擪�n���h�����擾
	D3D12_GPU_DESCRIPTOR_HANDLE handle = bmp[index].heap->GetGPUDescriptorHandleForHeapStart();

	//�q�[�v�̃Z�b�g
	dev.lock()->GetComList()->SetDescriptorHeaps(1, &bmp[index].heap);

	//�f�B�X�N���v�^�[�e�[�u���̃Z�b�g
	dev.lock()->GetComList()->SetGraphicsRootDescriptorTable(1, handle);
}

// �f�B�X�N���v�^�[�̃Z�b�g
void Texture::SetDescriptorWIC(USHORT* index)
{
	//�q�[�v�̐擪�n���h�����擾
	D3D12_GPU_DESCRIPTOR_HANDLE handle = wic[index].heap->GetGPUDescriptorHandleForHeapStart();

	//�q�[�v�̃Z�b�g
	dev.lock()->GetComList()->SetDescriptorHeaps(1, &wic[index].heap);

	//�f�B�X�N���v�^�[�e�[�u���̃Z�b�g
	dev.lock()->GetComList()->SetGraphicsRootDescriptorTable(1, handle);
}

// �萔�o�b�t�@�p�̃q�[�v�̐���	
HRESULT Texture::CreateConstantHeap(USHORT* index, std::string fileName)
{
	//�q�[�v�ݒ�p�\���̂̐ݒ�
	D3D12_DESCRIPTOR_HEAP_DESC desc = {};
	desc.Flags				= D3D12_DESCRIPTOR_HEAP_FLAGS::D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	desc.NodeMask			= 0;
	desc.NumDescriptors		= 2;
	desc.Type				= D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;

	//�q�[�v����
	result = dev.lock()->GetDevice()->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&origin[fileName].heap));
	if (FAILED(result))
	{
		OutputDebugString(_T("\n�e�N�X�`���̒萔�o�b�t�@�p�q�[�v�̐����F���s\n"));
		return result;
	}

	return result;
}

// �萔�o�b�t�@�̐���
HRESULT Texture::CreateConstant(USHORT* index, std::string fileName)
{
	result = CreateConstantHeap(index, fileName);
	if (FAILED(result))
	{
		return result;
	}

	//�q�[�v�X�e�[�g�ݒ�p�\���̂̐ݒ�
	D3D12_HEAP_PROPERTIES prop = {};
	prop.CPUPageProperty		= D3D12_CPU_PAGE_PROPERTY::D3D12_CPU_PAGE_PROPERTY_WRITE_BACK;
	prop.CreationNodeMask		= 1;
	prop.MemoryPoolPreference	= D3D12_MEMORY_POOL::D3D12_MEMORY_POOL_L0;
	prop.Type					= D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_CUSTOM;
	prop.VisibleNodeMask		= 1;

	//���\�[�X�ݒ�p�\���̂̐ݒ�
	D3D12_RESOURCE_DESC desc = {};
	desc.Dimension				= D3D12_RESOURCE_DIMENSION::D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	desc.Width					= origin[fileName].size.width;
	desc.Height					= origin[fileName].size.height;
	desc.DepthOrArraySize		= 1;
	desc.Format					= DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM;
	desc.SampleDesc.Count		= 1;
	desc.Flags					= D3D12_RESOURCE_FLAGS::D3D12_RESOURCE_FLAG_NONE;
	desc.Layout					= D3D12_TEXTURE_LAYOUT::D3D12_TEXTURE_LAYOUT_UNKNOWN;

	//���\�[�X����
	{
		result = dev.lock()->GetDevice()->CreateCommittedResource(&prop, D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE, &desc, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&origin[fileName].resource));
		if (FAILED(result))
		{
			OutputDebugString(_T("\n�e�N�X�`���̒萔�o�b�t�@�p���\�[�X�̐����F���s\n"));
			return result;
		}
	}

	return result;
}

// �V�F�[�_���\�[�X�r���[�̐���
HRESULT Texture::CreateShaderResourceView(USHORT* index, std::string fileName)
{
	result = CreateConstant(index, fileName);
	if (FAILED(result))
	{
		return result;
	}
	
	//�V�F�[�_���\�[�X�r���[�ݒ�p�\���̂̐ݒ�
	D3D12_SHADER_RESOURCE_VIEW_DESC desc = {};
	desc.Format						= DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM;
	desc.ViewDimension				= D3D12_SRV_DIMENSION::D3D12_SRV_DIMENSION_TEXTURE2D;
	desc.Texture2D.MipLevels		= 1;
	desc.Texture2D.MostDetailedMip	= 0;
	desc.Shader4ComponentMapping	= D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

	//�q�[�v�̐擪�n���h�����擾
	D3D12_CPU_DESCRIPTOR_HANDLE handle = origin[fileName].heap->GetCPUDescriptorHandleForHeapStart();

	//�V�F�[�_�[���\�[�X�r���[�̐���
	dev.lock()->GetDevice()->CreateShaderResourceView(origin[fileName].resource, &desc, handle);

	return S_OK;
}

// �萔�o�b�t�@�p�q�[�v�̐���
HRESULT Texture::CreateConstantHeapWIC(USHORT* index)
{
	//�q�[�v�ݒ�p�\���̂̐ݒ�
	D3D12_DESCRIPTOR_HEAP_DESC desc = {};
	desc.Flags				= D3D12_DESCRIPTOR_HEAP_FLAGS::D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	desc.NodeMask			= 0;
	desc.NumDescriptors		= 2;
	desc.Type				= D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;

	//�q�[�v����
	{
		result = dev.lock()->GetDevice()->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&wic[index].heap));
		if (FAILED(result))
		{
			OutputDebugString(_T("\nWIC�e�N�X�`���̒萔�o�b�t�@�p�q�[�v�̐����F���s\n"));
			return result;
		}
	}

	return result;
}

// �V�F�[�_���\�[�X�r���[�̐���
HRESULT Texture::CreateShaderResourceViewWIC(USHORT* index)
{
	result = CreateConstantHeapWIC(index);
	if (FAILED(result))
	{
		return result;
	}

	//�V�F�[�_���\�[�X�r���[�ݒ�p�\���̂̐ݒ�
	D3D12_SHADER_RESOURCE_VIEW_DESC desc = {};
	desc.Format						= wic[index].resource->GetDesc().Format;
	desc.ViewDimension				= D3D12_SRV_DIMENSION::D3D12_SRV_DIMENSION_TEXTURE2D;
	desc.Texture2D.MipLevels		= 1;
	desc.Texture2D.MostDetailedMip	= 0;
	desc.Shader4ComponentMapping	= D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

	//�q�[�v�̐擪�n���h�����擾
	D3D12_CPU_DESCRIPTOR_HANDLE handle = wic[index].heap->GetCPUDescriptorHandleForHeapStart();

	//�V�F�[�_�[���\�[�X�r���[�̐���
	dev.lock()->GetDevice()->CreateShaderResourceView(wic[index].resource, &desc, handle);

	return S_OK;
}

// ���_���\�[�X�̐���
HRESULT Texture::CreateVertex(USHORT* index)
{
	//�q�[�v�ݒ�p�\���̂̐ݒ�
	D3D12_HEAP_PROPERTIES prop = {};
	prop.Type					= D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_UPLOAD;
	prop.CPUPageProperty		= D3D12_CPU_PAGE_PROPERTY::D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	prop.MemoryPoolPreference	= D3D12_MEMORY_POOL::D3D12_MEMORY_POOL_UNKNOWN;
	prop.CreationNodeMask		= 1;
	prop.VisibleNodeMask		= 1;

	//���\�[�X�ݒ�p�\���̂̐ݒ�
	D3D12_RESOURCE_DESC desc = {};
	desc.Dimension				= D3D12_RESOURCE_DIMENSION::D3D12_RESOURCE_DIMENSION_BUFFER;
	desc.Width					= (sizeof(bmp[index].vertex.vertex));//((sizeof(v[index].vertex) + 0xff) &~0xff);
	desc.Height					= 1;
	desc.DepthOrArraySize		= 1;
	desc.MipLevels				= 1;
	desc.Format					= DXGI_FORMAT::DXGI_FORMAT_UNKNOWN;
	desc.SampleDesc.Count		= 1;
	desc.Flags					= D3D12_RESOURCE_FLAGS::D3D12_RESOURCE_FLAG_NONE;
	desc.Layout					= D3D12_TEXTURE_LAYOUT::D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

	//���\�[�X����
	{
		result = dev.lock()->GetDevice()->CreateCommittedResource(&prop, D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE, &desc, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&bmp[index].vertex.resource));
		if (FAILED(result))
		{
			OutputDebugString(_T("\n�e�N�X�`���̒��_�o�b�t�@�p���\�[�X�̐����F���s\n"));
			return result;
		}
	}

	//���M�͈�
	D3D12_RANGE range = { 0,0 };

	//�}�b�s���O
	{
		result = bmp[index].vertex.resource->Map(0, &range, reinterpret_cast<void**>(&bmp[index].vertex.data));
		if (FAILED(result))
		{
			OutputDebugString(_T("\n���_�p���\�[�X�̃}�b�s���O�F���s\n"));
			return result;
		}

		//���_�f�[�^�̃R�s�[
		memcpy(bmp[index].vertex.data, &bmp[index].vertex.vertex, sizeof(bmp[index].vertex.vertex));
	}

	//���_�o�b�t�@�ݒ�p�\���̂̐ݒ�
	bmp[index].vertex.view.BufferLocation	= bmp[index].vertex.resource->GetGPUVirtualAddress();
	bmp[index].vertex.view.SizeInBytes		= sizeof(bmp[index].vertex.vertex);
	bmp[index].vertex.view.StrideInBytes	= sizeof(Vertex);

	return result;
}

// ���_���\�[�X�̐���
HRESULT Texture::CreateVertexWIC(USHORT* index)
{
	//�q�[�v�ݒ�p�\���̂̐ݒ�
	D3D12_HEAP_PROPERTIES prop = {};
	prop.Type					= D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_UPLOAD;
	prop.CPUPageProperty		= D3D12_CPU_PAGE_PROPERTY::D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	prop.MemoryPoolPreference	= D3D12_MEMORY_POOL::D3D12_MEMORY_POOL_UNKNOWN;
	prop.CreationNodeMask		= 1;
	prop.VisibleNodeMask		= 1;

	//���\�[�X�ݒ�p�\���̂̐ݒ�
	D3D12_RESOURCE_DESC desc = {};
	desc.Dimension				= D3D12_RESOURCE_DIMENSION::D3D12_RESOURCE_DIMENSION_BUFFER;
	desc.Width					= sizeof(wic[index].vertex.vertex);//((sizeof(v[index].vertex) + 0xff) &~0xff);
	desc.Height					= 1;
	desc.DepthOrArraySize		= 1;
	desc.MipLevels				= 1;
	desc.Format					= DXGI_FORMAT::DXGI_FORMAT_UNKNOWN;
	desc.SampleDesc.Count		= 1;
	desc.Flags					= D3D12_RESOURCE_FLAGS::D3D12_RESOURCE_FLAG_NONE;
	desc.Layout					= D3D12_TEXTURE_LAYOUT::D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

	//���\�[�X����
	{
		result = dev.lock()->GetDevice()->CreateCommittedResource(&prop, D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE, &desc, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&wic[index].vertex.resource));
		if (FAILED(result))
		{
			OutputDebugString(_T("\n�e�N�X�`���̒��_�o�b�t�@�p���\�[�X�̐����F���s\n"));
			return result;
		}
	}

	//���M�͈�
	D3D12_RANGE range = { 0,0 };

	//�}�b�s���O
	{
		result = wic[index].vertex.resource->Map(0, &range, reinterpret_cast<void**>(&wic[index].vertex.data));
		if (FAILED(result))
		{
			OutputDebugString(_T("\nWIC���_�p���\�[�X�̃}�b�s���O�F���s\n"));
			return result;
		}
	}

	//���_�f�[�^�̃R�s�[
	memcpy(wic[index].vertex.data, &wic[index].vertex.vertex, sizeof(wic[index].vertex.vertex));

	//���_�o�b�t�@�ݒ�p�\���̂̐ݒ�
	wic[index].vertex.view.BufferLocation = wic[index].vertex.resource->GetGPUVirtualAddress();
	wic[index].vertex.view.SizeInBytes = sizeof(wic[index].vertex.vertex);
	wic[index].vertex.view.StrideInBytes = sizeof(Vertex);

	return result;
}

// �`�揀��
void Texture::SetDrawBMP(USHORT* index)
{
	//�{�b�N�X�ݒ�p�\���̂̐ݒ�
	D3D12_BOX box = {};
	box.back	= 1;
	box.bottom	= bmp[index].size.height;
	box.front	= 0;
	box.left	= 0;
	box.right	= bmp[index].size.width;
	box.top		= 0;

	//�T�u���\�[�X�ɏ�������
	{
		result = bmp[index].resource->WriteToSubresource(0, &box, &bmp[index].data[0], (box.right * 4), (box.bottom * 4));
		if (FAILED(result))
		{
			OutputDebugString(_T("\n�e�N�X�`���̃T�u���\�[�X�ւ̏������݁F���s\n"));
			return;
		}
	}

	//�q�[�v�̃Z�b�g
	{
		SetDescriptorBMP(index);
	}
}

// �`�揀��
void Texture::SetDrawWIC(USHORT* index)
{
	//���\�[�X�ݒ�p�\����
	D3D12_RESOURCE_DESC desc = {};
	desc = wic[index].resource->GetDesc();

	//�{�b�N�X�ݒ�p�\���̂̐ݒ�
	D3D12_BOX box = {};
	box.back	= 1;
	box.bottom	= desc.Height;
	box.front	= 0;
	box.left	= 0;
	box.right	= (UINT)desc.Width;
	box.top		= 0;

	//�T�u���\�[�X�ɏ�������
	{
		result = wic[index].resource->WriteToSubresource(0, &box, wic[index].decode.get(), wic[index].sub.RowPitch, wic[index].sub.SlicePitch);
		if (FAILED(result))
		{
			OutputDebugString(_T("WIC�T�u���\�[�X�ւ̏������݁F���s\n"));
			return;
		}
	}

	//�q�[�v�̃Z�b�g
	{
		SetDescriptorWIC(index);
	}
}

// �`��
void Texture::DrawBMP(USHORT* index, Vector2<FLOAT>pos, Vector2<FLOAT>size)
{
	bmp[index].vertex.vertex[0] = { {  (pos.x / (FLOAT)(WINDOW_X / 2)) - 1.0f,			  1.0f -  (pos.y / (FLOAT)(WINDOW_Y / 2)),			 0.0f}, {0.0f, 0.0f} };//����
	bmp[index].vertex.vertex[1] = { { ((pos.x + size.x) / (FLOAT)(WINDOW_X / 2)) - 1.0f,  1.0f -  (pos.y / (FLOAT)(WINDOW_Y / 2)),			 0.0f}, {1.0f, 0.0f} };//�E��
	bmp[index].vertex.vertex[2] = { { ((pos.x + size.x) / (FLOAT)(WINDOW_X / 2)) - 1.0f,  1.0f - ((pos.y + size.y) / (FLOAT)(WINDOW_Y / 2)), 0.0f}, {1.0f, 1.0f} };//�E��
	bmp[index].vertex.vertex[3] = { { ((pos.x + size.x) / (FLOAT)(WINDOW_X / 2)) - 1.0f,  1.0f - ((pos.y + size.y) / (FLOAT)(WINDOW_Y / 2)), 0.0f}, {1.0f, 1.0f} };//�E��
	bmp[index].vertex.vertex[4] = { {  (pos.x / (FLOAT)(WINDOW_X / 2)) - 1.0f,			  1.0f - ((pos.y + size.y) / (FLOAT)(WINDOW_Y / 2)), 0.0f}, {0.0f, 1.0f} };//����
	bmp[index].vertex.vertex[5] = { {  (pos.x / (FLOAT)(WINDOW_X / 2)) - 1.0f,			  1.0f -  (pos.y / (FLOAT)(WINDOW_Y / 2)),			 0.0f}, {0.0f, 0.0f} };//����

	//���_�f�[�^�̃R�s�[
	memcpy(bmp[index].vertex.data, &bmp[index].vertex.vertex, (sizeof(bmp[index].vertex.vertex)));

	//���_�o�b�t�@�ݒ�p�\���̂̐ݒ�
	bmp[index].vertex.view.BufferLocation	= bmp[index].vertex.resource->GetGPUVirtualAddress();
	bmp[index].vertex.view.SizeInBytes		= sizeof(bmp[index].vertex.vertex);
	bmp[index].vertex.view.StrideInBytes	= sizeof(Vertex);
	
	//���_�o�b�t�@�r���[�̃Z�b�g
	dev.lock()->GetComList()->IASetVertexBuffers(0, 1, &bmp[index].vertex.view);

	//�g�|���W�[�ݒ�
	dev.lock()->GetComList()->IASetPrimitiveTopology(D3D12_PRIMITIVE_TOPOLOGY::D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	SetDrawBMP(index);

	//�`��
	dev.lock()->GetComList()->DrawInstanced(6, 1, 0, 0);
}

// �`��
void Texture::DrawWIC(USHORT* index, Vector2<FLOAT> pos, Vector2<FLOAT> size)
{
	wic[index].vertex.vertex[0] = { {  (pos.x / (FLOAT)(WINDOW_X / 2)) - 1.0f,			   1.0f -  (pos.y / (FLOAT)(WINDOW_Y / 2)),			  0.0f },{ 0.0f, 0.0f } };//����
	wic[index].vertex.vertex[1] = { { ((pos.x + size.x) / (FLOAT)(WINDOW_X / 2)) - 1.0f,   1.0f -  (pos.y / (FLOAT)(WINDOW_Y / 2)),			  0.0f },{ 1.0f, 0.0f } };//�E��
	wic[index].vertex.vertex[2] = { { ((pos.x + size.x) / (FLOAT)(WINDOW_X / 2)) - 1.0f,   1.0f - ((pos.y + size.y) / (FLOAT)(WINDOW_Y / 2)), 0.0f },{ 1.0f, 1.0f } };//�E��
	wic[index].vertex.vertex[3] = { { ((pos.x + size.x) / (FLOAT)(WINDOW_X / 2)) - 1.0f,   1.0f - ((pos.y + size.y) / (FLOAT)(WINDOW_Y / 2)), 0.0f },{ 1.0f, 1.0f } };//�E��
	wic[index].vertex.vertex[4] = { {  (pos.x / (FLOAT)(WINDOW_X / 2)) - 1.0f,			   1.0f - ((pos.y + size.y) / (FLOAT)(WINDOW_Y / 2)), 0.0f },{ 0.0f, 1.0f } };//����
	wic[index].vertex.vertex[5] = { {  (pos.x / (FLOAT)(WINDOW_X / 2)) - 1.0f,			   1.0f -  (pos.y / (FLOAT)(WINDOW_Y / 2)),			  0.0f },{ 0.0f, 0.0f } };//����

	//���_�f�[�^�̃R�s�[
	memcpy(wic[index].vertex.data, &wic[index].vertex.vertex, (sizeof(wic[index].vertex.vertex)));

	//���_�o�b�t�@�ݒ�p�\���̂̐ݒ�
	wic[index].vertex.view.BufferLocation	= wic[index].vertex.resource->GetGPUVirtualAddress();
	wic[index].vertex.view.SizeInBytes		= sizeof(wic[index].vertex.vertex);
	wic[index].vertex.view.StrideInBytes	= sizeof(Vertex);

	//���_�o�b�t�@�r���[�̃Z�b�g
	dev.lock()->GetComList()->IASetVertexBuffers(0, 1, &wic[index].vertex.view);

	//�g�|���W�[�ݒ�
	dev.lock()->GetComList()->IASetPrimitiveTopology(D3D12_PRIMITIVE_TOPOLOGY::D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	SetDrawWIC(index);

	//�`��
	dev.lock()->GetComList()->DrawInstanced(6, 1, 0, 0);
}

// �����`��
void Texture::DrawRect(USHORT* index, Vector2<FLOAT> pos, Vector2<FLOAT> size, Vector2<FLOAT> rect, Vector2<FLOAT> rSize, bool turn)
{
	if (turn == false)
	{
		bmp[index].vertex.vertex[0] = { {  (pos.x / (FLOAT)(WINDOW_X / 2)) - 1.0f,			  1.0f -  (pos.y / (FLOAT)(WINDOW_Y / 2)),			 0.0f },{  rect.x / (FLOAT)bmp[index].size.width,              rect.y / (FLOAT)bmp[index].size.height            } };//����
		bmp[index].vertex.vertex[1] = { { ((pos.x + size.x) / (FLOAT)(WINDOW_X / 2)) - 1.0f,  1.0f -  (pos.y / (FLOAT)(WINDOW_Y / 2)),			 0.0f },{ (rect.x + rSize.x) / (FLOAT)bmp[index].size.width,   rect.y / (FLOAT)bmp[index].size.height            } };//�E��
		bmp[index].vertex.vertex[2] = { { ((pos.x + size.x) / (FLOAT)(WINDOW_X / 2)) - 1.0f,  1.0f - ((pos.y + size.y) / (FLOAT)(WINDOW_Y / 2)), 0.0f },{ (rect.x + rSize.x) / (FLOAT)bmp[index].size.width, (rect.y + (FLOAT)rSize.y) / bmp[index].size.height  } };//�E��
		bmp[index].vertex.vertex[3] = { { ((pos.x + size.x) / (FLOAT)(WINDOW_X / 2)) - 1.0f,  1.0f - ((pos.y + size.y) / (FLOAT)(WINDOW_Y / 2)), 0.0f },{ (rect.x + rSize.x) / (FLOAT)bmp[index].size.width, (rect.y + (FLOAT)rSize.y) / bmp[index].size.height  } };//�E��
		bmp[index].vertex.vertex[4] = { {  (pos.x / (FLOAT)(WINDOW_X / 2)) - 1.0f,			  1.0f - ((pos.y + size.y) / (FLOAT)(WINDOW_Y / 2)), 0.0f },{  rect.x / (FLOAT)bmp[index].size.width,             (rect.y + rSize.y) / (FLOAT)bmp[index].size.height } };//����
		bmp[index].vertex.vertex[5] = { {  (pos.x / (FLOAT)(WINDOW_X / 2)) - 1.0f,			  1.0f -  (pos.y / (FLOAT)(WINDOW_Y / 2)),			 0.0f },{  rect.x / (FLOAT)bmp[index].size.width,              rect.y / (FLOAT)bmp[index].size.height            } };//����
	}
	else
	{
		bmp[index].vertex.vertex[0] = { {  (pos.x / (FLOAT)(WINDOW_X / 2)) - 1.0f,			  1.0f - (pos.y / (FLOAT)(WINDOW_Y / 2)),			 0.0f },{ (rect.x + rSize.x) / (FLOAT)bmp[index].size.width,   rect.y / (FLOAT)bmp[index].size.height            } };//����
		bmp[index].vertex.vertex[1] = { { ((pos.x + size.x) / (FLOAT)(WINDOW_X / 2)) - 1.0f,  1.0f - (pos.y / (FLOAT)(WINDOW_Y / 2)),			 0.0f },{  rect.x / (FLOAT)bmp[index].size.width,              rect.y / (FLOAT)bmp[index].size.height            } };//�E��
		bmp[index].vertex.vertex[2] = { { ((pos.x + size.x) / (FLOAT)(WINDOW_X / 2)) - 1.0f,  1.0f - ((pos.y + size.y) / (FLOAT)(WINDOW_Y / 2)), 0.0f },{  rect.x / (FLOAT)bmp[index].size.width,             (rect.y + rSize.y) / (FLOAT)bmp[index].size.height } };//�E��
		bmp[index].vertex.vertex[3] = { { ((pos.x + size.x) / (FLOAT)(WINDOW_X / 2)) - 1.0f,  1.0f - ((pos.y + size.y) / (FLOAT)(WINDOW_Y / 2)), 0.0f },{  rect.x / (FLOAT)bmp[index].size.width,             (rect.y + rSize.y) / (FLOAT)bmp[index].size.height } };//�E��
		bmp[index].vertex.vertex[4] = { {  (pos.x / (FLOAT)(WINDOW_X / 2)) - 1.0f,			  1.0f - ((pos.y + size.y) / (FLOAT)(WINDOW_Y / 2)), 0.0f },{ (rect.x + rSize.x) / (FLOAT)bmp[index].size.width,  (rect.y + (FLOAT)rSize.y) / bmp[index].size.height } };//����
		bmp[index].vertex.vertex[5] = { {  (pos.x / (FLOAT)(WINDOW_X / 2)) - 1.0f,			  1.0f - (pos.y / (FLOAT)(WINDOW_Y / 2)),			 0.0f },{ (rect.x + rSize.x) / (FLOAT)bmp[index].size.width,   rect.y / (FLOAT)bmp[index].size.height            } };//����
	}

	//���_�f�[�^�̃R�s�[
	memcpy(bmp[index].vertex.data, &bmp[index].vertex.vertex, (sizeof(bmp[index].vertex.vertex)));

	//���_�o�b�t�@�ݒ�p�\���̂̐ݒ�
	bmp[index].vertex.view.BufferLocation	= bmp[index].vertex.resource->GetGPUVirtualAddress();
	bmp[index].vertex.view.SizeInBytes		= sizeof(bmp[index].vertex.vertex);
	bmp[index].vertex.view.StrideInBytes	= sizeof(Vertex);

	//���_�o�b�t�@�r���[�̃Z�b�g
	dev.lock()->GetComList()->IASetVertexBuffers(0, 1, &bmp[index].vertex.view);

	//�g�|���W�[�ݒ�
	dev.lock()->GetComList()->IASetPrimitiveTopology(D3D12_PRIMITIVE_TOPOLOGY::D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	SetDrawBMP(index);

	//�`��
	dev.lock()->GetComList()->DrawInstanced(6, 1, 0, 0);
}

// �����`��
void Texture::DrawRectWIC(USHORT* index, Vector2<FLOAT> pos, Vector2<FLOAT> size, Vector2<FLOAT> rect, Vector2<FLOAT> rSize, bool turn)
{
	if (turn == false)
	{
		wic[index].vertex.vertex[0] = { {  (pos.x / (FLOAT)(WINDOW_X / 2)) - 1.0f,			   1.0f - (pos.y / (FLOAT)(WINDOW_Y / 2)),			  0.0f },{  rect.x / (FLOAT)(wic[index].resource->GetDesc().Width),             rect.y / (FLOAT)(wic[index].resource->GetDesc().Height)            } };//����
		wic[index].vertex.vertex[1] = { { ((pos.x + size.x) / (FLOAT)(WINDOW_X / 2)) - 1.0f,   1.0f - (pos.y / (FLOAT)(WINDOW_Y / 2)),			  0.0f },{ (rect.x + rSize.x) / (FLOAT)(wic[index].resource->GetDesc().Width),  rect.y / (FLOAT)(wic[index].resource->GetDesc().Height)            } };//�E��
		wic[index].vertex.vertex[2] = { { ((pos.x + size.x) / (FLOAT)(WINDOW_X / 2)) - 1.0f,   1.0f - ((pos.y + size.y) / (FLOAT)(WINDOW_Y / 2)), 0.0f },{ (rect.x + rSize.x) / (FLOAT)(wic[index].resource->GetDesc().Width), (rect.y + rSize.y) / (FLOAT)(wic[index].resource->GetDesc().Height) } };//�E��
		wic[index].vertex.vertex[3] = { { ((pos.x + size.x) / (FLOAT)(WINDOW_X / 2)) - 1.0f,   1.0f - ((pos.y + size.y) / (FLOAT)(WINDOW_Y / 2)), 0.0f },{ (rect.x + rSize.x) / (FLOAT)(wic[index].resource->GetDesc().Width), (rect.y + rSize.y) / (FLOAT)(wic[index].resource->GetDesc().Height) } };//�E��
		wic[index].vertex.vertex[4] = { {  (pos.x / (FLOAT)(WINDOW_X / 2)) - 1.0f,			   1.0f - ((pos.y + size.y) / (FLOAT)(WINDOW_Y / 2)), 0.0f },{  rect.x / (FLOAT)(wic[index].resource->GetDesc().Width),            (rect.y + rSize.y) / (FLOAT)(wic[index].resource->GetDesc().Height) } };//����
		wic[index].vertex.vertex[5] = { {  (pos.x / (FLOAT)(WINDOW_X / 2)) - 1.0f,			   1.0f - (pos.y / (FLOAT)(WINDOW_Y / 2)),			  0.0f },{  rect.x / (FLOAT)(wic[index].resource->GetDesc().Width),             rect.y / (FLOAT)(wic[index].resource->GetDesc().Height)            } };//����
	}
	else
	{
		wic[index].vertex.vertex[0] = { {  (pos.x / (FLOAT)(WINDOW_X / 2)) - 1.0f,			   1.0f - (pos.y / (FLOAT)(WINDOW_Y / 2)),			  0.0f },{ (rect.x + rSize.x) / (FLOAT)(wic[index].resource->GetDesc().Width),  rect.y / (FLOAT)(wic[index].resource->GetDesc().Height)            } };//����
		wic[index].vertex.vertex[1] = { { ((pos.x + size.x) / (FLOAT)(WINDOW_X / 2)) - 1.0f,   1.0f - (pos.y / (FLOAT)(WINDOW_Y / 2)),			  0.0f },{  rect.x / (FLOAT)(wic[index].resource->GetDesc().Width),             rect.y / (FLOAT)(wic[index].resource->GetDesc().Height)            } };//�E��
		wic[index].vertex.vertex[2] = { { ((pos.x + size.x) / (FLOAT)(WINDOW_X / 2)) - 1.0f,   1.0f - ((pos.y + size.y) / (FLOAT)(WINDOW_Y / 2)), 0.0f },{  rect.x / (FLOAT)(wic[index].resource->GetDesc().Width),            (rect.y + rSize.y) / (FLOAT)(wic[index].resource->GetDesc().Height) } };//�E��
		wic[index].vertex.vertex[3] = { { ((pos.x + size.x) / (FLOAT)(WINDOW_X / 2)) - 1.0f,   1.0f - ((pos.y + size.y) / (FLOAT)(WINDOW_Y / 2)), 0.0f },{  rect.x / (FLOAT)(wic[index].resource->GetDesc().Width),            (rect.y + rSize.y) / (FLOAT)(wic[index].resource->GetDesc().Height) } };//�E��
		wic[index].vertex.vertex[4] = { {  (pos.x / (FLOAT)(WINDOW_X / 2)) - 1.0f,			   1.0f - ((pos.y + size.y) / (FLOAT)(WINDOW_Y / 2)), 0.0f },{ (rect.x + rSize.x) / (FLOAT)(wic[index].resource->GetDesc().Width), (rect.y + rSize.y) / (FLOAT)(wic[index].resource->GetDesc().Height) } };//����
		wic[index].vertex.vertex[5] = { {  (pos.x / (FLOAT)(WINDOW_X / 2)) - 1.0f,			   1.0f - (pos.y / (FLOAT)(WINDOW_Y / 2)),			  0.0f },{ (rect.x + rSize.x) / (FLOAT)(wic[index].resource->GetDesc().Width),  rect.y / (FLOAT)(wic[index].resource->GetDesc().Height)            } };//����
	}

	//���_�f�[�^�̃R�s�[
	memcpy(wic[index].vertex.data, &wic[index].vertex.vertex, (sizeof(wic[index].vertex.vertex)));

	//���_�o�b�t�@�ݒ�p�\���̂̐ݒ�
	wic[index].vertex.view.BufferLocation	= wic[index].vertex.resource->GetGPUVirtualAddress();
	wic[index].vertex.view.SizeInBytes		= sizeof(wic[index].vertex.vertex);
	wic[index].vertex.view.StrideInBytes	= sizeof(Vertex);

	//���_�o�b�t�@�r���[�̃Z�b�g
	dev.lock()->GetComList()->IASetVertexBuffers(0, 1, &wic[index].vertex.view);

	//�g�|���W�[�ݒ�
	dev.lock()->GetComList()->IASetPrimitiveTopology(D3D12_PRIMITIVE_TOPOLOGY::D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	SetDrawWIC(index);

	//�`��
	dev.lock()->GetComList()->DrawInstanced(6, 1, 0, 0);
}