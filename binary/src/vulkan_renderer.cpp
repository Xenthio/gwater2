// vulkan_renderer.cpp  –  External Vulkan fluid renderer prototype
//
// This file provides STUB implementations that document how GWater2 would use
// Vulkan to render a screen-space fluid surface and composite it back into the
// Source Engine frame buffer.
//
// The rendering technique is screen-space fluid rendering as described in:
//   "Screen Space Fluid Rendering with Curvature Flow" (van der Laan et al.)
//   https://dl.acm.org/doi/10.1145/1507149.1507164
//
// and inspired by the Gelly GMod addon implementation:
//   https://github.com/gelly-gmod/gelly
//
// HOW TO ACTIVATE THIS RENDERER
// ──────────────────────────────
//  1. Install the Vulkan SDK (https://vulkan.lunarg.com/), making sure the
//     VULKAN_SDK environment variable is set.
//  2. Build with:  premake5 vs2022 --renderer=vulkan
//     The premake5.lua helpers will add the SDK include/lib dirs.
//  3. Uncomment the SDK includes below.
//  4. Implement the TODO sections to complete each render pass.

#include "vulkan_renderer.h"
#include <cstring>
#include <cstdio>

// ---------------------------------------------------------------------------
// Uncomment when the Vulkan SDK is available:
// ---------------------------------------------------------------------------
// #include <vulkan/vulkan.h>
// #ifdef _WIN32
// #include <vulkan/vulkan_win32.h>  // VK_KHR_external_memory_win32
// #else
// #include <vulkan/vulkan_linux.h>  // VK_EXT_external_memory_dma_buf
// #endif
// ---------------------------------------------------------------------------

// Validation layers (debug builds only)
#ifdef _DEBUG
static const char* k_validation_layers[] = { "VK_LAYER_KHRONOS_validation" };
static const int   k_num_validation_layers = 1;
#else
static const int   k_num_validation_layers = 0;
#endif

// Required device extensions
static const char* k_device_exts[] = {
	// "VK_KHR_swapchain",                 // not needed (off-screen render)
	// "VK_KHR_external_memory",
	// "VK_KHR_external_semaphore",
#ifdef _WIN32
	// "VK_KHR_external_memory_win32",
	// "VK_KHR_external_semaphore_win32",
#else
	// "VK_KHR_external_memory_fd",
	// "VK_KHR_external_semaphore_fd",
#endif
};

// ---------------------------------------------------------------------------
// Constructor / Destructor
// ---------------------------------------------------------------------------
VulkanRenderer::VulkanRenderer() {
	// TODO: call init_vulkan() once the SDK is available.
	// init_vulkan();
}

VulkanRenderer::~VulkanRenderer() {
	// shutdown_vulkan();
}

// ---------------------------------------------------------------------------
// Vulkan device initialisation  (stub – documented API calls)
// ---------------------------------------------------------------------------
bool VulkanRenderer::init_vulkan() {
	// ── Step 1: Create VkInstance ─────────────────────────────────────────
	// Request extensions required for DX11 / OpenGL interop.
	//
	// const char* inst_exts[] = {
	//     VK_KHR_EXTERNAL_MEMORY_CAPABILITIES_EXTENSION_NAME,
	//     VK_KHR_EXTERNAL_SEMAPHORE_CAPABILITIES_EXTENSION_NAME,
	//     VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME,
	// };
	// VkApplicationInfo app_info{};
	// app_info.sType              = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	// app_info.pApplicationName   = "GWater2";
	// app_info.applicationVersion = VK_MAKE_VERSION(2, 0, 0);
	// app_info.pEngineName        = "GarrysMod";
	// app_info.apiVersion         = VK_API_VERSION_1_3;
	//
	// VkInstanceCreateInfo inst_ci{};
	// inst_ci.sType                   = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	// inst_ci.pApplicationInfo        = &app_info;
	// inst_ci.enabledExtensionCount   = sizeof(inst_exts)/sizeof(inst_exts[0]);
	// inst_ci.ppEnabledExtensionNames = inst_exts;
	// inst_ci.enabledLayerCount       = k_num_validation_layers;
	// inst_ci.ppEnabledLayerNames     = k_validation_layers;
	// vkCreateInstance(&inst_ci, nullptr, &m_instance);

	// ── Step 2: Select physical device ────────────────────────────────────
	// Prefer the same GPU that Source Engine is using (identified by LUID on
	// Windows, or matching device UUID on Linux).
	//
	// uint32_t dev_count = 0;
	// vkEnumeratePhysicalDevices(m_instance, &dev_count, nullptr);
	// std::vector<VkPhysicalDevice> devs(dev_count);
	// vkEnumeratePhysicalDevices(m_instance, &dev_count, devs.data());
	// m_phys_device = pick_device_matching_dx11(devs);   // see helper below

	// ── Step 3: Create VkDevice ───────────────────────────────────────────
	// Enable all features needed for external memory + particle rendering.
	//
	// float queue_priority = 1.f;
	// VkDeviceQueueCreateInfo q_ci{};
	// q_ci.sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
	// q_ci.queueFamilyIndex = find_graphics_queue(m_phys_device);
	// q_ci.queueCount       = 1;
	// q_ci.pQueuePriorities = &queue_priority;
	//
	// VkPhysicalDeviceFeatures features{};
	// features.geometryShader = VK_TRUE;
	//
	// VkDeviceCreateInfo dev_ci{};
	// dev_ci.sType                   = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	// dev_ci.queueCreateInfoCount    = 1;
	// dev_ci.pQueueCreateInfos       = &q_ci;
	// dev_ci.enabledExtensionCount   = sizeof(k_device_exts)/sizeof(k_device_exts[0]);
	// dev_ci.ppEnabledExtensionNames = k_device_exts;
	// dev_ci.pEnabledFeatures        = &features;
	// vkCreateDevice(m_phys_device, &dev_ci, nullptr, &m_device);
	// vkGetDeviceQueue(m_device, q_ci.queueFamilyIndex, 0, &m_graphics_queue);

	// ── Step 4: Create command pool ───────────────────────────────────────
	// VkCommandPoolCreateInfo pool_ci{};
	// pool_ci.sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	// pool_ci.flags            = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	// pool_ci.queueFamilyIndex = q_ci.queueFamilyIndex;
	// vkCreateCommandPool(m_device, &pool_ci, nullptr, &m_cmd_pool);

	// ── Step 5: Allocate command buffers ──────────────────────────────────
	// VkCommandBufferAllocateInfo alloc_ci{};
	// alloc_ci.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	// alloc_ci.commandPool        = m_cmd_pool;
	// alloc_ci.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	// alloc_ci.commandBufferCount = 1;
	// vkAllocateCommandBuffers(m_device, &alloc_ci, &m_depth_cmd);
	// vkAllocateCommandBuffers(m_device, &alloc_ci, &m_smooth_cmd);
	// vkAllocateCommandBuffers(m_device, &alloc_ci, &m_normal_cmd);
	// vkAllocateCommandBuffers(m_device, &alloc_ci, &m_composite_cmd);

	return true;
}

void VulkanRenderer::shutdown_vulkan() {
	destroy_render_targets();
	destroy_pipelines();

	// if (m_frame_fence)     vkDestroyFence(m_device, m_frame_fence, nullptr);
	// if (m_render_finished) vkDestroySemaphore(m_device, m_render_finished, nullptr);
	// if (m_cmd_pool)        vkDestroyCommandPool(m_device, m_cmd_pool, nullptr);
	// if (m_device)          vkDestroyDevice(m_device, nullptr);
	// if (m_instance)        vkDestroyInstance(m_instance, nullptr);
}

// ---------------------------------------------------------------------------
// Render target creation
// ---------------------------------------------------------------------------
bool VulkanRenderer::create_render_targets(int width, int height) {
	m_viewport_w = width;
	m_viewport_h = height;

	// Each render target is created as a VkImage with VK_EXTERNAL_MEMORY flag
	// so it can be shared with the DX11 / OpenGL renderer.
	//
	// Example for the fluid output image (the one composited by Source Engine):
	//
	// VkExternalMemoryImageCreateInfo ext_info{};
	// ext_info.sType       = VK_STRUCTURE_TYPE_EXTERNAL_MEMORY_IMAGE_CREATE_INFO;
	// ext_info.handleTypes =
	// #ifdef _WIN32
	//     VK_EXTERNAL_MEMORY_HANDLE_TYPE_D3D11_TEXTURE_BIT;
	// #else
	//     VK_EXTERNAL_MEMORY_HANDLE_TYPE_DMA_BUF_BIT_EXT;
	// #endif
	//
	// VkImageCreateInfo img_ci{};
	// img_ci.sType       = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	// img_ci.pNext       = &ext_info;
	// img_ci.imageType   = VK_IMAGE_TYPE_2D;
	// img_ci.format      = VK_FORMAT_R8G8B8A8_UNORM;
	// img_ci.extent      = { (uint32_t)width, (uint32_t)height, 1 };
	// img_ci.mipLevels   = 1;
	// img_ci.arrayLayers = 1;
	// img_ci.samples     = VK_SAMPLE_COUNT_1_BIT;
	// img_ci.tiling      = VK_IMAGE_TILING_OPTIMAL;
	// img_ci.usage       = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT
	//                    | VK_IMAGE_USAGE_SAMPLED_BIT;
	// vkCreateImage(m_device, &img_ci, nullptr, &m_fluid_output.image);
	//
	// Export the memory handle:
	// #ifdef _WIN32
	// VkMemoryGetWin32HandleInfoKHR hinfo{};
	// hinfo.sType      = VK_STRUCTURE_TYPE_MEMORY_GET_WIN32_HANDLE_INFO_KHR;
	// hinfo.memory     = m_fluid_output.memory;
	// hinfo.handleType = VK_EXTERNAL_MEMORY_HANDLE_TYPE_D3D11_TEXTURE_BIT;
	// vkGetMemoryWin32HandleKHR(m_device, &hinfo, &m_fluid_output.win32_handle);
	// #endif

	// Create depth target, smoothed depth, and normals images similarly…

	return true;
}

void VulkanRenderer::destroy_render_targets() {
	// vkDestroyImageView / vkDestroyImage / vkFreeMemory for each target.
}

// ---------------------------------------------------------------------------
// Pipeline creation
// ---------------------------------------------------------------------------
bool VulkanRenderer::create_pipelines() {
	// ── Depth splatting pipeline ──────────────────────────────────────────
	// Vertex shader: takes a particle (x,y,z,radius) and emits a screen-space
	//                quad (or uses a geometry shader to expand a point to a quad).
	// Fragment shader: outputs depth from sphere intersection.
	//
	// ── Bilateral depth filter pipeline ──────────────────────────────────
	// Compute shader: reads depth target, writes smoothed depth.
	// Kernel: bilateral filter (σ_spatial ≈ 10px, σ_range ≈ 0.05 in NDC depth).
	//
	// ── Normal reconstruction pipeline ───────────────────────────────────
	// Compute shader: reads smoothed depth, writes normals.
	// Uses finite differences in screen space to compute surface gradient.
	//
	// ── Composite pipeline ────────────────────────────────────────────────
	// Vertex shader: full-screen triangle.
	// Fragment shader:
	//   1. Unproject depth → world position
	//   2. Compute Fresnel reflectance from surface normal + view direction
	//   3. Sample scene colour (refracted) and environment reflection
	//   4. Blend foam (diffuse particles) onto the surface
	//   5. Output RGBA colour to m_fluid_output shared image

	return true;
}

void VulkanRenderer::destroy_pipelines() {
	// vkDestroyPipeline / vkDestroyPipelineLayout for each pipeline.
}

// ---------------------------------------------------------------------------
// build() – called every frame before draw_*()
// ---------------------------------------------------------------------------
void VulkanRenderer::build(ISolverInterface* solver, float diffuse_radius, bool /*cull*/) {
	if (!solver) return;
	m_diffuse_radius = diffuse_radius;

	// Resize render targets if the viewport changed.
	// (In practice you'd query the Source Engine viewport here.)
	// int w, h; engine->GetScreenSize(w, h);
	// if (w != m_viewport_w || h != m_viewport_h) resize(w, h);

	// Upload particle data to GPU buffers.
	upload_particles(solver);

	// Record and submit all render passes.
	// vkWaitForFences(m_device, 1, &m_frame_fence, VK_TRUE, UINT64_MAX);
	// vkResetFences(m_device, 1, &m_frame_fence);

	record_depth_pass();
	record_smooth_pass();
	record_normal_pass();
	record_composite_pass();

	// VkSubmitInfo submit{};
	// submit.sType              = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	// submit.commandBufferCount = 4;
	// VkCommandBuffer cmds[4] = {m_depth_cmd, m_smooth_cmd, m_normal_cmd, m_composite_cmd};
	// submit.pCommandBuffers    = cmds;
	// submit.signalSemaphoreCount = 1;
	// submit.pSignalSemaphores    = &m_render_finished;
	// vkQueueSubmit(m_graphics_queue, 1, &submit, m_frame_fence);
}

// ---------------------------------------------------------------------------
// upload_particles() – copy solver particle data to Vulkan GPU buffers
// ---------------------------------------------------------------------------
bool VulkanRenderer::upload_particles(ISolverInterface* solver) {
	GWParticleData pd = solver->get_particle_data();
	GWDiffuseData  dd = solver->get_diffuse_data();

	m_water_count   = pd.active_count;
	m_diffuse_count = dd.count;

	if (m_water_count == 0) return true;

	// Grow the GPU buffer if needed.
	// if (m_water_buf.capacity < pd.active_count) { ... vkCreateBuffer ... }

	// Upload positions for the depth-splatting pass.
	// void* mapped;
	// vkMapMemory(m_device, m_water_buf.memory, 0, VK_WHOLE_SIZE, 0, &mapped);
	// memcpy(mapped, pd.smoothed_pos,
	//        pd.active_count * 4 * sizeof(float));   // float4: x,y,z,inv_mass
	// vkUnmapMemory(m_device, m_water_buf.memory);

	// (Similarly upload diffuse particles to m_diffuse_buf.)

	return true;
}

// ---------------------------------------------------------------------------
// Render pass recording stubs
// ---------------------------------------------------------------------------
void VulkanRenderer::record_depth_pass() {
	// vkBeginCommandBuffer(m_depth_cmd, ...);
	// vkCmdBeginRenderPass(m_depth_cmd, &depth_rp_begin, VK_SUBPASS_CONTENTS_INLINE);
	// vkCmdBindPipeline(m_depth_cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_depth_pipeline);
	// vkCmdBindVertexBuffers(m_depth_cmd, 0, 1, &m_water_buf.buffer, {0});
	// vkCmdDraw(m_depth_cmd, m_water_count, 1, 0, 0);
	// vkCmdEndRenderPass(m_depth_cmd);
	// vkEndCommandBuffer(m_depth_cmd);
}

void VulkanRenderer::record_smooth_pass() {
	// Bilateral depth filter compute dispatch.
	// vkBeginCommandBuffer(m_smooth_cmd, ...);
	// vkCmdBindPipeline(m_smooth_cmd, VK_PIPELINE_BIND_POINT_COMPUTE, m_smooth_pipeline);
	// vkCmdBindDescriptorSets(m_smooth_cmd, VK_PIPELINE_BIND_POINT_COMPUTE,
	//     m_smooth_layout, 0, 1, &m_smooth_desc, 0, nullptr);
	// vkCmdDispatch(m_smooth_cmd,
	//     (m_viewport_w + 7) / 8, (m_viewport_h + 7) / 8, 1);
	// vkEndCommandBuffer(m_smooth_cmd);
}

void VulkanRenderer::record_normal_pass() {
	// Normal reconstruction from smoothed depth buffer.
	// vkBeginCommandBuffer(m_normal_cmd, ...);
	// vkCmdBindPipeline(m_normal_cmd, VK_PIPELINE_BIND_POINT_COMPUTE, m_normal_pipeline);
	// vkCmdDispatch(m_normal_cmd, (m_viewport_w+7)/8, (m_viewport_h+7)/8, 1);
	// vkEndCommandBuffer(m_normal_cmd);
}

void VulkanRenderer::record_composite_pass() {
	// Full-screen shading + composite into shared output image.
	// vkBeginCommandBuffer(m_composite_cmd, ...);
	// vkCmdBeginRenderPass(m_composite_cmd, &composite_rp_begin, VK_SUBPASS_CONTENTS_INLINE);
	// vkCmdBindPipeline(m_composite_cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_composite_pipeline);
	// vkCmdDraw(m_composite_cmd, 3, 1, 0, 0);  // full-screen triangle
	// vkCmdEndRenderPass(m_composite_cmd);
	// vkEndCommandBuffer(m_composite_cmd);
}

// ---------------------------------------------------------------------------
// draw_*() – Source Engine hooks that sample the shared texture.
// These functions run on the DX11 thread after Vulkan has finished rendering.
// ---------------------------------------------------------------------------
void VulkanRenderer::draw_water() {
	// Wait for the Vulkan render to finish.
	// On Windows, IDXGIKeyedMutex::AcquireSync() on m_fluid_output.win32_handle.
	//
	// Then bind the shared image as a Source Engine ITexture and draw a
	// full-screen quad using the GWaterFinalpass shader (or a new composite
	// shader that simply copies the Vulkan output).
	//
	// ITexture* fluid_tex = get_or_create_shared_texture(m_fluid_output.win32_handle);
	// IMaterial* mat = GetCompositorMaterial();
	// mat->FindVar("$basetexture")->SetTextureValue(fluid_tex);
	// materials->GetRenderContext()->DrawScreenSpaceRectangle(...);
	//
	// Release the mutex:
	// IDXGIKeyedMutex::ReleaseSync(1);
}

void VulkanRenderer::draw_diffuse() {
	// Diffuse (foam/bubble) particles are rendered inside the Vulkan composite
	// pass, so this is a no-op in the Vulkan renderer.  The Source Engine
	// FlexRenderer draw_diffuse() method is used only for the fallback path.
}

void VulkanRenderer::draw_cloth() {
	// Cloth can optionally be rendered by the Vulkan path as a triangle mesh,
	// or fall back to the Source Engine renderer.
}

// ---------------------------------------------------------------------------
// Misc
// ---------------------------------------------------------------------------
void VulkanRenderer::resize(int width, int height) {
	if (width == m_viewport_w && height == m_viewport_h) return;
	destroy_render_targets();
	create_render_targets(width, height);
}

void* VulkanRenderer::get_shared_handle() const {
#ifdef _WIN32
	return m_fluid_output.win32_handle;
#else
	// On Linux the DMA-buf FD is returned via the VkSharedImage struct directly.
	// Callers on Linux should use get_dma_buf_fd() rather than this helper.
	return nullptr;
#endif
}

#ifndef _WIN32
int VulkanRenderer::get_dma_buf_fd() const {
	return m_fluid_output.dma_buf_fd;
}
#endif
