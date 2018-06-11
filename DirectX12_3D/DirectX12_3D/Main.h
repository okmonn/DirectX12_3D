#pragma once

#ifdef EXPORT_MAIN
#define EXTERN_MAIN
#else
#define EXTERN_MAIN extern
#endif

#include "Window.h"
#include "Input.h"
#include "Device.h"
#include "Texture.h"
#include "PMD.h"

// �C���X�^���X����
void Create(void);

// �������������
void Destroy(void);


// �E�B���h�E
EXTERN_MAIN std::shared_ptr<Window>win;

// �C���v�b�g
EXTERN_MAIN std::shared_ptr<Input>input;

// �f�o�C�X
EXTERN_MAIN std::shared_ptr<Device>dev;

// �e�N�X�`��
EXTERN_MAIN std::shared_ptr<Texture>tex;

// PMD
EXTERN_MAIN std::shared_ptr<PMD>pmd;