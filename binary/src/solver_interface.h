#pragma once
#include <string>
#include <vector>
#include "mathlib/vector.h"
#include "mathlib/vector4d.h"
#include "mathlib/vmatrix.h"
#include "mathlib/vector2d.h"

// ---------------------------------------------------------------------------
// Common types shared across all solver backends
// ---------------------------------------------------------------------------

// Particle phases
enum GWParticlePhase {
	GW_PHASE_FLUID = 0,
	GW_PHASE_CLOTH = 1,
};

// Force-field application modes
enum GWForceMode {
	GW_FORCE_LINEAR  = 0,	// Continuous force proportional to mass
	GW_FORCE_IMPULSE = 1,	// Instantaneous velocity change
};

// Backend-agnostic particle descriptor
struct GWParticle {
	float   pos[4];		// x, y, z, inverse_mass (1/mass)
	float   vel[3];		// vx, vy, vz
	int     phase;		// GWParticlePhase or custom flags
	float   lifetime;	// Remaining simulation-time; FLT_MAX = immortal
};

// Backend-agnostic force field
struct GWForceField {
	float       position[3];
	float       radius;
	float       strength;
	GWForceMode mode;
	bool        linear_falloff;
};

// Opaque handle for a registered collision mesh
using GWMeshHandle = int;
static constexpr GWMeshHandle GW_INVALID_MESH = -1;

// ---------------------------------------------------------------------------
// Read-only snapshots of solver state, used by renderers each frame
// ---------------------------------------------------------------------------

// All arrays are float4 (stride 16 bytes) unless noted otherwise.
struct GWParticleData {
	const float* positions;		// float4: x,y,z,inv_mass
	const float* smoothed_pos;	// float4: smoothed positions for surface reconstruction
	const float* velocities;	// float3: per-particle velocity (stride = 12 bytes)
	const int*   phases;		// per-particle phase
	const int*   active_indices;// indices of active particles into the above arrays
	const float* anisotropy0;	// float4: principal axis 0 + scale
	const float* anisotropy1;	// float4: principal axis 1 + scale
	const float* anisotropy2;	// float4: principal axis 2 + scale
	int          active_count;
	int          max_count;
};

// Diffuse (foam/bubble) particle snapshot
struct GWDiffuseData {
	const float* positions;		// float4
	const float* velocities;	// float4
	int          count;
};

// Cloth/triangle mesh snapshot for rendering
struct GWClothData {
	const float* normals;		// float4: per-triangle normals
	const int*   indices;		// triangle vertex indices into the particle position array
	int          triangle_count;
};

// ---------------------------------------------------------------------------
// ISolverInterface  -  abstract base class for fluid simulation backends
//
// Unit convention: all positions/velocities are in Source Engine inches.
//                  Each backend is responsible for any internal unit conversion.
// ---------------------------------------------------------------------------
class ISolverInterface {
public:
	virtual ~ISolverInterface() = default;

	// ---- Particle management ------------------------------------------------
	// Returns true if the particle was accepted (i.e. capacity not exceeded).
	virtual bool add_particle(const GWParticle& particle) = 0;
	// Adds a grid of cloth particles with spring constraints.
	virtual void add_cloth(const VMatrix& transform, Vector2D size, const GWParticle& particle) = 0;
	// Queues a force field to be applied during the next tick.
	virtual void add_force_field(const GWForceField& field) = 0;

	// ---- Simulation ---------------------------------------------------------
	// Advance the simulation by dt seconds (Source-space time, before timescale).
	// Returns false if the tick was skipped (dt == 0, no active particles, etc.).
	virtual bool tick(float dt) = 0;

	// ---- State reset --------------------------------------------------------
	virtual void reset() = 0;
	virtual void reset_cloth() = 0;
	virtual void reset_diffuse() = 0;

	// ---- Queries ------------------------------------------------------------
	virtual int get_active_particles()       const = 0;
	virtual int get_max_particles()          const = 0;
	virtual int get_active_diffuse()         const = 0;
	virtual int get_max_diffuse_particles()  const = 0;
	virtual int get_active_triangles()       const = 0;

	// ---- Simulation parameters ----------------------------------------------
	// Returns false when the parameter name is not recognised.
	virtual bool  set_parameter(const std::string& name, float value) = 0;
	// Returns NaN when the parameter name is not recognised.
	virtual float get_parameter(const std::string& name) const = 0;

	// ---- Collision geometry -------------------------------------------------
	// Registers a triangle mesh collider belonging to entity `entity_id`.
	// The returned handle is used for subsequent position/rotation updates.
	virtual GWMeshHandle add_collision_mesh(const Vector* verts, int num_verts,
	                                         bool dynamic, bool convex,
	                                         int entity_id) = 0;
	// Removes all meshes associated with entity_id.
	virtual void remove_collision_mesh(int entity_id) = 0;
	virtual void set_mesh_position(GWMeshHandle handle, const Vector& pos) = 0;
	virtual void set_mesh_rotation(GWMeshHandle handle, const QAngle& ang) = 0;
	virtual void set_mesh_enabled (GWMeshHandle handle, bool enabled) = 0;

	// ---- World bounds -------------------------------------------------------
	virtual void enable_bounds (const Vector& mins, const Vector& maxs) = 0;
	virtual void disable_bounds() = 0;

	// ---- Rendering data -----------------------------------------------------
	// These return lightweight structs of raw pointers into backend-owned
	// memory. The pointers remain valid until the next call to tick().
	virtual GWParticleData get_particle_data() const = 0;
	virtual GWDiffuseData  get_diffuse_data()  const = 0;
	virtual GWClothData    get_cloth_data()    const = 0;

	// ---- Misc ---------------------------------------------------------------
	virtual void        enable_diffuse(bool enabled) = 0;
	// Human-readable backend identifier used for diagnostics and selection.
	virtual const char* get_backend_name() const = 0;
};
