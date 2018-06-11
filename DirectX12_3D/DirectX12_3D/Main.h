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

// インスタンス処理
void Create(void);

// メモリ解放処理
void Destroy(void);


// ウィンドウ
EXTERN_MAIN std::shared_ptr<Window>win;

// インプット
EXTERN_MAIN std::shared_ptr<Input>input;

// デバイス
EXTERN_MAIN std::shared_ptr<Device>dev;

// テクスチャ
EXTERN_MAIN std::shared_ptr<Texture>tex;

// PMD
EXTERN_MAIN std::shared_ptr<PMD>pmd;