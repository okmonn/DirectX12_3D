#pragma once
#include "Texture.h"

class PMD
{
	// ���_�o�b�t�@
	struct VertexBuffer
	{
		//���\�[�X
		ID3D12Resource*				resource;
		//���_�o�b�t�@�r���[
		D3D12_VERTEX_BUFFER_VIEW	view;
		//���M�f�[�^
		UINT*						data;
	};

	// ���_�C���f�b�N�X
	struct VertexIndex
	{
		//���\�[�X
		ID3D12Resource*			resource;
		//���M�f�[�^
		UINT*					data;
		//���_�C���f�b�N�X�r���[
		D3D12_INDEX_BUFFER_VIEW	view;
	};

	// �萔�o�b�t�@
	struct Constant
	{
		//�q�[�v
		ID3D12DescriptorHeap*	heap;
		//���\�[�X
		ID3D12Resource*			resource;
		//���M�f�[�^
		UINT8*					data;
		//�T�C�Y
		UINT					size;
	};

	// �}�e���A��
	struct Mat
	{
		//��{�F
		DirectX::XMFLOAT3 diffuse;
		//�e�N�X�`���t���O
		BOOL texFlag;
	};

#pragma pack(1)
	// �w�b�_�[
	struct Header
	{
		//�^�C�v
		UCHAR	type[3];
		//�o�[�W����
		FLOAT	ver;
		//���O
		UCHAR	name[20];
		//�R�����g
		UCHAR	comment[256];
		//���_��
		UINT	vertexNum;
	};
#pragma pack()

	// ���_
	struct VertexData
	{
		//���W
		DirectX::XMFLOAT3	pos;
		//�@��
		DirectX::XMFLOAT3	normal;
		//uv
		DirectX::XMFLOAT2	uv;
		//�{�[���ԍ�
		USHORT				bornNum[2];
		//�E�F�C�g
		UCHAR				bornWeight;
		//�֊s���t���O
		UCHAR				edge;
	};

#pragma pack(1)
	// �}�e���A��
	struct MaterialData
	{
		//��{�F
		DirectX::XMFLOAT3	diffuse;
		//�����x
		FLOAT				alpha;
		//���ˋ��x
		FLOAT				specularity;
		//���ːF
		DirectX::XMFLOAT3	specula;
		//���F
		DirectX::XMFLOAT3	mirror;
		//�g�D�[���ԍ�
		UCHAR				toonIndex;
		//�֊s���t���O
		UCHAR				edge;
		//�C���f�b�N�X��
		UINT				indexNum;
		//�e�N�X�`���p�X
		CHAR				textureFilePath[20];
	};
#pragma pack()

	// �{�[��
	struct BornData
	{
		//���O
		CHAR				name[20];
		//�e�{�[���ԍ�
		WORD				parent_born_index;
		//�q�{�[���ԍ�
		WORD				child_born_index;
		//�^�C�v
		BYTE				type;
		//IK�e�{�[���ԍ�
		WORD				ik_parent_born_index;
		//���W
		DirectX::XMFLOAT3	pos;
	};

	// �{�[�����W
	struct Pos
	{
		//�擪���W
		DirectX::XMFLOAT3	head;
		//�������W
		DirectX::XMFLOAT3	tail;
	};

	// �{�[���m�[�h
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
	// �R���X�g���N�^
	PMD(std::weak_ptr<Device>dev, std::weak_ptr<Texture>tex);
	// �f�X�g���N�^
	~PMD();

	// ������̌���
	std::string FindString(const std::string path, const CHAR find, INT offset = 1, bool start = false);

	// �t�H���_�[�Ƃ̘A��
	std::string FolderPath(std::string path, const char* textureName);

	// �ǂݍ���
	HRESULT Load(std::string fileName);

	// �e�N�X�`���̓ǂݍ���
	HRESULT LoadTexture(std::string fileName);

	// �`��
	void Draw(void);

private:
	// ���_�o�b�t�@�̐���
	HRESULT CreateVertexBuffer(void);
	// ���_�C���f�b�N�X�̐���
	HRESULT CreateVertexIndex(void);


	// �萔�o�b�t�@�p�q�[�v�̐���
	HRESULT CreateConstantHeap(void);
	// �萔�o�b�t�@�p���\�[�X�̐���
	HRESULT CreateConstant(void);
	// �萔�o�b�t�@�r���[�̐���
	HRESULT CreateConstantView(void);


	// �f�o�C�X
	std::weak_ptr<Device>dev;

	// �e�N�X�`��
	std::weak_ptr<Texture>tex;

	// �Q�ƌ���
	HRESULT result;

	// PMD
	Pmd pmd;

	// �}�e���A��
	Mat mat;
};