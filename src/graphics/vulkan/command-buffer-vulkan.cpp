#include "command-buffer-vulkan.h"
#include "graphics-vulkan.h"

#include <gsl/gsl>

namespace ak {

void CommandBufferVulkan::reset()
{
    VkResult result = _graphics->vkResetFences(_graphics->_device, 1, &_fence);
    assert(VK_SUCCEEDED(result) && "Could not reset fence");
    result = _graphics->vkResetCommandPool(_graphics->_device, _pool, 0);
    assert(VK_SUCCEEDED(result) && "Could not reset pool");
    result = _graphics->vkResetCommandBuffer(_buffer, 0);
    assert(VK_SUCCEEDED(result) && "Could not reset buffer");
    _open = false;
}

bool CommandBufferVulkan::begin_render_pass()
{
    if (_graphics->_swap_chain == VK_NULL_HANDLE) {
        return false;
    }
    auto const back_buffer_index = _graphics->get_back_buffer();
    constexpr VkClearValue clear_values[] = {
        {
            // color
            {
                {0.0f, 0.75f, 1.0f, 1.0f},
            },
            // depthStencil
            {
                0.0f,  // depth
                0x0,   // stencil
            },
        },
    };
    constexpr uint32_t num_clear_values = array_length(clear_values);

    auto const extent = _graphics->_surface_capabilities.currentExtent;
    VkRenderPassBeginInfo const render_pass_begin_info = {
        VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,     // sType
        nullptr,                                      // pNext
        _graphics->_render_pass,                      // renderPass
        _graphics->_framebuffers[back_buffer_index],  // framebuffer
        // renderArea
        {
            // offset
            {
                0,  // x
                0,  // y
            },
            extent,  // extent
        },
        num_clear_values,  // clearValueCount
        clear_values,      // pClearValues
    };
    _graphics->vkCmdBeginRenderPass(_buffer, &render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);

    VkRect2D const scissor = {
        {0, 0},  // offset
        extent,  // extent
    };
    _graphics->vkCmdSetScissor(_buffer, 0, 1, &scissor);

    VkViewport const viewport = {
        0, 0, static_cast<float>(extent.width), static_cast<float>(extent.height), 0.0f, 1.0f,
    };
    _graphics->vkCmdSetViewport(_buffer, 0, 1, &viewport);

    return true;
}

void CommandBufferVulkan::set_constant_data(void const* const upload_data, size_t size)
{
    if (!_current_render_state) {
        return;
    }
    auto const upload_offset = static_cast<VkDeviceSize>(static_cast<uint8_t const*>(upload_data) -
                                                         _graphics->_upload_start);
    VkDescriptorBufferInfo const buffer_info = {
        _graphics->_upload_buffer->_buffer,  // buffer
        upload_offset,                       // offset
        size,                                // range
    };

    VkWriteDescriptorSet const set_info = {
        VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,  // sType
        nullptr,                                 // pNext
        VK_NULL_HANDLE,                          // dstSet
        0,                                       // dstBinding
        0,                                       // dstArrayElement
        1,                                       // descriptorCount
        VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,       // descriptorType
        nullptr,                                 // pImageInfo
        &buffer_info,                            // pBufferInfo
        nullptr,                                 // pTexelBufferView
    };
    _graphics->vkCmdPushDescriptorSetKHR(_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                                         _current_render_state->_pipeline_layout, 0, 1, &set_info);
}

void CommandBufferVulkan::set_render_state(RenderState* const state)
{
    auto* const vulkan_state = static_cast<RenderStateVulkan*>(state);
    _graphics->vkCmdBindPipeline(_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vulkan_state->_pso);
    _current_render_state = vulkan_state;
}

void CommandBufferVulkan::set_vertex_buffer(Buffer* const buffer)
{
    if (!buffer) {
        return;
    }

    auto* const vulkan_buffer = static_cast<BufferVulkan*>(buffer);
    VkDeviceSize const offset = 0;
    _graphics->vkCmdBindVertexBuffers(_buffer, 0, 1, &vulkan_buffer->_buffer, &offset);
}

void CommandBufferVulkan::set_index_buffer(Buffer* const buffer)
{
    if (!buffer) {
        return;
    }
    auto* const vulkan_buffer = static_cast<BufferVulkan*>(buffer);
    VkDeviceSize const offset = 0;
    _graphics->vkCmdBindIndexBuffer(_buffer, vulkan_buffer->_buffer, 0, VK_INDEX_TYPE_UINT16);
}

void CommandBufferVulkan::draw(uint32_t const vertex_count)
{
    _graphics->vkCmdDraw(_buffer, vertex_count, 1, 0, 0);
}

void CommandBufferVulkan::draw_indexed(uint32_t const index_count)
{
    _graphics->vkCmdDrawIndexed(_buffer, index_count, 1, 0, 0, 0);
}

void CommandBufferVulkan::end_render_pass()
{
    _graphics->vkCmdEndRenderPass(_buffer);
}

}  // namespace ak
