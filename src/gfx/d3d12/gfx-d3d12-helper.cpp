#include "gfx-d3d12-helper.h"
#include <assert.h>
#include <stdio.h>
#include <d3dcompiler.h>

HRESULT CompileHLSLShader(char const* const sourceCode,
                          char const* const target,
                          char const* const entrypoint,
                          ID3DBlob** const outBlob)
{
    assert(sourceCode);
    assert(target);
    assert(entrypoint);
    auto const compilerModule = LoadLibraryA("d3dcompiler_47.dll");
    assert(compilerModule && "Need a D3D compiler");
    decltype(D3DCompile)* const pD3DCompile = reinterpret_cast<decltype(&D3DCompile)>(GetProcAddress(compilerModule, "D3DCompile"));
    assert(pD3DCompile && "Can't find D3DCompile method");

    UINT compilerFlags = D3DCOMPILE_PACK_MATRIX_COLUMN_MAJOR |
                         D3DCOMPILE_ENABLE_STRICTNESS |
                         D3DCOMPILE_WARNINGS_ARE_ERRORS |
                         D3DCOMPILE_ENABLE_UNBOUNDED_DESCRIPTOR_TABLES;
#if _DEBUG
    compilerFlags |= D3DCOMPILE_DEBUG;
#endif
    size_t const sourceCodeLength = strnlen(sourceCode, 1024 * 1024);
    ID3DBlob*   shaderBlob = nullptr;
    ID3DBlob*   errorBlob = nullptr;
    HRESULT const hr = pD3DCompile(sourceCode, sourceCodeLength,
                                   nullptr, nullptr, nullptr,
                                   entrypoint, target, compilerFlags, 0,
                                   &shaderBlob, &errorBlob);
    if (errorBlob) {
        fprintf(stderr, "Eror compiling shader: %s",
                static_cast<char const*>(errorBlob->GetBufferPointer()));
        errorBlob->Release();
    }
    if (FAILED(hr)) {
        return hr;
    }
    assert(shaderBlob && "Shader blob not created");
    assert(SUCCEEDED(hr));
    if (outBlob) {
        *outBlob = shaderBlob;
    }
    return hr;
}

DXGI_FORMAT DXGIFormatFromGfxFormat(GfxPixelFormat format)
{
    switch (format) {
        case kUnknown:
            return DXGI_FORMAT_UNKNOWN;
        // r8
        case kR8Uint:
            return DXGI_FORMAT_R8_UINT;
        case kR8Sint:
            return DXGI_FORMAT_R8_SINT;
        case kR8SNorm:
            return DXGI_FORMAT_R8_SNORM;
        case kR8UNorm:
            return DXGI_FORMAT_R8_UNORM;
        // rg8
        case kRG8Uint:
            return DXGI_FORMAT_R8G8_UINT;
        case kRG8Sint:
            return DXGI_FORMAT_R8G8_SINT;
        case kRG8SNorm:
            return DXGI_FORMAT_R8G8_SNORM;
        case kRG8UNorm:
            return DXGI_FORMAT_R8G8_UNORM;
        // r8g8b8
        case kRGBX8Uint:
            return DXGI_FORMAT_R8G8B8A8_UINT;
        case kRGBX8Sint:
            return DXGI_FORMAT_R8G8B8A8_SINT;
        case kRGBX8SNorm:
            return DXGI_FORMAT_R8G8B8A8_SNORM;
        case kRGBX8UNorm:
            return DXGI_FORMAT_R8G8B8A8_UNORM;
        case kRGBX8UNorm_sRGB:
            return DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
        // r8g8b8a8
        case kRGBA8Uint:
            return DXGI_FORMAT_R8G8B8A8_UINT;
        case kRGBA8Sint:
            return DXGI_FORMAT_R8G8B8A8_SINT;
        case kRGBA8SNorm:
            return DXGI_FORMAT_R8G8B8A8_SNORM;
        case kRGBA8UNorm:
            return DXGI_FORMAT_R8G8B8A8_UNORM;
        case kRGBA8UNorm_sRGB:
            return DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
        // b8g8r8a8
        case kBGRA8UNorm:
            return DXGI_FORMAT_B8G8R8A8_UNORM;
        case kBGRA8UNorm_sRGB:
            return DXGI_FORMAT_B8G8R8A8_UNORM_SRGB;
        // r16
        case kR16Uint:
            return DXGI_FORMAT_R16_UINT;
        case kR16Sint:
            return DXGI_FORMAT_R16_SINT;
        case kR16Float:
            return DXGI_FORMAT_R16_FLOAT;
        case kR16SNorm:
            return DXGI_FORMAT_R16_SNORM;
        case kR16UNorm:
            return DXGI_FORMAT_R16_UNORM;
        // r16g16
        case kRG16Uint:
            return DXGI_FORMAT_R16G16_UINT;
        case kRG16Sint:
            return DXGI_FORMAT_R16G16_SINT;
        case kRG16Float:
            return DXGI_FORMAT_R16G16_FLOAT;
        case kRG16SNorm:
            return DXGI_FORMAT_R16G16_SNORM;
        case kRG16UNorm:
            return DXGI_FORMAT_R16G16_UNORM;
        // r16g16b16a16
        case kRGBA16Uint:
            return DXGI_FORMAT_R16G16B16A16_UINT;
        case kRGBA16Sint:
            return DXGI_FORMAT_R16G16B16A16_SINT;
        case kRGBA16Float:
            return DXGI_FORMAT_R16G16B16A16_FLOAT;
        case kRGBA16SNorm:
            return DXGI_FORMAT_R16G16B16A16_SNORM;
        case kRGBA16UNorm:
            return DXGI_FORMAT_R16G16B16A16_UNORM;
        // r32
        case kR32Uint:
            return DXGI_FORMAT_R32_UINT;
        case kR32Sint:
            return DXGI_FORMAT_R32_SINT;
        case kR32Float:
            return DXGI_FORMAT_R32_FLOAT;
        // r32g32
        case kRG32Uint:
            return DXGI_FORMAT_R32G32_UINT;
        case kRG32Sint:
            return DXGI_FORMAT_R32G32_SINT;
        case kRG32Float:
            return DXGI_FORMAT_R32G32_FLOAT;
        // r32g32b32x32
        case kRGBX32Uint:
            return DXGI_FORMAT_R32G32B32_UINT;
        case kRGBX32Sint:
            return DXGI_FORMAT_R32G32B32_SINT;
        case kRGBX32Float:
            return DXGI_FORMAT_R32G32B32_FLOAT;
        // r32g32b32a32
        case kRGBA32Uint:
            return DXGI_FORMAT_R32G32B32A32_UINT;
        case kRGBA32Sint:
            return DXGI_FORMAT_R32G32B32A32_SINT;
        case kRGBA32Float:
            return DXGI_FORMAT_R32G32B32A32_FLOAT;
        // rbg10a2
        case kRGB10A2Uint:
            return DXGI_FORMAT_R10G10B10A2_UINT;
        case kRGB10A2UNorm:
            return DXGI_FORMAT_R10G10B10A2_UNORM;
        // r11g11b10
        case kRG11B10Float:
            return DXGI_FORMAT_R11G11B10_FLOAT;
        // rgb9e5
        case kRGB9E5Float:
            return DXGI_FORMAT_R9G9B9E5_SHAREDEXP;
        // bgr5a1
        case kBGR5A1Unorm:
            return DXGI_FORMAT_B5G5R5A1_UNORM;
        // b5g6r5
        case kB5G6R5Unorm:
            return DXGI_FORMAT_B5G6R5_UNORM;
        // b4g4r4a4
        case kBGRA4Unorm:
            return DXGI_FORMAT_B4G4R4A4_UNORM;
        // bc1-7
        case kBC1Unorm:
            return DXGI_FORMAT_BC1_UNORM;
        case kBC1Unorm_SRGB:
            return DXGI_FORMAT_BC1_UNORM_SRGB;
        case kBC2Unorm:
            return DXGI_FORMAT_BC2_UNORM;
        case kBC2Unorm_SRGB:
            return DXGI_FORMAT_BC2_UNORM_SRGB;
        case kBC3Unorm:
            return DXGI_FORMAT_BC3_UNORM;
        case kBC3Unorm_SRGB:
            return DXGI_FORMAT_BC3_UNORM_SRGB;
        case kBC4Unorm:
            return DXGI_FORMAT_BC4_UNORM;
        case kBC4Snorm:
            return DXGI_FORMAT_BC4_SNORM;
        case kBC5Unorm:
            return DXGI_FORMAT_BC5_UNORM;
        case kBC5Snorm:
            return DXGI_FORMAT_BC5_SNORM;
        case kBC6HUfloat:
            return DXGI_FORMAT_BC6H_UF16;
        case kBC6HSfloat:
            return DXGI_FORMAT_BC6H_SF16;
        case kBC7Unorm:
            return DXGI_FORMAT_BC7_UNORM;
        case kBC7Unorm_SRGB:
            return DXGI_FORMAT_BC7_UNORM_SRGB;
        // d24s8
        case kD24UnormS8Uint:
            return DXGI_FORMAT_D24_UNORM_S8_UINT;
        // d32
        case kD32Float:
            return DXGI_FORMAT_D32_FLOAT;
        // d32s8
        case kD32FloatS8Uint:
            return DXGI_FORMAT_D32_FLOAT_S8X24_UINT;
        default:
            break;
    }
    return DXGI_FORMAT_UNKNOWN;
}
