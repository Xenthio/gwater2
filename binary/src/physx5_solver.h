#pragma once
#include "solver_interface.h"
#include <map>
#include <vector>

// ---------------------------------------------------------------------------
// PhysX5Solver  –  prototype fluid solver backed by NVIDIA PhysX 5
//
// PhysX 5 introduces a GPU-accelerated Position-Based Dynamics (PBD) particle
// system (PxPBDParticleSystem) that supports fluid simulation with similar
// features to the legacy NvFleX library:
//   • Particle-particle fluid interactions (SPH density estimation)
//   • Rigid-body / triangle-mesh collisions
//   • Diffuse particles (foam, bubbles, spray)
//   • Spring constraints for cloth
//
// This file is a PROTOTYPE stub.  It documents the mapping from the GWater2
// solver interface to the PhysX 5 API, but does not link against the PhysX 5
// SDK.  To activate this backend:
//   1. Add the PhysX 5 SDK as a submodule (or download it separately):
//         https://github.com/NVIDIA-Omniverse/PhysX
//   2. Add its include/ and lib/ paths to premake5.lua under the physx5 filter.
//   3. Uncomment the #include blocks below.
//   4. Build with:  premake5 vs2022 --backend=physx5
//
// Key API differences vs NvFleX
// ─────────────────────────────
//  NvFleX                             PhysX 5
//  ─────────────────────────────────  ───────────────────────────────────────
//  NvFlexInit / NvFlexCreateSolver    PxCreateFoundation + PxCreatePhysics
//                                       + PxCreateScene + PxCreatePBDParticleSystem
//  NvFlexBuffer (host mapped)         PxGpuBuffer / PxParticleBuffer (CUDA)
//  NvFlexSetParams (NvFlexParams)     PxPBDParticleSystem setters (per param)
//  NvFlexUpdateSolver (dt, substeps)  PxScene::simulate(dt) + fetchResults()
//  NvFlexCollisionGeometry            PxRigidStatic + PxTriangleMesh / PxConvexMesh
//  NvFlexExtForceField                PxParticleSystem::addParticleAttachment or
//                                       custom CUDA kernel via PxGpuDynamicsMemConfig
// ---------------------------------------------------------------------------

// Forward-declare PhysX 5 types so this header can be included without the
// PhysX SDK in the include path (the physx5_solver.cpp TU does the real work).
namespace physx {
	class PxFoundation;
	class PxPhysics;
	class PxScene;
	class PxCudaContextManager;
	struct PxCudaContextManagerDesc;
	class PxDefaultCpuDispatcher;
	class PxPBDParticleSystem;
	class PxParticleBuffer;
	class PxParticleAndDiffuseBuffer;
	class PxRigidStatic;
	class PxMaterial;
	class PxTriangleMesh;
	class PxConvexMesh;
	class PxShape;
}

// Unit conversion: PhysX 5 uses SI metres; Source Engine uses inches.
static constexpr float PX5_INCH_TO_M = 0.0254f;
static constexpr float PX5_M_TO_INCH = 1.0f / PX5_INCH_TO_M;

// Per-mesh collision record
struct PhysX5Mesh {
	int                   entity_id = 0;
	bool                  enabled   = true;
	physx::PxRigidStatic* actor     = nullptr;  // PhysX scene actor
};

// ---------------------------------------------------------------------------
class PhysX5Solver : public ISolverInterface {
public:
	// 'max_particles'         upper bound on fluid (PBD) particles
	// 'max_diffuse_particles' upper bound on diffuse (foam/bubble) particles
	PhysX5Solver(int max_particles, int max_diffuse_particles);
	~PhysX5Solver() override;

	// ---- ISolverInterface --------------------------------------------------
	bool add_particle    (const GWParticle&  particle) override;
	void add_cloth       (const VMatrix& transform, Vector2D size,
	                      const GWParticle&  particle) override;
	void add_force_field (const GWForceField& field)   override;

	bool tick(float dt) override;

	void reset()         override;
	void reset_cloth()   override;
	void reset_diffuse() override;

	int  get_active_particles()      const override;
	int  get_max_particles()         const override;
	int  get_active_diffuse()        const override;
	int  get_max_diffuse_particles() const override;
	int  get_active_triangles()      const override;

	bool  set_parameter(const std::string& name, float value) override;
	float get_parameter(const std::string& name) const override;

	GWMeshHandle add_collision_mesh(const Vector* verts, int num_verts,
	                                bool dynamic, bool convex,
	                                int entity_id) override;
	void remove_collision_mesh(int entity_id) override;
	void set_mesh_position(GWMeshHandle handle, const Vector& pos) override;
	void set_mesh_rotation(GWMeshHandle handle, const QAngle& ang) override;
	void set_mesh_enabled (GWMeshHandle handle, bool enabled)      override;

	void enable_bounds (const Vector& mins, const Vector& maxs) override;
	void disable_bounds() override;

	GWParticleData get_particle_data() const override;
	GWDiffuseData  get_diffuse_data()  const override;
	GWClothData    get_cloth_data()    const override;

	void        enable_diffuse(bool enabled) override;
	const char* get_backend_name() const override { return "PhysX5"; }

private:
	// PhysX foundation / scene objects
	physx::PxFoundation*           m_foundation   = nullptr;
	physx::PxPhysics*              m_physics      = nullptr;
	physx::PxCudaContextManager*   m_cuda_mgr     = nullptr;
	physx::PxDefaultCpuDispatcher* m_cpu_dispatch = nullptr;
	physx::PxScene*                m_scene        = nullptr;
	physx::PxMaterial*             m_material     = nullptr;

	// PBD particle system (replaces NvFlexSolver)
	physx::PxPBDParticleSystem*         m_particle_sys  = nullptr;
	// Combined fluid + diffuse particle buffer
	physx::PxParticleAndDiffuseBuffer*  m_fluid_buf     = nullptr;

	// CPU-side shadow copies updated after each PxScene::fetchResults()
	std::vector<float> m_positions;    // float4: x,y,z,inv_mass  (source inches)
	std::vector<float> m_smoothed;     // float4: smoothed positions
	std::vector<float> m_velocities;   // float3 (source inches/s)
	std::vector<int>   m_phases;
	std::vector<int>   m_active;
	std::vector<float> m_lifetimes;
	std::vector<float> m_aniso0, m_aniso1, m_aniso2;  // float4

	std::vector<float> m_diffuse_pos; // float4
	std::vector<float> m_diffuse_vel; // float4
	int                m_diffuse_count = 0;
	int                m_active_count  = 0;

	// Cloth triangle data
	std::vector<int>   m_tri_indices;
	std::vector<float> m_tri_normals; // float4
	int                m_tri_count = 0;

	int m_max_particles;
	int m_max_diffuse;

	// Collision meshes
	std::vector<PhysX5Mesh> m_meshes;

	// Simulation parameters (mirrors NvFlexParams semantics)
	std::map<std::string, float> m_params;
	bool m_diffuse_enabled = true;

	// Internal helpers
	bool init_physx();
	void shutdown_physx();
	void init_parameters();
	void sync_from_gpu();  // copy GPU results into CPU shadow buffers

	// Convert Source inches to PhysX metres and back
	static float to_m(float inch) { return inch * PX5_INCH_TO_M; }
	static float to_inch(float m)  { return m  * PX5_M_TO_INCH;  }
};
