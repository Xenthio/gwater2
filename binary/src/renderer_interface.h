#pragma once
#include "solver_interface.h"

// ---------------------------------------------------------------------------
// IRendererInterface  -  abstract base class for fluid renderers
//
// A renderer consumes the read-only particle/mesh snapshots produced by an
// ISolverInterface and emits Source Engine draw calls (or an equivalent
// external render such as Vulkan) each frame.
//
// Usage pattern each frame:
//   renderer->build(solver, diffuse_radius, cull);   // prepare GPU buffers
//   renderer->draw_water();                          // submit water geometry
//   renderer->draw_diffuse();                        // submit foam geometry
//   renderer->draw_cloth();                          // submit cloth geometry
// ---------------------------------------------------------------------------
class IRendererInterface {
public:
	virtual ~IRendererInterface() = default;

	// Prepare all rendering data from the solver's current particle snapshot.
	// This may run mesh-building work on background threads.
	// 'diffuse_radius' : visual radius used for foam/bubble sprites
	// 'cull'           : enable PVS-based frustum culling of particles
	virtual void build(ISolverInterface* solver, float diffuse_radius, bool cull) = 0;

	// Submit water-particle geometry (billboards + anisotropy).
	virtual void draw_water()   = 0;
	// Submit diffuse (foam/bubble) particle geometry.
	virtual void draw_diffuse() = 0;
	// Submit cloth triangle geometry.
	virtual void draw_cloth()   = 0;

	// Human-readable renderer identifier used for diagnostics and selection.
	virtual const char* get_renderer_name() const = 0;
};
