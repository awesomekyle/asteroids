#include "gfx-metal-helper.h"
#import <Metal/Metal.h>
#include <TargetConditionals.h>
#import <Metal/MTLPixelFormat.h>

MTLPixelFormat MetalFormatFromGfxFormat(GfxFormat format)
{
    switch (format) {
        case kUnknown:
            return MTLPixelFormatInvalid;
        // r8
        case kR8Uint:
            return  MTLPixelFormatR8Uint;
        case kR8Sint:
            return  MTLPixelFormatR8Sint;
        case kR8SNorm:
            return  MTLPixelFormatR8Snorm;
        case kR8UNorm:
            return  MTLPixelFormatR8Unorm;
        // rg8
        case kRG8Uint:
            return  MTLPixelFormatRG8Uint;
        case kRG8Sint:
            return  MTLPixelFormatRG8Sint;
        case kRG8SNorm:
            return  MTLPixelFormatRG8Snorm;
        case kRG8UNorm:
            return  MTLPixelFormatRG8Unorm;
        // r8g8b8
        case kRGBX8Uint:
            return  MTLPixelFormatInvalid;
        case kRGBX8Sint:
            return  MTLPixelFormatInvalid;
        case kRGBX8SNorm:
            return  MTLPixelFormatInvalid;
        case kRGBX8UNorm:
            return  MTLPixelFormatInvalid;
        case kRGBX8UNorm_sRGB:
            return  MTLPixelFormatInvalid;
        // r8g8b8a8
        case kRGBA8Uint:
            return  MTLPixelFormatRGBA8Uint;
        case kRGBA8Sint:
            return  MTLPixelFormatRGBA8Sint;
        case kRGBA8SNorm:
            return  MTLPixelFormatRGBA8Snorm;
        case kRGBA8UNorm:
            return  MTLPixelFormatRGBA8Unorm;
        case kRGBA8UNorm_sRGB:
            return  MTLPixelFormatRGBA8Unorm_sRGB;
        // b8g8r8a8
        case kBGRA8UNorm:
            return  MTLPixelFormatBGRA8Unorm;
        case kBGRA8UNorm_sRGB:
            return  MTLPixelFormatBGRA8Unorm_sRGB;
        // r16
        case kR16Uint:
            return  MTLPixelFormatR16Uint;
        case kR16Sint:
            return  MTLPixelFormatR16Sint;
        case kR16Float:
            return  MTLPixelFormatR16Float;
        case kR16SNorm:
            return  MTLPixelFormatR16Snorm;
        case kR16UNorm:
            return  MTLPixelFormatR16Unorm;
        // r16g16
        case kRG16Uint:
            return  MTLPixelFormatRG16Uint;
        case kRG16Sint:
            return  MTLPixelFormatRG16Sint;
        case kRG16Float:
            return  MTLPixelFormatRG16Float;
        case kRG16SNorm:
            return  MTLPixelFormatRG16Snorm;
        case kRG16UNorm:
            return  MTLPixelFormatRG16Unorm;
        // r16g16b16a16
        case kRGBA16Uint:
            return  MTLPixelFormatRGBA16Uint;
        case kRGBA16Sint:
            return  MTLPixelFormatRGBA16Sint;
        case kRGBA16Float:
            return  MTLPixelFormatRGBA16Float;
        case kRGBA16SNorm:
            return  MTLPixelFormatRGBA16Snorm;
        case kRGBA16UNorm:
            return  MTLPixelFormatRGBA16Unorm;
        // r32
        case kR32Uint:
            return  MTLPixelFormatR32Uint;
        case kR32Sint:
            return  MTLPixelFormatR32Sint;
        case kR32Float:
            return  MTLPixelFormatR32Float;
        // r32g32
        case kRG32Uint:
            return  MTLPixelFormatRG32Uint;
        case kRG32Sint:
            return  MTLPixelFormatRG32Sint;
        case kRG32Float:
            return  MTLPixelFormatRG32Float;
        // r32g32b32x32
        case kRGBX32Uint:
            return  MTLPixelFormatInvalid;
        case kRGBX32Sint:
            return  MTLPixelFormatInvalid;
        case kRGBX32Float:
            return  MTLPixelFormatInvalid;
        // r32g32b32a32
        case kRGBA32Uint:
            return  MTLPixelFormatRGBA32Uint;
        case kRGBA32Sint:
            return  MTLPixelFormatRGBA32Sint;
        case kRGBA32Float:
            return  MTLPixelFormatRGBA32Float;
        // rbg10a2
        case kRGB10A2Uint:
            return  MTLPixelFormatRGB10A2Uint;
        case kRGB10A2UNorm:
            return  MTLPixelFormatRGB10A2Unorm;
        // r11g11b10
        case kRG11B10Float:
            return  MTLPixelFormatRG11B10Float;
        // rgb9e5
        case kRGB9E5Float:
            return  MTLPixelFormatRGB9E5Float;

#if TARGET_OS_IPHONE || TARGET_OS_SIMULATOR
        // bgr5a1
        case kBGR5A1Unorm:
            return  MTLPixelFormatA1BGR5Unorm;
        // b5g6r5
        case kB5G6R5Unorm:
            return  MTLPixelFormatB5G6R5Unorm;
        // b4g4r4a4
        case kBGRA4Unorm:
            return  MTLPixelFormatABGR4Unorm;
#else
        // bgr5a1
        case kBGR5A1Unorm:
            return  MTLPixelFormatR16Uint;
        // b5g6r5
        case kB5G6R5Unorm:
            return  MTLPixelFormatR16Uint;
        // b4g4r4a4
        case kBGRA4Unorm:
            return  MTLPixelFormatR16Uint;
#endif

        // bc1-7
        case kBC1Unorm:
            return  MTLPixelFormatBC1_RGBA;
        case kBC1Unorm_SRGB:
            return  MTLPixelFormatBC1_RGBA_sRGB;
        case kBC2Unorm:
            return  MTLPixelFormatBC2_RGBA;
        case kBC2Unorm_SRGB:
            return  MTLPixelFormatBC2_RGBA_sRGB;
        case kBC3Unorm:
            return  MTLPixelFormatBC3_RGBA;
        case kBC3Unorm_SRGB:
            return  MTLPixelFormatBC3_RGBA_sRGB;
        case kBC4Unorm:
            return  MTLPixelFormatBC4_RUnorm;
        case kBC4Snorm:
            return  MTLPixelFormatBC4_RSnorm;
        case kBC5Unorm:
            return  MTLPixelFormatBC5_RGUnorm;
        case kBC5Snorm:
            return  MTLPixelFormatBC5_RGSnorm;
        case kBC6HUfloat:
            return  MTLPixelFormatBC6H_RGBUfloat;
        case kBC6HSfloat:
            return  MTLPixelFormatBC6H_RGBFloat;
        case kBC7Unorm:
            return  MTLPixelFormatBC7_RGBAUnorm;
        case kBC7Unorm_SRGB:
            return  MTLPixelFormatBC7_RGBAUnorm_sRGB;
        // d24s8
        case kD24UnormS8Uint:
            return  MTLPixelFormatDepth24Unorm_Stencil8;
        // d32
        case kD32Float:
            return  MTLPixelFormatDepth32Float;
        // d32s8
        case kD32FloatS8Uint:
            return  MTLPixelFormatDepth32Float_Stencil8;
        default:
            break;
    }
    return MTLPixelFormatInvalid;
}
