#pragma once
#include "gfx/gfx.h"
#include <dxgiformat.h>
#include <d3dcompiler.h>

HRESULT CompileHLSLShader(char const* source_code, char const* target, char const* entrypoint, ID3DBlob** outBlob);

DXGI_FORMAT DXGIFormatFromGfxFormat(GfxPixelFormat format);
