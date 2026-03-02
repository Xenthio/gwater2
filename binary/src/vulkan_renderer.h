#pragma once
#include "renderer_interface.h"
#include <vector>

// ---------------------------------------------------------------------------
// VulkanRenderer  –  prototype external Vulkan fluid renderer
//
// This renderer produces a screen-space fluid surface by rendering particle
// data with Vulkan and compositing the result into the Source Engine frame
// buffer via a shared DX11/Vulkan texture (Windows) or external memory
// extension (Linux).
//
// ARCHITECTURE OVERVIEW
// ─────────────────────
// The approach is inspired by the Gelly GMod addon:
//   https://github.com/gelly-gmod/gelly
//
// Render pipeline (executed every frame):
//
//   1. DEPTH PASS
//      For each active particle, splat a sphere into a Vulkan depth texture
//      that matches the Source Engine back-buffer resolution.  Uses
//      vkCmdDrawIndirect with a GPU-filled draw-args buffer to avoid CPU round
//      trips.
//
//   2. SMOOTHING PASS  (optional, expensive)
//      Run a bilateral / narrow-range filter over the depth texture to merge
//      individual particle blobs into a smooth liquid surface.
//
//   3. NORMAL RECONSTRUCTION PASS
//      Reconstruct surface normals from the smoothed depth buffer using
//      screen-space finite differences (ddx/ddy equivalent in a compute shader).
//
//   4. COMPOSITE PASS
//      Sample the Source Engine scene colour (shared from DX11 → Vulkan) and
//      apply the fluid shading (specular, refraction, foam overlay) into the
//      final shared texture.  This texture is then bound as a Source material
//      and drawn as a full-screen quad in the DX11 render path (PostRender hook).
//
// INTEROP MECHANISM
// ─────────────────
//  Windows:  Use VK_KHR_external_memory + VK_KHR_external_memory_win32 to
//             import the Source Engine DX11 back-buffer as a VkImage, and export
//             the composited fluid image back to DX11 via IDXGIKeyedMutex.
//
//  Linux:    Use VK_KHR_external_memory + VK_EXT_external_memory_dma_buf with
//             a DRM Prime FD.  Source Engine's OpenGL (via DXVK / togl) can
//             then import the FD via GL_EXT_memory_object_fd.
//
// TO ACTIVATE THIS RENDERER
// ──────────────────────────
//  1. Install the Vulkan SDK ≥ 1.3 (https://vulkan.lunarg.com/).
//     Vulkan 1.3 is required because the composite shader relies on dynamic
//     rendering (VK_KHR_dynamic_rendering, promoted to core in 1.3) and the
//     external memory extensions require VK_KHR_get_physical_device_properties2
//     (also core in 1.1+).  Vulkan 1.2 could work if all required extensions
//     are manually enabled, but 1.3 is the recommended minimum.
//  2. In premake5.lua build with --renderer=vulkan.
//     The helpers will add:
//         includedirs { "$(VULKAN_SDK)/Include" }
//         libdirs     { "$(VULKAN_SDK)/Lib" }
//         links       { "vulkan-1" }           -- Windows
//         -- links    { "vulkan" }             -- Linux
//  3. Uncomment the SDK includes in vulkan_renderer.cpp.
// ---------------------------------------------------------------------------

// Forward-declare Vulkan handles (avoids dragging in vulkan.h everywhere).
// The .cpp TU does the real work.
struct VkInstance_T;      using VkInstance       = VkInstance_T*;
struct VkPhysicalDevice_T;using VkPhysicalDevice = VkPhysicalDevice_T*;
struct VkDevice_T;        using VkDevice         = VkDevice_T*;
struct VkQueue_T;         using VkQueue          = VkQueue_T*;
struct VkCommandPool_T;   using VkCommandPool    = VkCommandPool_T*;
struct VkCommandBuffer_T; using VkCommandBuffer  = VkCommandBuffer_T*;
struct VkRenderPass_T;    using VkRenderPass     = VkRenderPass_T*;
struct VkPipeline_T;      using VkPipeline       = VkPipeline_T*;
struct VkPipelineLayout_T;using VkPipelineLayout = VkPipelineLayout_T*;
struct VkDescriptorPool_T;using VkDescriptorPool = VkDescriptorPool_T*;
struct VkDescriptorSet_T; using VkDescriptorSet  = VkDescriptorSet_T*;
struct VkBuffer_T;        using VkBuffer         = VkBuffer_T*;
struct VkDeviceMemory_T;  using VkDeviceMemory   = VkDeviceMemory_T*;
struct VkImage_T;         using VkImage          = VkImage_T*;
struct VkImageView_T;     using VkImageView      = VkImageView_T*;
struct VkSampler_T;       using VkSampler        = VkSampler_T*;
struct VkFramebuffer_T;   using VkFramebuffer    = VkFramebuffer_T*;
struct VkSemaphore_T;     using VkSemaphore      = VkSemaphore_T*;
struct VkFence_T;         using VkFence          = VkFence_T*;

// Shared render target exported to DX11 (Windows) / DRM (Linux)
struct VkSharedImage {
	VkImage       image      = nullptr;
	VkImageView   view       = nullptr;
	VkDeviceMemory memory    = nullptr;
	// Handle used for DX11 / OpenGL import:
#if defined(_WIN32)
	void*         win32_handle = nullptr;  // HANDLE
#else
	int           dma_buf_fd   = -1;
#endif
	int  width  = 0;
	int  height = 0;
};

// GPU-side particle position buffer for the depth-splatting pass
struct VkParticleBuffer {
	VkBuffer       buffer = nullptr;
	VkDeviceMemory memory = nullptr;
	int            capacity = 0;  // max particles
};

// ---------------------------------------------------------------------------
class VulkanRenderer : public IRendererInterface {
public:
	VulkanRenderer();
	~VulkanRenderer() override;

	// IRendererInterface
	void build(ISolverInterface* solver, float diffuse_radius, bool cull) override;
	void draw_water()   override;
	void draw_diffuse() override;
	void draw_cloth()   override;
	const char* get_renderer_name() const override { return "Vulkan"; }

	// Resize the render targets when the Source Engine viewport changes.
	void resize(int width, int height);

	// Returns the DX11-importable handle (Windows) for the composited fluid
	// image.  The Source Engine renderer binds this as a material texture and
	// draws a full-screen compositing quad.  Returns nullptr on Linux
	// (use get_dma_buf_fd() instead on that platform).
	void* get_shared_handle() const;

#ifndef _WIN32
	// Returns the DMA-buf file descriptor for the composited fluid image on
	// Linux.  Import with GL_EXT_memory_object_fd or a Vulkan KHR_external_memory
	// consumer.  Returns -1 if the renderer has not been initialised yet.
	int get_dma_buf_fd() const;
#endif

private:
	// ---- Vulkan core objects ----
	VkInstance       m_instance        = nullptr;
	VkPhysicalDevice m_phys_device     = nullptr;
	VkDevice         m_device          = nullptr;
	VkQueue          m_graphics_queue  = nullptr;
	VkCommandPool    m_cmd_pool        = nullptr;

	// ---- Per-frame command buffers ----
	VkCommandBuffer  m_depth_cmd       = nullptr;
	VkCommandBuffer  m_smooth_cmd      = nullptr;
	VkCommandBuffer  m_normal_cmd      = nullptr;
	VkCommandBuffer  m_composite_cmd   = nullptr;

	// ---- Render passes ----
	VkRenderPass m_depth_pass     = nullptr;
	VkRenderPass m_composite_pass = nullptr;

	// ---- Pipelines ----
	VkPipelineLayout m_depth_layout     = nullptr;
	VkPipeline       m_depth_pipeline   = nullptr;   // particle depth splatting

	VkPipelineLayout m_smooth_layout    = nullptr;
	VkPipeline       m_smooth_pipeline  = nullptr;   // bilateral depth filter

	VkPipelineLayout m_normal_layout    = nullptr;
	VkPipeline       m_normal_pipeline  = nullptr;   // normal reconstruction

	VkPipelineLayout m_composite_layout = nullptr;
	VkPipeline       m_composite_pipeline = nullptr; // final shading + composite

	// ---- Render targets ----
	VkSharedImage    m_depth_target{};      // particle depth / thickness map
	VkSharedImage    m_smoothed_depth{};    // bilateral-filtered depth
	VkSharedImage    m_normal_target{};     // reconstructed normals
	VkSharedImage    m_fluid_output{};      // composited fluid frame (shared → DX11)

	// ---- GPU particle data ----
	VkParticleBuffer m_water_buf{};
	VkParticleBuffer m_diffuse_buf{};

	// ---- Descriptor sets ----
	VkDescriptorPool m_desc_pool = nullptr;
	VkDescriptorSet  m_depth_desc{};
	VkDescriptorSet  m_smooth_desc{};
	VkDescriptorSet  m_normal_desc{};
	VkDescriptorSet  m_composite_desc{};

	// ---- Synchronisation ----
	VkSemaphore m_render_finished = nullptr;
	VkFence     m_frame_fence     = nullptr;

	// ---- State ----
	int  m_viewport_w  = 0;
	int  m_viewport_h  = 0;
	int  m_water_count = 0;
	int  m_diffuse_count = 0;
	float m_diffuse_radius = 1.f;

	// ---- Internals ----
	bool init_vulkan();
	void shutdown_vulkan();

	bool create_render_targets(int width, int height);
	void destroy_render_targets();

	bool create_pipelines();
	void destroy_pipelines();

	bool upload_particles(ISolverInterface* solver);

	void record_depth_pass();
	void record_smooth_pass();
	void record_normal_pass();
	void record_composite_pass();
};
