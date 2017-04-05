#pragma once
#include <stdint.h>

typedef struct Gfx Gfx; ///< Object representing a graphics device (ID3D11Device, vkDevice, MTLDevice)

/// @brief Creates a new graphics device
Gfx* gfxCreate(void);
/// @brief Terminates and destroys a graphics device
void gfxDestroy(Gfx* G);
