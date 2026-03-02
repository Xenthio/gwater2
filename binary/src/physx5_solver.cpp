// physx5_solver.cpp  –  PhysX 5 backend prototype implementation
//
// This file provides STUB implementations that document the mapping from the
// GWater2 ISolverInterface to the PhysX 5 PBD particle-system API.
//
// HOW TO ACTIVATE THIS BACKEND
// ─────────────────────────────
//  1. Clone the PhysX 5 SDK alongside the binary directory:
//         git clone https://github.com/NVIDIA-Omniverse/PhysX binary/PhysX5
//  2. Build the PhysX 5 SDK with GPU support enabled:
//         cd binary/PhysX5/physx && generate_projects.bat/sh
//         <build vc2022 / gmake>
//  3. In premake5.lua, build with --backend=physx5 and the helpers will add:
//         includedirs { "PhysX5/physx/include" }
//         libdirs     { "PhysX5/physx/bin/..." }
//         links       { "PhysX_64", "PhysXCommon_64", "PhysXFoundation_64",
//                       "PhysXCooking_64", "PhysXExtensions_static_64",
//                       "PhysXPvdSDK_static_64" }
//  4. Uncomment the #include block below and remove the stub bodies.
//
// IMPORTANT NOTES
// ───────────────
//  • PhysX 5 uses SI metres. GWater2 internally uses Source Engine inches.
//    All positions/velocities must be scaled by PX5_INCH_TO_M on the way in
//    and PX5_M_TO_INCH on the way out. See to_m() / to_inch() helpers.
//  • The PxPBDParticleSystem requires CUDA. Make sure the user has an
//    NVIDIA GPU with driver ≥ 527.41 (CUDA 12 toolkit compat level).
//  • Diffuse particles (foam/bubbles) use PxParticleAndDiffuseBuffer.
//  • Cloth is represented by PxParticleSystems with spring constraints via
//    PxParticleBuffer spring attachments.

#include "physx5_solver.h"

// ---------------------------------------------------------------------------
// Uncomment when the PhysX 5 SDK is available in the include path:
// ---------------------------------------------------------------------------
// #include <PxPhysicsAPI.h>
// #include <extensions/PxDefaultCpuDispatcher.h>
// #include <extensions/PxDefaultSimulationFilterShader.h>
// #include <cudamanager/PxCudaContextManager.h>
// #include <PxPBDParticleSystem.h>
// #include <PxParticleBuffer.h>
// #include <PxParticleAndDiffuseBuffer.h>
// using namespace physx;
// ---------------------------------------------------------------------------

#include <cfloat>
#include <cmath>
#include <cstring>
#include <algorithm>

// ---------------------------------------------------------------------------
// Constructor / Destructor
// ---------------------------------------------------------------------------

PhysX5Solver::PhysX5Solver(int max_particles, int max_diffuse_particles)
	: m_max_particles(max_particles), m_max_diffuse(max_diffuse_particles)
{
	init_parameters();

	m_positions  .resize(max_particles * 4, 0.f);
	m_smoothed   .resize(max_particles * 4, 0.f);
	m_velocities .resize(max_particles * 3, 0.f);
	m_phases     .resize(max_particles,     0);
	m_active     .resize(max_particles,     0);
	m_lifetimes  .resize(max_particles,     0.f);
	m_aniso0     .resize(max_particles * 4, 0.f);
	m_aniso1     .resize(max_particles * 4, 0.f);
	m_aniso2     .resize(max_particles * 4, 0.f);
	m_diffuse_pos.resize(max_diffuse_particles * 4, 0.f);
	m_diffuse_vel.resize(max_diffuse_particles * 4, 0.f);

	// TODO: call init_physx() once the SDK is available.
	// init_physx();
}

PhysX5Solver::~PhysX5Solver() {
	// TODO: call shutdown_physx() once the SDK is available.
	// shutdown_physx();
}

// ---------------------------------------------------------------------------
// PhysX 5 initialisation  (stub – fill in with real API calls)
// ---------------------------------------------------------------------------
bool PhysX5Solver::init_physx() {
	// ── Step 1: Create PxFoundation ────────────────────────────────────────
	// PxFoundation is the root object for all PhysX 5 systems.
	//
	// m_foundation = PxCreateFoundation(
	//     PX_PHYSICS_VERSION,
	//     PxDefaultAllocator(),
	//     PxDefaultErrorCallback()
	// );
	// if (!m_foundation) return false;

	// ── Step 2: Create PxPhysics ───────────────────────────────────────────
	// PxPhysics manages PhysX objects (scenes, meshes, materials …).
	//
	// PxTolerancesScale tolerances;
	// m_physics = PxCreatePhysics(PX_PHYSICS_VERSION, *m_foundation,
	//                              tolerances, true, nullptr);
	// if (!m_physics) return false;
	// PxInitExtensions(*m_physics, nullptr);

	// ── Step 3: Create CUDA context manager ───────────────────────────────
	// Needed for GPU-accelerated PBD simulation (replaces NvFlex CUDA init).
	//
	// PxCudaContextManagerDesc cudaDesc;
	// m_cuda_mgr = PxCreateCudaContextManager(*m_foundation, cudaDesc,
	//                                          PxGetProfilerCallback());
	// if (!m_cuda_mgr || !m_cuda_mgr->contextIsValid()) return false;

	// ── Step 4: Create PxScene ─────────────────────────────────────────────
	// The scene holds the particle system and static collision actors.
	//
	// m_cpu_dispatch = PxDefaultCpuDispatcherCreate(2);
	// PxSceneDesc scene_desc(m_physics->getTolerancesScale());
	// scene_desc.gravity           = PxVec3(0, 0, -to_m(386.f));  // Source Engine default gravity (~9.81 m/s^2)
	// scene_desc.cpuDispatcher     = m_cpu_dispatch;
	// scene_desc.filterShader      = PxDefaultSimulationFilterShader;
	// scene_desc.cudaContextManager= m_cuda_mgr;
	// scene_desc.flags            |= PxSceneFlag::eENABLE_GPU_DYNAMICS;
	// scene_desc.broadPhaseType    = PxBroadPhaseType::eGPU;
	// m_scene = m_physics->createScene(scene_desc);
	// if (!m_scene) return false;

	// ── Step 5: Create PBD particle system ────────────────────────────────
	// PxPBDParticleSystem is the PhysX 5 equivalent of NvFlexSolver.
	// It supports fluid (SPH), cloth (springs) and granular materials.
	//
	// PxPBDParticleSystemDesc pbd_desc;
	// pbd_desc.maxParticles         = m_max_particles + m_max_diffuse;
	// pbd_desc.particleContactOffset= to_m(get_parameter("radius"));
	// pbd_desc.restOffset           = to_m(get_parameter("fluid_rest_distance"));
	// pbd_desc.fluidRestOffset      = to_m(get_parameter("fluid_rest_distance"));
	// m_particle_sys = m_physics->createPBDParticleSystem(*m_cuda_mgr, pbd_desc);
	// if (!m_particle_sys) return false;
	// m_scene->addActor(*m_particle_sys);

	// ── Step 6: Create combined fluid + diffuse particle buffer ───────────
	// PxParticleAndDiffuseBuffer manages both the primary fluid particles and
	// the secondary diffuse (foam/bubble/spray) particles.
	//
	// PxParticleBufferDesc buf_desc;
	// buf_desc.maxParticles = m_max_particles;
	// PxDiffuseParticleParams diff_params;
	// diff_params.maxDiffuseParticles = m_max_diffuse;
	// m_fluid_buf = PxCreateAndDiffuseParticleBuffer(
	//     buf_desc, diff_params, *m_cuda_mgr);
	// m_particle_sys->addParticleBuffer(m_fluid_buf);

	// ── Step 7: Shared material for collision actors ───────────────────────
	// m_material = m_physics->createMaterial(0.5f, 0.5f, 0.1f);

	return true;  // stub – always succeeds
}

void PhysX5Solver::shutdown_physx() {
	// Release in reverse creation order.
	// if (m_fluid_buf)    m_fluid_buf->release();
	// if (m_particle_sys) m_particle_sys->release();
	// if (m_material)     m_material->release();
	// if (m_scene)        m_scene->release();
	// if (m_cpu_dispatch) m_cpu_dispatch->release();
	// if (m_cuda_mgr)     m_cuda_mgr->release();
	// if (m_physics)    { PxCloseExtensions(); m_physics->release(); }
	// if (m_foundation)   m_foundation->release();
}

// ---------------------------------------------------------------------------
// Parameter table  (mirrors NvFlexParams field semantics)
// ---------------------------------------------------------------------------
void PhysX5Solver::init_parameters() {
	// Core fluid parameters
	m_params["radius"]                 = 20.f;   // particle interaction radius (inches)
	m_params["fluid_rest_distance"]    = 0.55f;  // ratio of radius
	m_params["solid_rest_distance"]    = 0.6f;
	m_params["viscosity"]              = 0.f;
	m_params["dynamic_friction"]       = 0.f;
	m_params["restitution"]            = 0.f;
	m_params["adhesion"]               = 0.f;
	m_params["cohesion"]               = 0.025f;
	m_params["surface_tension"]        = 0.f;
	m_params["buoyancy"]               = 1.f;
	m_params["gravity"]                = -386.f; // inches/s^2  (≈ 9.81 m/s^2)

	// Solver quality
	m_params["substeps"]               = 3.f;
	m_params["timescale"]              = 1.f;
	m_params["iterations"]             = 3.f;

	// Diffuse particles
	m_params["diffuse_threshold"]      = 100.f;
	m_params["diffuse_buoyancy"]       = 1.f;
	m_params["diffuse_drag"]           = 0.f;
	m_params["diffuse_ballistic"]      = 16.f;
	m_params["diffuse_lifetime"]       = 2.f;

	// Anisotropy / smoothing (rendering hints)
	m_params["anisotropy_scale"]       = 1.f;
	m_params["smoothing"]              = 1.f;

	// Reaction forces / contacts
	m_params["reaction_forces"]        = 0.f;
}

// ---------------------------------------------------------------------------
// Parameter setters / getters
// PhysX 5 uses per-property setter methods rather than a single NvFlexParams
// struct.  Each set_parameter() call maps to the appropriate PxPBDParticleSystem
// setter when the SDK is available.
// ---------------------------------------------------------------------------
bool PhysX5Solver::set_parameter(const std::string& name, float value) {
	auto it = m_params.find(name);
	if (it == m_params.end()) return false;
	it->second = value;

	if (!m_particle_sys) return true;  // SDK not yet initialised (stub)

	// ── Map parameter name to PhysX 5 API call ──────────────────────────
	// if (name == "radius") {
	//     m_particle_sys->setParticleContactOffset(to_m(value));
	// } else if (name == "fluid_rest_distance") {
	//     m_particle_sys->setFluidRestOffset(to_m(value));
	//     m_particle_sys->setRestOffset(to_m(value));
	// } else if (name == "viscosity") {
	//     m_particle_sys->setViscosity(value);
	// } else if (name == "dynamic_friction") {
	//     m_particle_sys->setFriction(value);
	// } else if (name == "surface_tension") {
	//     m_particle_sys->setSurfaceTension(value);
	// } else if (name == "cohesion") {
	//     m_particle_sys->setCohesion(value);
	// } else if (name == "adhesion") {
	//     m_particle_sys->setAdhesion(value);
	// } else if (name == "gravity") {
	//     m_scene->setGravity(PxVec3(0.f, 0.f, to_m(value)));
	// }
	// (add remaining mappings as needed)

	return true;
}

float PhysX5Solver::get_parameter(const std::string& name) const {
	auto it = m_params.find(name);
	return (it != m_params.end()) ? it->second : std::numeric_limits<float>::quiet_NaN();
}

// ---------------------------------------------------------------------------
// Particle management
// ---------------------------------------------------------------------------
bool PhysX5Solver::add_particle(const GWParticle& p) {
	if (m_active_count >= m_max_particles) return false;

	int idx = m_active_count++;
	m_active[idx] = idx;

	// Store in CPU shadow buffer (in inches – converted to metres on GPU upload)
	m_positions[idx * 4 + 0] = p.pos[0];
	m_positions[idx * 4 + 1] = p.pos[1];
	m_positions[idx * 4 + 2] = p.pos[2];
	m_positions[idx * 4 + 3] = p.pos[3];  // inv_mass

	m_velocities[idx * 3 + 0] = p.vel[0];
	m_velocities[idx * 3 + 1] = p.vel[1];
	m_velocities[idx * 3 + 2] = p.vel[2];

	m_phases[idx]   = p.phase;
	m_lifetimes[idx] = p.lifetime;

	// TODO (with SDK): upload to PxParticleBuffer on GPU
	// PxVec4* posPtr = m_fluid_buf->getPositionInvMasses();
	// posPtr[idx] = PxVec4(to_m(p.pos[0]), to_m(p.pos[1]), to_m(p.pos[2]), p.pos[3]);
	// PxVec4* velPtr = m_fluid_buf->getVelocities();
	// velPtr[idx] = PxVec4(to_m(p.vel[0]), to_m(p.vel[1]), to_m(p.vel[2]), 0.f);
	// m_fluid_buf->setNbActiveParticles(m_active_count);
	// m_fluid_buf->markDirty(PxParticleBufferFlag::eALL);

	return true;
}

void PhysX5Solver::add_cloth(const VMatrix& /*transform*/, Vector2D /*size*/, const GWParticle& /*particle*/) {
	// TODO: create a PxParticleBuffer segment with PxParticleSpring constraints.
	//
	// In PhysX 5 cloth is represented by normal PBD particles that are connected
	// by distance springs (analogous to NvFlex spring constraints):
	//
	//   PxParticleSpring spring;
	//   spring.ind0      = particleA;
	//   spring.ind1      = particleB;
	//   spring.length    = restLength;
	//   spring.stiffness = 1.f;
	//   spring.damping   = 0.f;
	//   spring.pad       = 0.f;
	//   m_fluid_buf->addSprings(&spring, 1);
	//
	// Triangle data for rendering normals is stored separately.
}

void PhysX5Solver::add_force_field(const GWForceField& /*field*/) {
	// TODO: PhysX 5 does not have a direct force-field API equivalent to
	// NvFlexExtForceField.  Options:
	//
	//   a) Custom CUDA kernel attached via PxScene::setSimulationEventCallback
	//      that applies per-particle forces during the substep callback.
	//   b) Use PxArticulationMimicJoint or user force buffers
	//      (PxParticleBuffer user data channel).
	//
	// For the prototype, force fields are queued but not applied.
}

// ---------------------------------------------------------------------------
// Simulation tick
// ---------------------------------------------------------------------------
bool PhysX5Solver::tick(float dt) {
	float timescale = get_parameter("timescale");
	dt *= timescale;
	if (dt <= 0.f || m_active_count == 0) return false;

	// TODO (with SDK):
	// m_scene->simulate(dt);
	// m_scene->fetchResults(true /* block */);
	// sync_from_gpu();

	return true;
}

// Copies GPU particle data back to the CPU shadow buffers.
void PhysX5Solver::sync_from_gpu() {
	// TODO (with SDK):
	//
	// PxVec4* px_pos = m_fluid_buf->getPositionInvMasses();
	// PxVec4* px_vel = m_fluid_buf->getVelocities();
	// int n = m_fluid_buf->getNbActiveParticles();
	// for (int i = 0; i < n; i++) {
	//     m_positions[i*4+0] = to_inch(px_pos[i].x);
	//     m_positions[i*4+1] = to_inch(px_pos[i].y);
	//     m_positions[i*4+2] = to_inch(px_pos[i].z);
	//     m_positions[i*4+3] = px_pos[i].w;
	//     m_velocities[i*3+0] = to_inch(px_vel[i].x);
	//     m_velocities[i*3+1] = to_inch(px_vel[i].y);
	//     m_velocities[i*3+2] = to_inch(px_vel[i].z);
	// }
	// m_active_count = n;
	//
	// Diffuse:
	// PxDiffuseParticleParams* diff = m_fluid_buf->getDiffuseParticles();
	// m_diffuse_count = (int)m_fluid_buf->getNbDiffuseParticles();
	// ...
}

// ---------------------------------------------------------------------------
// State reset
// ---------------------------------------------------------------------------
void PhysX5Solver::reset() {
	m_active_count  = 0;
	m_diffuse_count = 0;
	m_tri_count     = 0;
	std::fill(m_lifetimes.begin(), m_lifetimes.end(), 0.f);

	// TODO (with SDK):
	// m_fluid_buf->setNbActiveParticles(0);
	// m_fluid_buf->setNbActiveDiffuseParticles(0);
	// m_fluid_buf->markDirty(PxParticleBufferFlag::eALL);
}

void PhysX5Solver::reset_cloth() {
	m_tri_count = 0;
	// Mark cloth particles as expired (zero lifetime).
}

void PhysX5Solver::reset_diffuse() {
	m_diffuse_count = 0;
	// TODO (with SDK): m_fluid_buf->setNbActiveDiffuseParticles(0);
}

// ---------------------------------------------------------------------------
// Queries
// ---------------------------------------------------------------------------
int PhysX5Solver::get_active_particles()      const { return m_active_count; }
int PhysX5Solver::get_max_particles()         const { return m_max_particles; }
int PhysX5Solver::get_active_diffuse()        const { return m_diffuse_enabled ? m_diffuse_count : 0; }
int PhysX5Solver::get_max_diffuse_particles() const { return m_max_diffuse; }
int PhysX5Solver::get_active_triangles()      const { return m_tri_count; }

// ---------------------------------------------------------------------------
// Collision geometry
// ---------------------------------------------------------------------------
GWMeshHandle PhysX5Solver::add_collision_mesh(const Vector* verts, int num_verts,
                                               bool /*dynamic*/, bool convex,
                                               int entity_id) {
	PhysX5Mesh mesh;
	mesh.entity_id = entity_id;
	mesh.enabled   = true;
	mesh.actor     = nullptr;  // TODO (with SDK):

	// ── Build triangle or convex mesh ──────────────────────────────────────
	// if (convex) {
	//     PxConvexMeshDesc cdesc;
	//     cdesc.points.count  = (PxU32)num_verts;
	//     cdesc.points.stride = sizeof(PxVec3);
	//     // fill cdesc.points.data with metre-scaled vertices
	//     cdesc.flags = PxConvexFlag::eCOMPUTE_CONVEX;
	//     PxDefaultMemoryOutputStream buf;
	//     m_physics->getCooking().cookConvexMesh(cdesc, buf);
	//     // create PxConvexMesh from buf …
	//     PxShape* shape = m_physics->createShape(PxConvexMeshGeometry(cmesh), *m_material);
	//     mesh.actor = m_physics->createRigidStatic(PxTransform(PxIdentity));
	//     mesh.actor->attachShape(*shape);
	//     m_scene->addActor(*mesh.actor);
	// } else {
	//     // PxTriangleMeshDesc / cookTriangleMesh path …
	// }

	int handle = (int)m_meshes.size();
	m_meshes.push_back(mesh);
	return handle;
}

void PhysX5Solver::remove_collision_mesh(int entity_id) {
	for (auto& m : m_meshes) {
		if (m.entity_id == entity_id) {
			// TODO (with SDK): m_scene->removeActor(*m.actor); m.actor->release();
			m.actor = nullptr;
		}
	}
}

void PhysX5Solver::set_mesh_position(GWMeshHandle handle, const Vector& pos) {
	if (handle < 0 || handle >= (int)m_meshes.size()) return;
	// TODO (with SDK):
	// PxTransform t = m_meshes[handle].actor->getGlobalPose();
	// t.p = PxVec3(to_m(pos.x), to_m(pos.y), to_m(pos.z));
	// m_meshes[handle].actor->setGlobalPose(t);
}

void PhysX5Solver::set_mesh_rotation(GWMeshHandle handle, const QAngle& /*ang*/) {
	if (handle < 0 || handle >= (int)m_meshes.size()) return;
	// TODO (with SDK): convert QAngle → PxQuat and update actor pose.
}

void PhysX5Solver::set_mesh_enabled(GWMeshHandle handle, bool enabled) {
	if (handle < 0 || handle >= (int)m_meshes.size()) return;
	m_meshes[handle].enabled = enabled;
	// TODO (with SDK): PxShape flags or actor scene add/remove.
}

// ---------------------------------------------------------------------------
// World bounds
// ---------------------------------------------------------------------------
void PhysX5Solver::enable_bounds(const Vector& mins, const Vector& maxs) {
	// TODO (with SDK): add six PxPlane actors to the scene defining the AABB.
	// (Equivalent to NvFlex's NvFlexParams.planes array.)
	(void)mins; (void)maxs;
}

void PhysX5Solver::disable_bounds() {
	// TODO (with SDK): remove the six PxPlane actors.
}

// ---------------------------------------------------------------------------
// Rendering data
// ---------------------------------------------------------------------------
GWParticleData PhysX5Solver::get_particle_data() const {
	GWParticleData d{};
	d.positions     = m_positions.data();
	d.smoothed_pos  = m_smoothed.data();
	d.velocities    = m_velocities.data();
	d.phases        = m_phases.data();
	d.active_indices= m_active.data();
	d.anisotropy0   = m_aniso0.data();
	d.anisotropy1   = m_aniso1.data();
	d.anisotropy2   = m_aniso2.data();
	d.active_count  = m_active_count;
	d.max_count     = m_max_particles;
	return d;
}

GWDiffuseData PhysX5Solver::get_diffuse_data() const {
	GWDiffuseData d{};
	d.positions = m_diffuse_pos.data();
	d.velocities = m_diffuse_vel.data();
	d.count      = m_diffuse_enabled ? m_diffuse_count : 0;
	return d;
}

GWClothData PhysX5Solver::get_cloth_data() const {
	GWClothData d{};
	d.normals        = m_tri_normals.data();
	d.indices        = m_tri_indices.data();
	d.triangle_count = m_tri_count;
	return d;
}

// ---------------------------------------------------------------------------
// Misc
// ---------------------------------------------------------------------------
void PhysX5Solver::enable_diffuse(bool enabled) {
	m_diffuse_enabled = enabled;
}
