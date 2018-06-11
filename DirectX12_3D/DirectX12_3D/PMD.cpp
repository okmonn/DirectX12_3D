#include "PMD.h"
#include "d3dx12.h"
#include <sstream>
#include <tchar.h>

// �R���X�g���N�^
PMD::PMD(std::weak_ptr<Device>dev, std::weak_ptr<Texture>tex) : dev(dev), tex(tex)
{
	//�Q�ƌ���
	result = S_OK;

	//PMD
	pmd = {};

	//�}�e���A��
	mat = {};
}

// �f�X�g���N�^
PMD::~PMD()
{
	for (auto itr = pmd.begin(); itr != pmd.end(); ++itr)
	{
		itr->second.con.resource->Unmap(0, nullptr);

		RELEASE(itr->second.icon.resource);
		RELEASE(itr->second.vcon.resource);
		RELEASE(itr->second.con.resource);
		RELEASE(itr->second.con.heap);
	}
}

// ������̌���
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

// �t�H���_�[�Ƃ̘A��
std::string PMD::FolderPath(std::string path, const char* textureName)
{
	//�_�~�[�錾
	int pathIndex1 = path.rfind('/');
	int pathIndex2 = path.rfind('\\');
	int pathIndex = max(pathIndex1, pathIndex2);

	std::string folderPath = path.substr(0, pathIndex);
	folderPath += "/";
	folderPath += textureName;

	return folderPath;
}

// �ǂݍ���
HRESULT PMD::Load(USHORT* index, std::string fileName)
{
	//�t�@�C��
	FILE *file;

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

	//�w�b�_�[�̓ǂݍ���
	{
		fread(&pmd[index].header, sizeof(pmd[index].header), 1, file);
	}

	//���_�f�[�^�̓ǂݍ���
	{
		//���_�f�[�^�z��̃������T�C�Y�m��
		pmd[index].vertex.resize(pmd[index].header.vertexNum);

		for (auto& v : pmd[index].vertex)
		{
			fread(&v.pos,        sizeof(v.pos),        1, file);
			fread(&v.normal,     sizeof(v.normal),     1, file);
			fread(&v.uv,         sizeof(v.uv),         1, file);
			fread(&v.bornNum,    sizeof(v.bornNum),    1, file);
			fread(&v.bornWeight, sizeof(v.bornWeight), 1, file);
			fread(&v.edge,       sizeof(v.edge),       1, file);
		}
	}

	//�C���f�b�N�X�f�[�^�̓ǂݍ���
	{
		//�C���f�b�N�X���i�[�p
		UINT indexNum = 0;
		//�C���f�b�N�X���̓ǂݍ���
		fread(&indexNum, sizeof(UINT), 1, file);

		//�C���f�b�N�X�f�[�^�z��̃������T�C�Y�m��
		pmd[index].index.resize(indexNum);

		for (auto& i : pmd[index].index)
		{
			fread(&i, sizeof(USHORT), 1, file);
		}
	}
	
	//�}�e���A���f�[�^�̓ǂݍ���
	{
		//�}�e���A�����i�[�p
		UINT materialNum = 0;
		//�}�e���A�����̓ǂݍ���
		fread(&materialNum, sizeof(UINT), 1, file);

		//�}�e���A���f�[�^�z��̃������T�C�Y�m��
		pmd[index].material.resize(materialNum);

		fread(&pmd[index].material[0], sizeof(MaterialData), materialNum, file);
	}

	//�{�[���f�[�^�̓ǂݍ���
	{
		//�{�[�����i�[�p
		UINT bornNum = 0;
		//�{�[�����̓ǂݍ���
		fread(&bornNum, sizeof(USHORT), 1, file);

		//�{�[���f�[�^�z��̃������T�C�Y�m��
		pmd[index].born.resize(bornNum);

		for (auto& b : pmd[index].born)
		{
			fread(&b.name,                 sizeof(b.name),				   1, file);
			fread(&b.parent_born_index,    sizeof(b.parent_born_index),	   1, file);
			fread(&b.child_born_index,     sizeof(b.child_born_index),	   1, file);
			fread(&b.type,                 sizeof(b.type),				   1, file);
			fread(&b.ik_parent_born_index, sizeof(b.ik_parent_born_index), 1, file);
			fread(&b.pos,				   sizeof(b.pos),				   1, file);
		}
	}

	//�t�@�C�������
	fclose(file);

	result = LoadTexture(index, FindString(fileName, '/'));
	result = CreateVertexIndex(index);
	result = CreateConstantView(index);

	return result;
}

// �e�N�X�`���̓ǂݍ���
HRESULT PMD::LoadTexture(USHORT* index, std::string fileName)
{
	for (UINT i = 0; i < pmd[index].material.size(); ++i)
	{
		if (pmd[index].material[i].textureFilePath[0] != '\0')
		{
			tex.lock()->LoadWIC(&pmd[index].id[i], tex.lock()->ChangeUnicode(FolderPath(fileName, pmd[index].material[i].textureFilePath).c_str()));
			if (FAILED(result))
			{
				OutputDebugString(_T("\nPMD�̃e�N�X�`���ǂݍ��݁F���s\n"));
				return result;
			}
		}
	}
	return result;
}

// ���_�o�b�t�@�̐���
HRESULT PMD::CreateVertexBuffer(USHORT* index)
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
	desc.Width					= (sizeof(VertexData) * pmd[index].vertex.size());
	desc.Height					= 1;
	desc.DepthOrArraySize		= 1;
	desc.MipLevels				= 1;
	desc.Format					= DXGI_FORMAT::DXGI_FORMAT_UNKNOWN;
	desc.SampleDesc.Count		= 1;
	desc.Flags					= D3D12_RESOURCE_FLAGS::D3D12_RESOURCE_FLAG_NONE;
	desc.Layout					= D3D12_TEXTURE_LAYOUT::D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

	//���\�[�X����
	{
		result = dev.lock()->GetDevice()->CreateCommittedResource(&prop, D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE, &desc, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&pmd[index].vcon.resource));
		if (FAILED(result))
		{
			OutputDebugString(_T("\nPMD�̒��_�o�b�t�@�p���\�[�X�̐����F���s\n"));
			return result;
		}
	}

	//���M�͈�
	D3D12_RANGE range = { 0,0 };

	//�}�b�s���O
	{
		result = pmd[index].vcon.resource->Map(0, &range, reinterpret_cast<void**>(&pmd[index].vcon.data));
		if (FAILED(result))
		{
			OutputDebugString(_T("\nPMD�̒��_�o�b�t�@�p���\�[�X�̃}�b�s���O�F���s\n"));
			return result;
		}

		//���_�f�[�^�̃R�s�[
		memcpy(pmd[index].vcon.data, &pmd[index].vertex[0], (sizeof(VertexData) * pmd[index].vertex.size()));
	}

	//�A���}�b�s���O
	pmd[index].vcon.resource->Unmap(0, nullptr);

	//���_�o�b�t�@�ݒ�p�\���̂̐ݒ�
	{
		pmd[index].vcon.view.BufferLocation		= pmd[index].vcon.resource->GetGPUVirtualAddress();
		pmd[index].vcon.view.SizeInBytes		= sizeof(VertexData) * pmd[index].vertex.size();
		pmd[index].vcon.view.StrideInBytes		= sizeof(VertexData);
	}

	return result;
}

// ���_�C���f�b�N�X�̐���
HRESULT PMD::CreateVertexIndex(USHORT* index)
{
	result = CreateVertexBuffer(index);
	if (FAILED(result))
	{
		return result;
	}

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
	desc.Width					= (sizeof(USHORT) * pmd[index].index.size());
	desc.Height					= 1;
	desc.DepthOrArraySize		= 1;
	desc.MipLevels				= 1;
	desc.Format					= DXGI_FORMAT::DXGI_FORMAT_UNKNOWN;
	desc.SampleDesc.Count		= 1;
	desc.Flags					= D3D12_RESOURCE_FLAGS::D3D12_RESOURCE_FLAG_NONE;
	desc.Layout					= D3D12_TEXTURE_LAYOUT::D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

	//���\�[�X����
	{
		result = dev.lock()->GetDevice()->CreateCommittedResource(&prop, D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE, &desc, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&pmd[index].icon.resource));
		if (FAILED(result))
		{
			OutputDebugString(_T("\nPMD�̒��_�C���f�b�N�X�p���\�[�X�̐����F���s\n"));
			return result;
		}
	}

	//���M�͈�
	D3D12_RANGE range = { 0,0 };

	//�}�b�s���O
	{
		result = pmd[index].icon.resource->Map(0, &range, reinterpret_cast<void**>(&pmd[index].icon.data));
		if (FAILED(result))
		{
			OutputDebugString(_T("\nPMD�̒��_�C���f�b�N�X�̃}�b�s���O�F���s\n"));
			return result;
		}
	}

	//�R�s�[
	memcpy(pmd[index].icon.data, &pmd[index].index[0], sizeof(USHORT) * pmd[index].index.size());

	//�A���}�b�s���O
	pmd[index].icon.resource->Unmap(0, nullptr);

	//���_�C���f�b�N�X�r���[�ݒ�p�\���̂̐ݒ�
	{
		pmd[index].icon.view.BufferLocation		= pmd[index].icon.resource->GetGPUVirtualAddress();
		pmd[index].icon.view.SizeInBytes		= sizeof(USHORT) * pmd[index].index.size();
		pmd[index].icon.view.Format				= DXGI_FORMAT::DXGI_FORMAT_R16_UINT;
	}

	return result;
}

// �萔�o�b�t�@�p�q�[�v�̐���
HRESULT PMD::CreateConstantHeap(USHORT* index)
{
	//�萔�o�b�t�@�ݒ�p�\���̂̐ݒ�
	D3D12_DESCRIPTOR_HEAP_DESC desc = {};
	desc.NumDescriptors		= 2 + pmd[index].material.size();
	desc.Flags				= D3D12_DESCRIPTOR_HEAP_FLAGS::D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	desc.Type				= D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;

	//�q�[�v����
	{
		result = dev.lock()->GetDevice()->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&pmd[index].con.heap));
		if (FAILED(result))
		{
			OutputDebugString(_T("\nPMD�̒萔�o�b�t�@�p�q�[�v�̐����F���s\n"));
			return S_FALSE;
		}

		//�q�[�v�T�C�Y���擾
		pmd[index].con.size = dev.lock()->GetDevice()->GetDescriptorHandleIncrementSize(desc.Type);
	}

	return result;
}

// �萔�o�b�t�@�p���\�[�X�̐���
HRESULT PMD::CreateConstant(USHORT* index)
{
	result = CreateConstantHeap(index);
	if (FAILED(result))
	{
		return result;
	}

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
	desc.Width					= ((sizeof(Mat) + 0xff) &~0xff) * ((sizeof(Mat) + 0xff) &~0xff);
	desc.Height					= 1;
	desc.DepthOrArraySize		= 1;
	desc.MipLevels				= 1;
	desc.Format					= DXGI_FORMAT::DXGI_FORMAT_UNKNOWN;
	desc.SampleDesc.Count		= 1;
	desc.Flags					= D3D12_RESOURCE_FLAGS::D3D12_RESOURCE_FLAG_NONE;
	desc.Layout					= D3D12_TEXTURE_LAYOUT::D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

	//���\�[�X����
	{
		result = dev.lock()->GetDevice()->CreateCommittedResource(&prop, D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE, &desc, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&pmd[index].con.resource));
		if (FAILED(result))
		{
			OutputDebugString(_T("\nPMD�̒萔�o�b�t�@�p���\�[�X�̐����F���s\n"));
			return result;
		}
	}

	return result;
}

// �萔�o�b�t�@�r���[�̐���
HRESULT PMD::CreateConstantView(USHORT* index)
{
	result = CreateConstant(index);
	if (FAILED(result))
	{
		return result;
	}

	//�萔�o�b�t�@�V�r���[�ݒ�p�\���̂̐ݒ�
	D3D12_CONSTANT_BUFFER_VIEW_DESC desc = {};
	desc.SizeInBytes = (sizeof(Mat) + 0xff) &~0xff;

	//GPU�A�h���X
	D3D12_GPU_VIRTUAL_ADDRESS address = pmd[index].con.resource->GetGPUVirtualAddress();
	//�f�B�X�N���v�^�[��CPU�n���h��
	D3D12_CPU_DESCRIPTOR_HANDLE handle = pmd[index].con.heap->GetCPUDescriptorHandleForHeapStart();

	for (UINT i = 0; i < pmd[index].material.size(); ++i)
	{
		desc.BufferLocation = address;

		//�萔�o�b�t�@�r���[����
		dev.lock()->GetDevice()->CreateConstantBufferView(&desc, handle);

		address += desc.SizeInBytes;

		handle.ptr += pmd[index].con.size;
	}

	//���M�͈�
	D3D12_RANGE range = { 0, 0 };

	//�}�b�s���O
	{
		result = pmd[index].con.resource->Map(0, &range, (void**)(&pmd[index].con.data));
		if (FAILED(result))
		{
			OutputDebugString(_T("\nPMD�̒萔�o�b�t�@�p���\�[�X�̃}�b�s���O�F���s\n"));
			return result;
		}

		//�R�s�[
		memcpy(pmd[index].con.data, &mat, sizeof(DirectX::XMMATRIX));
	}

	return result;
}

// �`��
void PMD::Draw(USHORT* index)
{
	//�Z�b�g
	{
		//���_�o�b�t�@�r���[�̃Z�b�g
		dev.lock()->GetComList()->IASetVertexBuffers(0, 1, &pmd[index].vcon.view);

		//���_�C���f�b�N�X�r���[�̃Z�b�g
		dev.lock()->GetComList()->IASetIndexBuffer(&pmd[index].icon.view);

		dev.lock()->GetComList()->IASetPrimitiveTopology(D3D12_PRIMITIVE_TOPOLOGY::D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	}

	//�I�t�Z�b�g
	UINT offset = 0;

	//���M�p�f�[�^
	UINT8* data = pmd[index].con.data;

	//�q�[�v�̐擪�n���h��
	D3D12_GPU_DESCRIPTOR_HANDLE handle = pmd[index].con.heap->GetGPUDescriptorHandleForHeapStart();

	for (UINT i = 0; i < pmd[index].material.size(); ++i)
	{
		mat.diffuse = pmd[index].material[i].diffuse;

		mat.texFlag = (pmd[index].material[i].textureFilePath[0] != '\0');
		if (mat.texFlag == TRUE)
		{
			tex.lock()->SetDrawWIC(&pmd[index].id[i]);
		}
		else
		{
			for (auto itr = pmd[index].id.begin(); itr != pmd[index].id.end(); ++itr)
			{
				tex.lock()->SetDescriptorWIC(&itr->second);
				break;
			}
		}

		//�萔�q�[�v�̃Z�b�g
		dev.lock()->GetComList()->SetDescriptorHeaps(1, &pmd[index].con.heap);

		//�f�B�X�N���v�^�[�e�[�u���̃Z�b�g
		dev.lock()->GetComList()->SetGraphicsRootDescriptorTable(2, handle);

		//�R�s�[
		memcpy(data, &mat, sizeof(Mat));

		//�`��
		dev.lock()->GetComList()->DrawIndexedInstanced(pmd[index].material[i].indexNum, 1, offset, 0, 0);

		//�n���h���X�V
		handle.ptr += pmd[index].con.size;

		//�f�[�^�X�V
		data = (UINT8*)(((sizeof(Mat) + 0xff) &~0xff) + (CHAR*)(data));

		//�I�t�Z�b�g�X�V
		offset +=pmd[index].material[i].indexNum;
	}
}