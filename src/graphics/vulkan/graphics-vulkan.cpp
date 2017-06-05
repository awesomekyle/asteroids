#include "graphics-vulkan.h"
#include "graphics/graphics.h"

#include <cinttypes>
#include <gsl/gsl>
#include <utility>
#include <Windows.h>

#include "vulkan-debug.h"

#define UNUSED(v) ((void)(v))

namespace {

///
/// Constants
///
constexpr char const* kDesiredExtensions[] = {
    VK_KHR_SURFACE_EXTENSION_NAME, VK_KHR_PUSH_DESCRIPTOR_EXTENSION_NAME,
#if defined(VK_USE_PLATFORM_WIN32_KHR)
    VK_KHR_WIN32_SURFACE_EXTENSION_NAME,
#endif
#if defined(_DEBUG)
    VK_EXT_DEBUG_REPORT_EXTENSION_NAME,
#endif
};
constexpr size_t kNumDesiredExtensions = array_length(kDesiredExtensions);  // NOLINT

constexpr char const* kValidationLayers[] = {
    "VK_LAYER_LUNARG_standard_validation",
};
constexpr size_t kNumValidationLayers =
#if defined(_DEBUG)
    array_length(kValidationLayers);
#else
    0;
#endif

constexpr VkImageSubresourceRange kFullSubresourceRange = {
    VK_IMAGE_ASPECT_COLOR_BIT,  // aspectMask
    0,                          // baseMipLevel
    1,                          // levelCount
    0,                          // baseArrayLayer
    1,                          // layerCount
};

}  // anonymous namespace

namespace ak {

GraphicsVulkan::GraphicsVulkan()
{
    auto const library = LoadLibraryW(L"vulkan-1.dll");  // NOLINT
    Ensures(library);
    vkGetInstanceProcAddr = reinterpret_cast<PFN_vkGetInstanceProcAddr>(
        GetProcAddress(library, "vkGetInstanceProcAddr"));
    Ensures(vkGetInstanceProcAddr);

// Load global functions
#define VK_GLOBAL_FUNCTION(fn) fn = reinterpret_cast<PFN_##fn>(vkGetInstanceProcAddr(nullptr, #fn));
#include "vulkan-global-method-list.inl"
#undef VK_GLOBAL_FUNCTION

    get_extensions();
    create_instance();
    create_debug_callback();
    get_physical_devices();
    select_physical_device();
    create_device();
    create_render_passes();

    vkGetDeviceQueue(_device, _queue_index, 0, &_render_queue);

    create_command_buffers();
    create_upload_buffer();
}

GraphicsVulkan::~GraphicsVulkan()
{
    vkDeviceWaitIdle(_device);
    _upload_buffer.reset();
    for (auto const& buffer : _command_buffers) {
        vkDestroyCommandPool(_device, buffer._pool, _vk_allocator);
        vkDestroyFence(_device, buffer._fence, _vk_allocator);
    }
    for (auto const& back_buffer : _back_buffer_views) {
        vkDestroyImageView(_device, back_buffer, _vk_allocator);
    }
    for (auto const& framebuffer : _framebuffers) {
        vkDestroyFramebuffer(_device, framebuffer, _vk_allocator);
    }
    vkDestroyImageView(_device, _depth_view, _vk_allocator);
    vkFreeMemory(_device, _depth_buffer_memory, _vk_allocator);
    vkDestroyImage(_device, _depth_buffer, _vk_allocator);
    vkDestroyRenderPass(_device, _render_pass, _vk_allocator);
    vkDestroySwapchainKHR(_device, _swap_chain, _vk_allocator);
    vkDestroySemaphore(_device, _swap_chain_semaphore, _vk_allocator);
    vkDestroySurfaceKHR(_instance, _surface, _vk_allocator);
    vkDestroyDevice(_device, _vk_allocator);
#if defined(_DEBUG)
    vkDestroyDebugReportCallbackEXT(_instance, _debug_report, _vk_allocator);
#endif  // _DEBUG
    vkDestroyInstance(_instance, _vk_allocator);
}

Graphics::API GraphicsVulkan::api_type() const
{
    return kVulkan;
}

bool GraphicsVulkan::create_swap_chain(void* window, void* application)  // NOLINT
{
#if defined(VK_USE_PLATFORM_WIN32_KHR)
    auto hwnd = static_cast<HWND>(window);
    auto hinstance = static_cast<HINSTANCE>(application);
    VkWin32SurfaceCreateInfoKHR const surface_create_info = {
        VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR,  // sType
        nullptr,                                          // pNext
        0,                                                // flags
        hinstance,                                        // hinstance
        hwnd,                                             // hwnd
    };

    VkResult result = vkCreateWin32SurfaceKHR(_instance, &surface_create_info, nullptr, &_surface);
    assert(VK_SUCCEEDED(result) && "Could not create surface");

    // Check that the queue supports present
    VkBool32 present_supported = VK_FALSE;
    vkGetPhysicalDeviceSurfaceSupportKHR(_physical_device, _queue_index, _surface,
                                         &present_supported);
    assert(present_supported && "Present not supported on selected queue");

    // create semaphore
    VkSemaphoreCreateInfo const semaphore_info = {
        VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,  // sType
        nullptr,                                  // pNext
        0                                         // flags
    };
    result = vkCreateSemaphore(_device, &semaphore_info, nullptr, &_swap_chain_semaphore);
    assert(VK_SUCCEEDED(result) && "Could not create semaphore");

    // Get surface capabilities
    result = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(_physical_device, _surface,
                                                       &_surface_capabilities);
    assert(VK_SUCCEEDED(result) && "Could not get surface capabilities");

    // Get supported formats
    uint32_t num_formats = 0;
    std::vector<VkSurfaceFormatKHR> surface_formats;
    do {
        result =
            vkGetPhysicalDeviceSurfaceFormatsKHR(_physical_device, _surface, &num_formats, nullptr);
        if (result == VK_SUCCESS && num_formats) {
            surface_formats.resize(num_formats);
            result = vkGetPhysicalDeviceSurfaceFormatsKHR(_physical_device, _surface, &num_formats,
                                                          surface_formats.data());
            assert(VK_SUCCEEDED(result));
        }
    } while (result == VK_INCOMPLETE);

    // Get supported present modes
    uint32_t num_present_modes = 0;
    std::vector<VkPresentModeKHR> present_modes;
    do {
        result = vkGetPhysicalDeviceSurfacePresentModesKHR(_physical_device, _surface,
                                                           &num_present_modes, nullptr);
        if (result == VK_SUCCESS && num_present_modes) {
            present_modes.resize(num_present_modes);
            result =
                vkGetPhysicalDeviceSurfacePresentModesKHR(_physical_device, _surface,
                                                          &num_present_modes, present_modes.data());
            assert(VK_SUCCEEDED(result));
        }
    } while (result == VK_INCOMPLETE);

    // select present mode
    constexpr VkPresentModeKHR desired_modes[] = {
        // TODO(kw): Only use IMMEDIATE for profiling
        VK_PRESENT_MODE_MAILBOX_KHR, VK_PRESENT_MODE_IMMEDIATE_KHR, VK_PRESENT_MODE_FIFO_KHR,
    };
    for (auto desired : desired_modes) {
        if (std::find(present_modes.begin(), present_modes.end(), desired) != present_modes.end()) {
            _present_mode = desired;
            break;
        }
    }

    // select surface format
    for (auto const& format : surface_formats) {
        if (format.format == VK_FORMAT_B8G8R8A8_SRGB) {  // TODO(kw): support other formats
            _surface_format = format;
            break;
        }
    }

    assert(_surface_format.format != VK_FORMAT_UNDEFINED);
    assert(_surface_capabilities.supportedUsageFlags & VK_IMAGE_USAGE_TRANSFER_DST_BIT &&
           "Cannot clear surface");

    return resize(0, 0);
#else
#warning "No swap chain implementation"
    return false;
#endif
}

bool GraphicsVulkan::resize(int /*width*/, int /*height*/)
{
    if (_surface == VK_NULL_HANDLE) {
        return false;
    }
    vkDeviceWaitIdle(_device);
    VkResult result = VK_SUCCESS;

    // Get surface capabilities
    result = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(_physical_device, _surface,
                                                       &_surface_capabilities);
    assert(VK_SUCCEEDED(result) && "Could not get surface capabilities");

    VkImageUsageFlags const image_usage_flags =
        VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
        VK_IMAGE_USAGE_TRANSFER_DST_BIT;  // TODO(kw): check support
    uint32_t const num_images = _surface_capabilities.minImageCount + 1;

    // Create swap chain
    VkSwapchainCreateInfoKHR const swap_chain_info = {
        VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,  // sType
        nullptr,                                      // pNext
        0,                                            // flags
        _surface,                                     // surface
        num_images,                                   // minImageCount
        _surface_format.format,                       // imageFormat
        _surface_format.colorSpace,                   // imageColorSpace
        _surface_capabilities.currentExtent,          // imageExtent
        1,                                            // imageArrayLayers
        image_usage_flags,                            // imageUsage
        VK_SHARING_MODE_EXCLUSIVE,                    // imageSharingMode
        0,                                            // queueFamilyIndexCount
        nullptr,                                      // pQueueFamilyIndices
        _surface_capabilities.currentTransform,       // preTransform
        VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,            // compositeAlpha
        _present_mode,                                // presentMode
        VK_TRUE,                                      // clipped
        _swap_chain,                                  // oldSwapchain
    };
    VkSwapchainKHR new_swap_chain = VK_NULL_HANDLE;
    result = vkCreateSwapchainKHR(_device, &swap_chain_info, nullptr, &new_swap_chain);
    assert(VK_SUCCEEDED(result) && "Could not create swap chain");
    if (_swap_chain != VK_NULL_HANDLE) {
        vkDestroySwapchainKHR(_device, _swap_chain, nullptr);
    }
    _swap_chain = new_swap_chain;

    // Get images
    result = vkGetSwapchainImagesKHR(_device, _swap_chain, &_num_back_buffers, nullptr);
    assert(VK_SUCCEEDED(result));
    _num_back_buffers = std::min(_num_back_buffers, array_length(_back_buffers));
    result = vkGetSwapchainImagesKHR(_device, _swap_chain, &_num_back_buffers,
                                     gsl::make_span(_back_buffers).data());
    assert(VK_SUCCEEDED(result));

    // Create image views & framebuffers
    for (uint32_t ii = 0; ii < _num_back_buffers; ++ii) {
        // Clear originals
        vkDestroyFramebuffer(_device, gsl::at(_framebuffers, ii), nullptr);
        vkDestroyImageView(_device, gsl::at(_back_buffer_views, ii), nullptr);
        // Image view
        VkImageViewCreateInfo const image_view_info = {
            VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,  // sType
            nullptr,                                   // pNext
            0,                                         // flags
            gsl::at(_back_buffers, ii),                // image
            VK_IMAGE_VIEW_TYPE_2D,                     // viewType
            _surface_format.format,                    // format
            // components
            {
                VK_COMPONENT_SWIZZLE_IDENTITY,  // r
                VK_COMPONENT_SWIZZLE_IDENTITY,  // g
                VK_COMPONENT_SWIZZLE_IDENTITY,  // b
                VK_COMPONENT_SWIZZLE_IDENTITY,  // a
            },
            kFullSubresourceRange,  // subresourceRange
        };
        result =
            vkCreateImageView(_device, &image_view_info, nullptr, &gsl::at(_back_buffer_views, ii));
        assert(VK_SUCCEEDED(result) && "Could not create image view");

        // Framebuffer
        VkFramebufferCreateInfo const framebuffer_info = {
            VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,   // sType
            nullptr,                                     // pNext
            0,                                           // flags
            _render_pass,                                // renderPass
            1,                                           // attachmentCount
            &gsl::at(_back_buffer_views, ii),            // pAttachments
            _surface_capabilities.currentExtent.width,   // width
            _surface_capabilities.currentExtent.height,  // height
            1,                                           // layers
        };
        result =
            vkCreateFramebuffer(_device, &framebuffer_info, nullptr, &gsl::at(_framebuffers, ii));
        assert(VK_SUCCEEDED(result) && "Could not create framebuffer");
    }
    return true;
}

bool GraphicsVulkan::present()
{
    if (_swap_chain == VK_NULL_HANDLE) {
        return false;
    }
    uint32_t const curr_index = get_back_buffer();

    // Present
    VkPresentInfoKHR const presentInfo = {
        VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,  // sType
        nullptr,                             // pNext
        1,                                   // waitSemaphoreCount
        &_swap_chain_semaphore,              // pWaitSemaphores
        1,                                   // swapchainCount
        &_swap_chain,                        // pSwapchains
        &curr_index,                         // pImageIndices
        nullptr,                             // pResults
    };
    VkResult const result = vkQueuePresentKHR(_render_queue, &presentInfo);
    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
        resize(0, 0);
    }
    _back_buffer_index = UINT32_MAX;
    if (result != VK_ERROR_VALIDATION_FAILED_EXT) {
        assert(VK_SUCCEEDED(result) && "Could not get swap chain image index");
    }

    return true;
}

CommandBuffer* GraphicsVulkan::command_buffer()
{
    uint_fast32_t const currIndex = _current_command_buffer.fetch_add(1) % kMaxCommandBuffers;
    auto& buffer = gsl::at(_command_buffers, currIndex);
    if (vkGetFenceStatus(_device, buffer._fence) != VK_SUCCESS || buffer._open) {
        /// @TODO: This case needs to be handled
        return nullptr;
    }
    buffer.reset();

    constexpr VkCommandBufferBeginInfo const beginInfo = {
        VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,  // sType
        nullptr,                                      // pNext
        VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,  // flags
        nullptr,                                      // pInheritanceInfo
    };
    auto const result = vkBeginCommandBuffer(buffer._buffer, &beginInfo);
    assert(VK_SUCCEEDED(result) && "Could not begin buffer");

    buffer._open = true;
    return &buffer;
}
int GraphicsVulkan::num_available_command_buffers()
{
    int available_command_buffers = 0;
    for (auto const& buffer : _command_buffers) {
        if (!buffer._open) {
            available_command_buffers++;
        }
    }
    return available_command_buffers;
}
bool GraphicsVulkan::execute(CommandBuffer* command_buffer)
{
    Expects(command_buffer);
    Expects(_device);
    auto* const vk_buffer = static_cast<CommandBufferVulkan*>(command_buffer);
    VkResult result = vkEndCommandBuffer(vk_buffer->_buffer);
    assert(VK_SUCCEEDED(result) && "Could not end command buffer");
    VkSubmitInfo const submit_info = {
        VK_STRUCTURE_TYPE_SUBMIT_INFO,  // sType
        nullptr,                        // pNext
        0,                              // waitSemaphoreCount
        nullptr,                        // pWaitSemaphores
        nullptr,                        // pWaitDstStageMask
        1,                              // commandBufferCount
        &vk_buffer->_buffer,            // pCommandBuffers
        0,                              // signalSemaphoreCount
        nullptr,                        // pSignalSemaphores
    };
    result = vkQueueSubmit(_render_queue, 1, &submit_info, vk_buffer->_fence);
    assert(VK_SUCCEEDED(result) && "Could not submit command buffer");
    vk_buffer->_open = false;
    return true;
}

void GraphicsVulkan::wait_for_idle()
{
    vkDeviceWaitIdle(_device);
}

std::unique_ptr<RenderState> GraphicsVulkan::create_render_state(RenderStateDesc const& desc)
{
    auto state = std::make_unique<RenderStateVulkan>();
    state->_graphics = this;

    // Create shader modules
    VkShaderModuleCreateInfo const vs_module_info = {
        VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,                     // sType
        nullptr,                                                         // pNext
        0,                                                               // flags
        desc.vertex_shader.size,                                         // codeSize
        reinterpret_cast<uint32_t const*>(desc.vertex_shader.bytecode),  // pCode
    };
    VkResult result =
        vkCreateShaderModule(_device, &vs_module_info, _vk_allocator, &state->_vs_module);
    assert(VK_SUCCEEDED(result));

    VkShaderModuleCreateInfo const ps_module_info = {
        VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,                    // sType
        nullptr,                                                        // pNext
        0,                                                              // flags
        desc.pixel_shader.size,                                         // codeSize
        reinterpret_cast<uint32_t const*>(desc.pixel_shader.bytecode),  // pCode
    };
    result = vkCreateShaderModule(_device, &ps_module_info, _vk_allocator, &state->_ps_module);
    assert(VK_SUCCEEDED(result));

    // pipeline layout
    VkDescriptorSetLayoutBinding const layout_bindings[] = {
        {
            0,                                  // binding
            VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,  // descriptorType
            4,                                  // descriptorCount
            VK_SHADER_STAGE_VERTEX_BIT,         // stageFlags
            nullptr,                            // pImmutableSamplers
        },
        {
            1,                                  // binding
            VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,  // descriptorType
            4,                                  // descriptorCount
            VK_SHADER_STAGE_FRAGMENT_BIT,       // stageFlags
            nullptr,                            // pImmutableSamplers
        },
    };
    VkDescriptorSetLayoutCreateInfo const descriptor_set_info = {
        VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,      // sType
        nullptr,                                                  // pNext
        VK_DESCRIPTOR_SET_LAYOUT_CREATE_PUSH_DESCRIPTOR_BIT_KHR,  // flags
        array_length(layout_bindings),                            // bindingCount
        layout_bindings,                                          // pBindings
    };
    result = vkCreateDescriptorSetLayout(_device, &descriptor_set_info, _vk_allocator,
                                         &state->_desc_set_layout);
    assert(VK_SUCCEEDED(result));

    VkPipelineLayoutCreateInfo const layout_info = {
        VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,  // sType
        nullptr,                                        // pNext
        0,                                              // flags
        1,                                              // setLayoutCount
        &state->_desc_set_layout,                       // pSetLayouts
        0,                                              // pushConstantRangeCount
        nullptr                                         // pPushConstantRanges
    };
    result = vkCreatePipelineLayout(_device, &layout_info, _vk_allocator, &state->_pipeline_layout);
    assert(VK_SUCCEEDED(result));

    // shader stages
    VkPipelineShaderStageCreateInfo const stage_info[] = {
        // vertex shader
        {
            VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,  // sType
            nullptr,                                              // pNext
            0,                                                    // flags
            VK_SHADER_STAGE_VERTEX_BIT,                           // stage
            state->_vs_module,                                    // module
            "main",                                               // pName
            nullptr                                               // pSpecializationInfo
        },
        // pragment shader
        {
            VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,  // sType
            nullptr,                                              // pNext
            0,                                                    // flags
            VK_SHADER_STAGE_FRAGMENT_BIT,                         // stage
            state->_ps_module,                                    // module
            "main",                                               // pName
            nullptr                                               // pSpecializationInfo
        },
    };

    // input layout
    std::vector<VkVertexInputAttributeDescription> attribute_desc;
    auto const* layout = desc.input_layout;
    uint32_t current_offset = 0;
    while (layout && layout->name) {
        VkFormat format = VK_FORMAT_UNDEFINED;
        switch (layout->num_floats) {
            case 1:
                format = VK_FORMAT_R32_SFLOAT;
                break;
            case 2:
                format = VK_FORMAT_R32G32_SFLOAT;
                break;
            case 3:
                format = VK_FORMAT_R32G32B32_SFLOAT;
                break;
            case 4:
                format = VK_FORMAT_R32G32B32A32_SFLOAT;
                break;
            default:
                break;
        }
        attribute_desc.push_back({
            layout->slot,   // location
            0,              // binding
            format,         // format
            current_offset  // offset
        });
        current_offset += sizeof(float) * layout->num_floats;
        layout++;
    }

    VkVertexInputBindingDescription const vertex_binding_desc[] = {
        {
            0,                           // binding
            current_offset,              // stride
            VK_VERTEX_INPUT_RATE_VERTEX  // inputRate
        },
    };

    VkPipelineVertexInputStateCreateInfo const vertex_input_state_info = {
        VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,  // sType
        nullptr,                                                    // pNext
        0,                                                          // flags
        array_length(vertex_binding_desc),                          // vertexBindingDescriptionCount
        vertex_binding_desc,                                        // pVertexBindingDescriptions
        static_cast<uint32_t>(attribute_desc.size()),  // vertexAttributeDescriptionCount
        attribute_desc.data(),                         // pVertexAttributeDescriptions
    };

    // input assembler
    VkPipelineInputAssemblyStateCreateInfo const input_assembly_info = {
        VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,  // sType
        nullptr,                                                      // pNext
        0,                                                            // flags
        VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,                          // topology
        VK_FALSE                                                      // primitiveRestartEnable
    };

    // rasterizer
    VkPipelineRasterizationStateCreateInfo const rasterization_info = {
        VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,  // sType
        nullptr,                                                     // pNext
        0,                                                           // flags
        VK_FALSE,                                                    // depthClampEnable
        VK_FALSE,                                                    // rasterizerDiscardEnable
        VK_POLYGON_MODE_FILL,                                        // polygonMode
        VK_CULL_MODE_BACK_BIT,                                       // cullMode
        VK_FRONT_FACE_CLOCKWISE,                                     // frontFace
        VK_FALSE,                                                    // depthBiasEnable
        0.0f,                                                        // depthBiasConstantFactor
        0.0f,                                                        // depthBiasClamp
        0.0f,                                                        // depthBiasSlopeFactor
        1.0f                                                         // lineWidth
    };

    // MSAA
    VkPipelineMultisampleStateCreateInfo const msaa_info = {
        VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,  // sType
        nullptr,                                                   // pNext
        0,                                                         // flags
        VK_SAMPLE_COUNT_1_BIT,                                     // rasterizationSamples
        VK_FALSE,                                                  // sampleShadingEnable
        1.0f,                                                      // minSampleShading
        nullptr,                                                   // pSampleMask
        VK_FALSE,                                                  // alphaToCoverageEnable
        VK_FALSE                                                   // alphaToOneEnable
    };

    // blending
    VkPipelineColorBlendAttachmentState const color_blend_states[] = {
        {
            VK_FALSE,                                              // blendEnable
            VK_BLEND_FACTOR_ONE,                                   // srcColorBlendFactor
            VK_BLEND_FACTOR_ZERO,                                  // dstColorBlendFactor
            VK_BLEND_OP_ADD,                                       // colorBlendOp
            VK_BLEND_FACTOR_ONE,                                   // srcAlphaBlendFactor
            VK_BLEND_FACTOR_ZERO,                                  // dstAlphaBlendFactor
            VK_BLEND_OP_ADD,                                       // alphaBlendOp
            VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |  // colorWriteMask
                VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
        },
    };

    VkPipelineColorBlendStateCreateInfo const color_blend_state_info = {
        VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,  // sType
        nullptr,                                                   // pNext
        0,                                                         // flags
        VK_FALSE,                                                  // logicOpEnable
        VK_LOGIC_OP_COPY,                                          // logicOp
        array_length(color_blend_states),                          // attachmentCount
        color_blend_states,                                        // pAttachments
        {0.0f, 0.0f, 0.0f, 0.0f}                                   // blendConstants
    };

    VkPipelineViewportStateCreateInfo const viewport_info = {
        VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,  // sType
        nullptr,                                                // pNext
        0,                                                      // flags
        1,                                                      // viewportCount
        nullptr,                                                // pViewports
        1,                                                      // scissorCount
        nullptr                                                 // pScissors
    };

    // dynamic state
    VkDynamicState const dynamic_states[] = {
        VK_DYNAMIC_STATE_SCISSOR, VK_DYNAMIC_STATE_VIEWPORT,
    };
    VkPipelineDynamicStateCreateInfo const dynamic_info = {
        VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,  // sType
        nullptr,                                               // pNext
        0,                                                     // flags
        array_length(dynamic_states),                          // dynamicStateCount
        dynamic_states,                                        // pDynamicStates
    };

    // finally create the PSO
    VkGraphicsPipelineCreateInfo const pipeline_create_info = {
        VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,  // sType
        nullptr,                                          // pNext
        0,                                                // flags
        array_length(stage_info),                         // stageCount
        stage_info,                                       // pStages
        &vertex_input_state_info,                         // pVertexInputState;
        &input_assembly_info,                             // pInputAssemblyState
        nullptr,                                          // pTessellationState
        &viewport_info,                                   // pViewportState
        &rasterization_info,                              // pRasterizationState
        &msaa_info,                                       // pMultisampleState
        nullptr,                                          // pDepthStencilState
        &color_blend_state_info,                          // pColorBlendState
        &dynamic_info,                                    // pDynamicState
        state->_pipeline_layout,                          // layout
        _render_pass,                                     // renderPass
        0,                                                // subpass
        VK_NULL_HANDLE,                                   // basePipelineHandle
        -1                                                // basePipelineIndex
    };
    result = vkCreateGraphicsPipelines(_device, VK_NULL_HANDLE, 1, &pipeline_create_info,
                                       _vk_allocator, &state->_pso);
    assert(VK_SUCCEEDED(result));

    return state;
}  // namespace ak

std::unique_ptr<Buffer> GraphicsVulkan::create_vertex_buffer(uint32_t const size,
                                                             void const* const data)
{
    auto vertex_buffer =
        create_buffer(size, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);

    // upload data
    void* gpu_data = nullptr;
    VkResult const result = vkMapMemory(_device, vertex_buffer->_memory, 0, size, 0, &gpu_data);
    assert(VK_SUCCEEDED(result) && gpu_data);

    memcpy(gpu_data, data, size);

    VkMappedMemoryRange const range = {
        VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE,  // sType
        nullptr,                                // pNext
        vertex_buffer->_memory,                 // memory
        0,                                      // offset
        VK_WHOLE_SIZE                           // size
    };
    vkFlushMappedMemoryRanges(_device, 1, &range);

    vkUnmapMemory(_device, vertex_buffer->_memory);

    return vertex_buffer;
}

std::unique_ptr<Buffer> GraphicsVulkan::create_index_buffer(uint32_t const size,
                                                            void const* const data)
{
    auto index_buffer =
        create_buffer(size, VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);

    // upload data
    void* gpu_data = nullptr;
    VkResult const result = vkMapMemory(_device, index_buffer->_memory, 0, size, 0, &gpu_data);
    assert(VK_SUCCEEDED(result) && gpu_data);

    memcpy(gpu_data, data, size);

    VkMappedMemoryRange const range = {
        VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE,  // sType
        nullptr,                                // pNext
        index_buffer->_memory,                  // memory
        0,                                      // offset
        VK_WHOLE_SIZE                           // size
    };
    vkFlushMappedMemoryRanges(_device, 1, &range);

    vkUnmapMemory(_device, index_buffer->_memory);

    return index_buffer;
}

void GraphicsVulkan::get_extensions()
{
    uint32_t num_extensions = 0;
    VkResult result = VK_SUCCESS;
    do {
        result = vkEnumerateInstanceExtensionProperties(nullptr, &num_extensions, nullptr);
        if (result == VK_SUCCESS && num_extensions != 0) {
            _available_extensions.resize(num_extensions);
            result = vkEnumerateInstanceExtensionProperties(nullptr, &num_extensions,
                                                            _available_extensions.data());
            assert(VK_SUCCEEDED(result));
        }
    } while (result == VK_INCOMPLETE);
}

void GraphicsVulkan::create_instance()
{
    // Check for available extensions
    std::vector<char const*> enabled_extensions;
    for (auto const& available_ext : _available_extensions) {
        for (auto* const desired_ext : kDesiredExtensions) {
            if (strncmp(available_ext.extensionName, desired_ext, 256) == 0) {
                enabled_extensions.push_back(available_ext.extensionName);
            }
        }
    }

    VkApplicationInfo const application_info = {
        VK_STRUCTURE_TYPE_APPLICATION_INFO,  // sType
        nullptr,                             // pNext
        "Asteroids",                         // pApplicationName
        0,                                   // applicationVersion
        "Vulkan Graphics",                   // pEngineName
        VK_MAKE_VERSION(0, 1, 0),            // engineVersion
        VK_MAKE_VERSION(1, 0, 0),            // apiVersion
    };
    VkInstanceCreateInfo const create_info = {
        VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,            // sType
        nullptr,                                           // pNext
        0,                                                 // flags
        &application_info,                                 // pApplicationInfo
        kNumValidationLayers,                              // enabledLayerCount
        gsl::make_span(kValidationLayers).data(),          // ppEnabledLayerNames
        static_cast<uint32_t>(enabled_extensions.size()),  // enabledExtensionCount
        enabled_extensions.data(),                         // ppEnabledExtensionNames
    };
    auto const result = vkCreateInstance(&create_info, _vk_allocator, &_instance);
    assert(VK_SUCCEEDED(result) && "Could not create instance");
    Ensures(_instance);

// Load instance methods
#define VK_INSTANCE_FUNCTION(fn) \
    fn = reinterpret_cast<PFN_##fn>(vkGetInstanceProcAddr(_instance, #fn));
#include "vulkan-instance-method-list.inl"
#undef VK_INSTANCE_FUNCTION
}

void GraphicsVulkan::create_debug_callback()
{
    Expects(_instance);
#if _DEBUG
    VkDebugReportFlagsEXT const debug_flags = VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT |
                                              // VK_DEBUG_REPORT_DEBUG_BIT_EXT |
                                              VK_DEBUG_REPORT_ERROR_BIT_EXT |
                                              // VK_DEBUG_REPORT_INFORMATION_BIT_EXT |
                                              VK_DEBUG_REPORT_WARNING_BIT_EXT;
    VkDebugReportCallbackCreateInfoEXT const debug_info = {
        VK_STRUCTURE_TYPE_DEBUG_REPORT_CREATE_INFO_EXT,  // sType
        nullptr,                                         // pNext
        debug_flags,                                     // flags
        &debug_callback,                                 // pfnCallback
        this                                             // pUserData
    };
    auto const result =
        vkCreateDebugReportCallbackEXT(_instance, &debug_info, _vk_allocator, &_debug_report);
    assert(VK_SUCCEEDED(result) && "Could not create debug features");
#endif
}

void GraphicsVulkan::get_physical_devices()
{
    uint32_t num_devices = 0;
    VkResult result = VK_SUCCESS;
    do {
        result = vkEnumeratePhysicalDevices(_instance, &num_devices, nullptr);
        if (result == VK_SUCCESS && num_devices != 0) {
            _all_physical_devices.resize(num_devices);
            result =
                vkEnumeratePhysicalDevices(_instance, &num_devices, _all_physical_devices.data());
            assert(VK_SUCCEEDED(result));
        }
    } while (result == VK_INCOMPLETE);
}

void GraphicsVulkan::select_physical_device()
{
    VkPhysicalDevice best_physical_device = VK_NULL_HANDLE;
    for (auto const& device : _all_physical_devices) {
        VkPhysicalDeviceProperties properties = {};
        vkGetPhysicalDeviceProperties(device, &properties);
        if (properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
            best_physical_device = device;
            break;
        }
    }
    assert(best_physical_device);

    // TODO(kw): Support multiple queue types
    uint32_t num_queues = 0;
    std::vector<VkQueueFamilyProperties> queue_properties;
    vkGetPhysicalDeviceQueueFamilyProperties(best_physical_device, &num_queues, nullptr);
    if (num_queues != 0) {
        queue_properties.resize(num_queues);
        vkGetPhysicalDeviceQueueFamilyProperties(best_physical_device, &num_queues,
                                                 queue_properties.data());
    }
    for (uint32_t ii = 0; ii < static_cast<uint32_t>(queue_properties.size()); ++ii) {
        if (queue_properties[ii].queueCount >= 1 &&
            queue_properties[ii].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            _queue_index = ii;
            break;
        }
    }
    assert(_queue_index != UINT32_MAX);

    _physical_device = best_physical_device;
}

void GraphicsVulkan::create_device()
{
    constexpr float queuePriorities[] = {1.0f};
    VkDeviceQueueCreateInfo const queue_info = {
        VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,  // sType
        nullptr,                                     // pNext
        0,                                           // flags
        _queue_index,                                // queueFamilyIndex
        1,                                           // queueCount
        gsl::make_span(queuePriorities).data(),      // pQueuePriorities
    };
    constexpr char const* known_extensions[] = {
        // TODO(kw): Check these are supported
        VK_KHR_SWAPCHAIN_EXTENSION_NAME, VK_KHR_PUSH_DESCRIPTOR_EXTENSION_NAME,
    };
    constexpr uint32_t const num_known_extensions = array_length(known_extensions);
    VkDeviceCreateInfo const device_info = {
        VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,     // sType
        nullptr,                                  // pNext
        0,                                        // flags
        1,                                        // queueCreateInfoCount
        &queue_info,                              // pQueueCreateInfos
        0,                                        // enabledLayerCount
        nullptr,                                  // ppEnabledLayerNames
        num_known_extensions,                     // enabledExtensionCount
        gsl::make_span(known_extensions).data(),  // ppEnabledExtensionNames
        nullptr,                                  // pEnabledFeatures
    };
    VkResult const result = vkCreateDevice(_physical_device, &device_info, nullptr, &_device);
    assert(VK_SUCCEEDED(result) && "Could not create device");

// Load device methods
#define VK_DEVICE_FUNCTION(fn) \
    fn = reinterpret_cast<PFN_##fn>(vkGetInstanceProcAddr(_instance, #fn));
#include "vulkan-device-method-list.inl"
#undef VK_DEVICE_FUNCTION
}

void GraphicsVulkan::create_render_passes()
{
    constexpr VkAttachmentDescription attachments[] = {
        {
            0,                                 // flags
            VK_FORMAT_B8G8R8A8_SRGB,           // format // Wait until we get this from the surface?
            VK_SAMPLE_COUNT_1_BIT,             // samples
            VK_ATTACHMENT_LOAD_OP_CLEAR,       // loadOp
            VK_ATTACHMENT_STORE_OP_STORE,      // storeOp
            VK_ATTACHMENT_LOAD_OP_DONT_CARE,   // stencilLoadOp
            VK_ATTACHMENT_STORE_OP_DONT_CARE,  // stencilStoreOp
            VK_IMAGE_LAYOUT_UNDEFINED,         // initialLayout
            VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,   // finalLayout
        },
    };
    constexpr uint32_t const num_attachments = array_length(attachments);

    constexpr VkAttachmentReference const color_attachment_references[] = {
        {
            0,                                         // attachment
            VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,  // layout
        },
    };
    constexpr uint32_t const num_color_references = array_length(color_attachment_references);

    VkSubpassDescription const subpasses[] = {
        {
            0,                                                   // flags
            VK_PIPELINE_BIND_POINT_GRAPHICS,                     // pipelineBindPoint
            0,                                                   // inputAttachmentCount
            nullptr,                                             // pInputAttachments
            num_color_references,                                // colorAttachmentCount
            gsl::make_span(color_attachment_references).data(),  // pColorAttachments
            nullptr,                                             // pResolveAttachments
            nullptr,                                             // pDepthStencilAttachment
            0,                                                   // preserveAttachmentCount
            nullptr,                                             // pPreserveAttachments
        },
    };
    constexpr uint32_t const num_subpasses = array_length(subpasses);

    VkRenderPassCreateInfo const render_pass_info = {
        VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,  // sType
        nullptr,                                    // pNext
        0,                                          // flags
        num_attachments,                            // attachmentCount
        gsl::make_span(attachments).data(),         // pAttachments
        num_subpasses,                              // subpassCount
        gsl::make_span(subpasses).data(),           // pSubpasses
        0,                                          // dependencyCount
        nullptr,                                    // pDependencies
    };

    VkResult const result = vkCreateRenderPass(_device, &render_pass_info, nullptr, &_render_pass);
    assert(VK_SUCCEEDED(result) && "Could not create render pass");
}

void GraphicsVulkan::create_command_buffers()
{
    for (auto& buffer : _command_buffers) {
        VkCommandPoolCreateInfo const pool_info = {
            VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,       // sType
            nullptr,                                          // pNext
            VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,  // flags
            _queue_index,                                     // queueFamilyIndex
        };
        VkResult result = vkCreateCommandPool(_device, &pool_info, nullptr, &buffer._pool);
        assert(VK_SUCCEEDED(result));
        VkCommandBufferAllocateInfo const buffer_info = {
            VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,  // sType
            nullptr,                                         // pNext
            buffer._pool,                                    // commandPool
            VK_COMMAND_BUFFER_LEVEL_PRIMARY,                 // level
            1,                                               // commandBufferCount
        };
        result = vkAllocateCommandBuffers(_device, &buffer_info, &buffer._buffer);
        assert(VK_SUCCEEDED(result));

        // Fence
        VkFenceCreateInfo const fence_info = {
            VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,  // sType
            nullptr,                              // pNext
            VK_FENCE_CREATE_SIGNALED_BIT,         // flags
        };
        result = vkCreateFence(_device, &fence_info, nullptr, &buffer._fence);
        assert(VK_SUCCEEDED(result));

        buffer._graphics = this;
    }
}

void GraphicsVulkan::create_depth_buffer()
{
    // destroy existing depth buffer
    vkDestroyImageView(_device, _depth_view, _vk_allocator);
    vkFreeMemory(_device, _depth_buffer_memory, _vk_allocator);
    vkDestroyImage(_device, _depth_buffer, _vk_allocator);

    // create image
    VkExtent3D const extent = {
        _surface_capabilities.currentExtent.width, _surface_capabilities.currentExtent.height, 1,
    };
    VkImageCreateInfo const image_info = {
        VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,               // sType
        nullptr,                                           // pNext
        0,                                                 // flags
        VK_IMAGE_TYPE_2D,                                  // imageType
        VK_FORMAT_D32_SFLOAT,                              // format
        extent,                                            // extent
        1,                                                 // mipLevels
        1,                                                 // arrayLayers
        VK_SAMPLE_COUNT_1_BIT,                             // samples
        VK_IMAGE_TILING_OPTIMAL,                           // tiling
        VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,       // usage
        VK_SHARING_MODE_EXCLUSIVE,                         // sharingMode
        0,                                                 // queueFamilyIndexCount
        nullptr,                                           // pQueueFamilyIndices
        VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,  // initialLayout
    };

    VkResult result = vkCreateImage(_device, &image_info, _vk_allocator, &_depth_buffer);
    assert(VK_SUCCEEDED(result));

    // allocate memory
    VkMemoryRequirements memory_requirements = {};
    vkGetImageMemoryRequirements(_device, _depth_buffer, &memory_requirements);

    uint32_t const memory_type_index =
        get_memory_type_index(memory_requirements, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    assert(memory_type_index != UINT32_MAX && "Could not find acceptalbe memory type");
    VkMemoryAllocateInfo const alloc_info = {
        VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,  // sType
        nullptr,                                 // pNext
        memory_requirements.size,                // allocationSize
        memory_type_index,                       // memoryTypeIndex
    };
    result = vkAllocateMemory(_device, &alloc_info, _vk_allocator, &_depth_buffer_memory);
    assert(VK_SUCCEEDED(result));

    result = vkBindImageMemory(_device, _depth_buffer, _depth_buffer_memory, 0);
    assert(VK_SUCCEEDED(result));

    VkImageViewCreateInfo const image_view_info = {
        VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,  // sType
        nullptr,                                   // pNext
        0,                                         // flags
        _depth_buffer,                             // image
        VK_IMAGE_VIEW_TYPE_2D,                     // viewType
        image_info.format,                         // format
        // components
        {
            VK_COMPONENT_SWIZZLE_IDENTITY,  // r
            VK_COMPONENT_SWIZZLE_IDENTITY,  // g
            VK_COMPONENT_SWIZZLE_IDENTITY,  // b
            VK_COMPONENT_SWIZZLE_IDENTITY,  // a
        },
        kFullSubresourceRange,  // subresourceRange
    };
    result = vkCreateImageView(_device, &image_view_info, _vk_allocator, &_depth_view);
    assert(VK_SUCCEEDED(result));
}

std::unique_ptr<BufferVulkan> GraphicsVulkan::create_buffer(uint32_t size, VkBufferUsageFlags usage,
                                                            VkMemoryPropertyFlags property_flags)
{
    VkBufferCreateInfo const buffer_info = {
        VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,  // sType
        nullptr,                               // pNext
        0,                                     // flags
        size,                                  // size
        usage,                                 // usage
        VK_SHARING_MODE_EXCLUSIVE,             // sharingMode
        1,                                     // queueFamilyIndexCount
        &_queue_index,                         // pQueueFamilyIndices
    };
    VkBuffer buffer = VK_NULL_HANDLE;
    auto result = vkCreateBuffer(_device, &buffer_info, _vk_allocator, &buffer);
    assert(VK_SUCCEEDED(result));

    // Get memory requirements
    VkMemoryRequirements memory_requirements = {};
    vkGetBufferMemoryRequirements(_device, buffer, &memory_requirements);

    uint32_t const memory_type_index = get_memory_type_index(memory_requirements, property_flags);
    assert(memory_type_index != UINT32_MAX && "Could not find acceptalbe memory type");

    // Allocate memory
    VkMemoryAllocateInfo const allocation_info = {
        VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,  // sType
        nullptr,                                 // pNext
        memory_requirements.size,                // allocationSize
        memory_type_index,                       // memoryTypeIndex
    };
    VkDeviceMemory memory = VK_NULL_HANDLE;
    result = vkAllocateMemory(_device, &allocation_info, _vk_allocator, &memory);
    assert(VK_SUCCEEDED(result));

    result = vkBindBufferMemory(_device, buffer, memory, 0);
    assert(VK_SUCCEEDED(result));

    // return values
    auto out_buffer = std::make_unique<BufferVulkan>();
    out_buffer->_graphics = this;
    out_buffer->_buffer = buffer;
    out_buffer->_memory = memory;

    return out_buffer;
}

void GraphicsVulkan::create_upload_buffer()
{
    _upload_buffer =
        create_buffer(kUploadBufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    VkResult const result = vkMapMemory(_device, _upload_buffer->_memory, 0, kUploadBufferSize, 0,
                                        reinterpret_cast<void**>(&_upload_start));
    assert(VK_SUCCEEDED(result));

    _upload_current = _upload_start;
    _upload_end = _upload_start + kUploadBufferSize;
}

uint32_t GraphicsVulkan::get_back_buffer()
{
    Expects(_swap_chain);
    if (_back_buffer_index != UINT32_MAX) {
        return _back_buffer_index;
    }
    VkResult const result =
        vkAcquireNextImageKHR(_device, _swap_chain, UINT64_MAX, _swap_chain_semaphore,
                              VK_NULL_HANDLE, &_back_buffer_index);
    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
        resize(0, 0);
        return get_back_buffer();
    }
    assert(VK_SUCCEEDED(result) && "Could not get swap chain image index");

    return _back_buffer_index;
}
uint32_t GraphicsVulkan::get_memory_type_index(VkMemoryRequirements const& requirements,
                                               VkMemoryPropertyFlags const property_flags)
{
    VkPhysicalDeviceMemoryProperties memory_properties = {};
    vkGetPhysicalDeviceMemoryProperties(_physical_device, &memory_properties);

    for (uint32_t ii = 0; ii < memory_properties.memoryTypeCount; ++ii) {
        if ((requirements.memoryTypeBits & (1 << ii)) == 0) {
            continue;
        }
        if (memory_properties.memoryTypes[ii].propertyFlags & property_flags) {
            return ii;
        }
    }
    return UINT32_MAX;
}

size_t GraphicsVulkan::align_upload_buffer(size_t const alignment)
{
    uintptr_t current = (uintptr_t)_upload_current;
    size_t offset = 0;
    if (current % alignment != 0) {
        if (current + alignment > (uintptr_t)_upload_end) {
            offset = (uintptr_t)_upload_end - current;
            current = (uintptr_t)_upload_start;
        } else {
            current += alignment;
            current &= ~(alignment - 1);
            offset = current - (uintptr_t)_upload_current;
        }
        _upload_current = (uint8_t*)current;
    }
    return offset;
}

void* GraphicsVulkan::get_upload_data(size_t const size, size_t const alignment)
{
    size_t const offset = align_upload_buffer(alignment);
    size_t const available = _upload_end - _upload_current;
    if (available < size) {
        _upload_current = _upload_start;
    }
    uint8_t* const data = _upload_current;
    _upload_current += size;
    assert(_upload_current < _upload_end);
    return data;
}

ScopedGraphics create_graphics_vulkan()
{
    return std::make_unique<GraphicsVulkan>();
}

RenderStateVulkan::~RenderStateVulkan()
{
    auto device = _graphics->_device;
    _graphics->vkDestroyShaderModule(device, _vs_module, _graphics->_vk_allocator);
    _graphics->vkDestroyShaderModule(device, _ps_module, _graphics->_vk_allocator);
    _graphics->vkDestroyDescriptorSetLayout(device, _desc_set_layout, _graphics->_vk_allocator);
    _graphics->vkDestroyPipelineLayout(device, _pipeline_layout, _graphics->_vk_allocator);
    _graphics->vkDestroyPipeline(device, _pso, _graphics->_vk_allocator);
}

BufferVulkan::~BufferVulkan()
{
    if (!_graphics) {
        return;
    }
    auto allocator = _graphics->_vk_allocator;
    auto device = _graphics->_device;
    _graphics->vkFreeMemory(device, _memory, allocator);
    _graphics->vkDestroyBuffer(device, _buffer, allocator);
}

}  // namespace ak
