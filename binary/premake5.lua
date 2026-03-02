PROJECT_GENERATOR_VERSION = 3	-- 3 = 64 bit support

newoption({
	trigger = "gmcommon",
	description = "Sets the path to the garrysmod_common (https://github.com/danielga/garrysmod_common) directory",
	value = "./garrysmod_common",
	default = "./garrysmod_common"
})

-- ── Backend selection ──────────────────────────────────────────────────────
-- Controls which physics simulation library is compiled into the module.
--
--   flex    (default) – Nvidia FleX, the original GWater2 backend.
--                       Requires: binary/FleX submodule.
--
--   physx5            – Nvidia PhysX 5 PBD particle system (PROTOTYPE).
--                       Requires: binary/PhysX5 (https://github.com/NVIDIA-Omniverse/PhysX)
--                       Clone with:  git clone https://github.com/NVIDIA-Omniverse/PhysX binary/PhysX5
--                       Then build the SDK and set PHYSX5_PATH to its output dir.
newoption({
	trigger = "backend",
	description = "Fluid simulation backend to compile (flex | physx5)",
	value = "flex",
	default = "flex",
	allowed = {
		{ "flex",   "Nvidia FleX (default)" },
		{ "physx5", "Nvidia PhysX 5 PBD particle system (prototype)" },
	}
})

-- ── Renderer selection ─────────────────────────────────────────────────────
-- Controls which rendering pipeline is compiled into the module.
--
--   source  (default) – Existing Source Engine IMesh renderer using custom HLSL
--                       shaders injected into the material system.
--
--   vulkan            – External Vulkan screen-space fluid renderer (PROTOTYPE).
--                       Renders particles off-screen via Vulkan and composites
--                       the result back into Source Engine via a shared texture
--                       (DX11/Vulkan interop on Windows; DMA-buf on Linux).
--                       Requires: Vulkan SDK (https://vulkan.lunarg.com/).
newoption({
	trigger = "renderer",
	description = "Fluid rendering pipeline to compile (source | vulkan)",
	value = "source",
	default = "source",
	allowed = {
		{ "source", "Source Engine IMesh renderer (default)" },
		{ "vulkan", "External Vulkan screen-space renderer (prototype)" },
	}
})

local backend  = _OPTIONS.backend  or "flex"
local renderer = _OPTIONS.renderer or "source"

local gmcommon = assert(_OPTIONS.gmcommon or os.getenv("GARRYSMOD_COMMON"),
	"you didn't provide a path to your garrysmod_common (https://github.com/danielga/garrysmod_common) directory")

include(gmcommon)

CreateWorkspace({name = "gwater2", abi_compatible = true, path = ""})
	--CreateProject({serverside = true, source_path = "source", manual_files = false})
	--	IncludeLuaShared()
	--	IncludeScanning()
	--	IncludeDetouring()
	--	IncludeSDKCommon()
	--	IncludeSDKTier0()
	--	IncludeSDKTier1()

	CreateProject({serverside = false, source_path = "src"})
		IncludeLuaShared()
		IncludeScanning()
		--IncludeDetouring()
		--IncludeSteamAPI()
		IncludeSDKCommon()
		IncludeSDKTier0()
		IncludeSDKTier1()
		IncludeSDKMathlib()

		includedirs {
			"BSPParser",
			"GMFS",
			"src/sourceengine",
			"ThreadPool"
		}

		files {
			"BSPParser/**",
			"GMFS/**",
			"src/sourceengine/*",
			"src/shaders/*"
		}

		-- ── Backend: FleX (default) ───────────────────────────────────────────
		if backend == "flex" then
			defines { "GWATER2_BACKEND_FLEX" }
			includedirs { "FleX/include" }

			files {
				"src/flex_solver.cpp",
				"src/flex_mesh.cpp",
				"src/flex_renderer.cpp",
			}

			filter({"system:windows", "platforms:x86"})
				libdirs { "FleX/lib/win32" }
				links   { "NvFlexReleaseD3D_x86", "NvFlexExtReleaseD3D_x86" }

			filter({"system:windows", "platforms:x86_64"})
				libdirs { "FleX/lib/win64" }
				links   { "NvFlexReleaseD3D_x64", "NvFlexExtReleaseD3D_x64" }

			filter({"system:linux", "platforms:x86_64"})
				libdirs { "FleX/lib/linux64", "src/cuda" }
				links   {
					":NvFlexReleaseCUDA_x64.a",
					":NvFlexExtReleaseCUDA_x64.a",
					"cudart_static",
				}

		-- ── Backend: PhysX 5 (prototype) ─────────────────────────────────────
		elseif backend == "physx5" then
			defines { "GWATER2_BACKEND_PHYSX5" }

			-- PhysX 5 SDK path: clone https://github.com/NVIDIA-Omniverse/PhysX
			-- into binary/PhysX5, build it, then update the paths below.
			local px5_root    = "PhysX5/physx"
			local px5_include = px5_root .. "/include"
			local px5_lib_win = px5_root .. "/bin/win.x86_64.vc143.mt/release"
			local px5_lib_lnx = px5_root .. "/bin/linux.clang/release"

			includedirs { px5_include }

			files {
				"src/physx5_solver.cpp",
				"src/flex_mesh.cpp",
			}

			filter({"system:windows"})
				libdirs { px5_lib_win }
				links   {
					"PhysX_64", "PhysXCommon_64", "PhysXFoundation_64",
					"PhysXCooking_64",
					"PhysXExtensions_static_64",
					"PhysXPvdSDK_static_64",
				}

			filter({"system:linux"})
				libdirs { px5_lib_lnx }
				links   {
					"PhysX_static_64", "PhysXCommon_static_64",
					"PhysXFoundation_static_64", "PhysXCooking_static_64",
					"PhysXExtensions_static_64",
				}
		end

		-- ── Renderer: Source Engine (default) ─────────────────────────────────
		if renderer == "source" then
			defines { "GWATER2_RENDERER_SOURCE" }
			if backend ~= "flex" then
				files { "src/flex_renderer.cpp" }
			end

		-- ── Renderer: Vulkan (prototype) ──────────────────────────────────────
		elseif renderer == "vulkan" then
			defines { "GWATER2_RENDERER_VULKAN" }

			local vk_sdk = os.getenv("VULKAN_SDK") or "$(VULKAN_SDK)"

			files { "src/vulkan_renderer.cpp" }

			filter({"system:windows"})
				includedirs { vk_sdk .. "/Include" }
				libdirs     { vk_sdk .. "/Lib" }
				links       { "vulkan-1" }

			filter({"system:linux"})
				links { "vulkan" }
		end

		-- Reset filter so the per-platform suffix filters below are clean
		filter {}

		filter({"system:windows", "platforms:x86"})
			targetsuffix("_win32")

		filter({"system:windows", "platforms:x86_64"})
			targetsuffix("_win64")

		filter({"system:linux", "platforms:x86_64"})
			targetsuffix("_linux64")

			includedirs {
				--"/usr/local/cuda-9.2/include",
				--"/usr/local/cuda-9.2/extras/cupti/include",
			}

			defines {
				"DX_TO_GL_ABSTRACTION",
				"IsPlatformOpenGL()"
			}
