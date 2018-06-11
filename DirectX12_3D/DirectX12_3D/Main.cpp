#define EXPORT_MAIN
#include "Main.h"

// �C���X�^���X����
void Create(void)
{
	//�E�B���h�E�N���X�̃C���X�^��
	win = std::make_shared<Window>();

	//�C���v�b�g�N���X�̃C���X�^���X
	input = std::make_shared<Input>(win);

	//�f�o�C�X�N���X�̃C���X�^���X
	dev = std::make_shared<Device>(win);

	//�e�N�X�`��
	tex = std::make_shared<Texture>(dev);

	//PMD
	pmd = std::make_shared<PMD>(dev, tex);

	pmd->Load("img/�~�N.pmd");
}

// �������������
void Destroy(void)
{
}

// ���C���֐�
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	Create();

	//�\�����������E�B���h�E�n���h��,�\���̎w��(SW_����Ȃ��`�g�p)
	ShowWindow(win->GetWindowHandle(), nCmdShow);

	//���b�Z�[�W�p�\����
	MSG msg = {};

	// ���C�����[�v
	while (msg.message != WM_QUIT)
	{
		//�Ăяo�����X���b�h�����L���Ă���E�B���h�E�ɑ��M���ꂽ���b�Z�[�W�ۗ̕�����Ă��镨���擾
		if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
		{
			//���z�L�[���b�Z�[�W�𕶎����b�Z�[�W�ɕϊ�
			TranslateMessage(&msg);
			//1�̃E�B�h�E�v���V�[�W���Ƀ��b�Z�[�W�𑗏o����
			DispatchMessage(&msg);
		}
		else
		{
			//�G�X�P�[�v�L�[�Ń��[�v�I��
			if (input->InputKey(DIK_ESCAPE) == TRUE)
			{
				break;
			}

			dev->Set();
			pmd->Draw();
			dev->Do();
		}
	}

	Destroy();

	return msg.wParam;
}