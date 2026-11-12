# physics_RT

The host integrates the numerical and geometry work from
[physics_RT `81fe77d`](https://github.com/doyaGu/physics_RT/tree/81fe77d86609e7436424642c838e81ec6b58e5f4)
and records the IVP revision through its submodule gitlink.
Imported sources and fixtures retain the upstream license in `UPSTREAM-LICENSE`;
IVP and its numerical dependency retain their own source licenses.

## Validate in the original installation

The normal module imports the local CK2/VxMath ABI. To run these physics
sources with the original runtime, use the separate SDK validation
project. Import libraries and headers must come from the original Virtools
2.1 SDK; the original installation supplies its existing runtime DLLs.

```powershell
cmake -S Source/BuildingBlocks/physics_RT/tests/retail-host -B build/physics-original `
    -G "Visual Studio 17 2022" -A Win32 `
    -DPHYSICS_RT_RETAIL_SDK_ROOT=C:/path/to/Virtools-SDK-2.1
cmake --build build/physics-original --config Release --target physics_RT_retail_validation
```

The resulting `Release/physics_RT.dll` is for original-runtime validation.
Use the original runtime and application data from the selected installation
for reference launches and replays. Snapshot any temporarily replaced test
modules and verify their hashes after restoration.
Do not deploy the ordinary module into that original runtime: its C++
imports differ even though the public plugin entry points have the same names.

## Floating-point evaluation and storage

Enable `IVP_ENABLE_X87_EMULATION=ON` for the original evaluation and write-back
rules. It defaults to `ON`. The engine also enables
`VXMATH_ENABLE_X87_EMULATION=ON`; both settings are used for original replay
acceptance. The original Ipion convex hull builder remains the default.

On MSVC Win32, IVP propagates `/arch:IA32` to preserve x87 intermediate
precision and floating-point return values. VxMath owns portable extended
arithmetic through ordinary structures and functions, with the existing SIMD
abstraction for binary32 matrix work.
The non-SIMD implementation evaluates the same operations in software. Native
IVP and VxMath share the existing FPU control-word API across platforms.

The integration preserves the named collision-surface cache, cached
respawn state, parent/world scale handling, multiball shapes, double-anchor
ball joints, contact bounds checks, behavior-ID lookup and explicit resource
ownership. It also preserves the local negative-time-factor guard, buoyancy
strength/idempotence handling, missing-reference guards and profiler units.
World scale is captured once per physicalization and reused while publishing
poses, so rounded rotation matrices cannot feed scale drift into later updates.
Axis lengths within two float epsilons of one are treated as unit scale so
unscaled rotations keep the original physics pose matrix.
The reference's collision-position/normal mistakes and raw behavior pointers
are not imported.

Callback cancellation covers executing callbacks and pending batches. A
callback can clear its own behavior safely; Reset runs after callback execution
has unwound. Object deletion invalidates dependent BB handles, including spring
handles whose IVP anchors have already been destroyed. Continuous-contact
setting changes release the old listener and pending contact records.

## Build and test the local host

The focused validation project builds this module against the local CK2,
VxMath and static RenderEngine without building a GPU rasterizer. It requires
the NeMo2 component checkout and an SDL3 3.4.20+ package for the selected
architecture. All builds must be out of source.

```powershell
$sdl3 = 'C:/path/to/SDL3/cmake'
cmake -S Source/BuildingBlocks/physics_RT/tests/host -B build/physics-release `
    -G "Visual Studio 17 2022" -A Win32 "-DSDL3_DIR=$sdl3" -DIVP_BUILD_TESTS=ON
cmake --build build/physics-release --config Release
ctest --test-dir build/physics-release -C Release --output-on-failure --no-tests=error
```

Use a separate directory with `-A x64` for an x64 build. Internal diagnostics
remain off. Original-runtime replay qualification is recorded separately for
each architecture.
For a different component layout, set `PHYSICS_RT_COMPONENT_ROOT`
to the directory containing the four local component repositories.

The NeMo2 build also registers host tests when `CKBB_BUILD_TESTS`,
`BUILD_TESTING` and `CKRE_BUILD_STATIC` are enabled. The host suite exercises
the input/output ports and parameters of all 15 building blocks, behavior
execution, float rounding, matrix conversion, caching, scale, state access,
callbacks and real teardown.
IVP's 11 core tests and two extended-arithmetic tests are registered separately.
Windows shared builds additionally test loading the actual module and its
plugin/state/diagnostic exports.

## Original-image comparisons

Configure a Win32 MSVC validation directory with:

```powershell
cmake -S Source/BuildingBlocks/physics_RT/tests/host -B build/physics-diagnostics `
    -G "Visual Studio 17 2022" -A Win32 "-DSDL3_DIR=$sdl3" `
    -DIVP_BUILD_TESTS=ON -DPHYSICS_RT_ENABLE_DIAGNOSTICS=ON `
    -DPHYSICS_RT_BUILD_PARITY_TESTS=ON `
    -DPHYSICS_RT_RETAIL_DLL=C:/path/to/original/BuildingBlocks/physics_RT.dll `
    -DPHYSICS_RT_RETAIL_BIN=C:/path/to/original/Bin
cmake --build build/physics-diagnostics --config Release
ctest --test-dir build/physics-diagnostics -C Release --output-on-failure --no-tests=error
```

The scenarios in `tests/retail_scenarios.inc` cover mathematics, geometry, dynamics, contact, pressure and
continuous collision. The harness verifies the original image SHA-256 before
using its audited addresses. Candidate numerical entry points are statically
linked into the harness, so loading the original image does not resolve a
candidate DLL's imports against the original CK2/VxMath. This requires
`CKBB_BUILD_STATIC=ON`; the focused project enables it automatically.
These tests do not require committing original binaries or game assets.

Release builds omit the internal diagnostic sources and exports. Diagnostics
and the original-image harness require MSVC Win32 because their layouts,
member-function addresses and assembly follow that ABI. Enabling diagnostics
does not install runtime hooks. Ordinary arithmetic is tested independently;
the extended-arithmetic bitwise comparisons are not its acceptance criterion.

## Versioned state access

Resolve `PhysicsRT_GetStateApi` dynamically and request version 1. The table
in `PhysicsState.h` is owned by the module; check `size` and `version` before
using it. Unsupported versions return null. The table uses native pointer
width and the platform calling convention declared in that header.

Manager/entity arguments must be live handles from the same host. Perform
state changes on the simulation thread, outside callbacks that may destroy the
world. The interface does not expose reconstructed class layouts.

| Operation | Semantics |
| --- | --- |
| `reset_time` | Reset base/current/last PSI to zero, next PSI to `1/66` seconds, and directly write smoothed delta in milliseconds |
| `read_baseline`, `restore_baseline` | Read/restore the raw time factor, movement-check countdown and shared random seed |
| `set_time_factor` | Write the raw internal time factor; unlike the Globals BB setter it does not multiply by `0.001` |
| `read_velocity` | Include pending linear/angular impulses; either output may be null; return zero for an unphysicalized entity |
| `is_sleeping` | Report a calm core; return zero for an unphysicalized entity |
| `set_seed`, `read_seed`, `random` | Access IVP's shared random generator |

Clock/baseline restoration does not reconstruct bodies, controllers or contacts
and is not a complete world snapshot. Baseline output pointers must be non-null.
Internal diagnostic transports and raw solver addresses are specific to the
pinned IVP implementation and are excluded from the release interface.
