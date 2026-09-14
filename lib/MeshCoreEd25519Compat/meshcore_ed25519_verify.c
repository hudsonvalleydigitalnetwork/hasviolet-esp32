// Pass-through translation unit: compiles MeshCore's own vendored ed25519
// reference implementation (lib/ed25519/verify.c in meshcore-dev/MeshCore,
// fetched unmodified by PlatformIO's lib_deps -- see platformio.ini) as
// part of this project's build. MeshCore's own build_as_lib.py SRC_FILTER
// only compiles its src/ tree, not its sibling lib/ directory, when
// consumed as a plain lib_deps entry rather than by cloning the whole
// repo -- this (and the matching -I flag in platformio.ini) is how that
// gap gets closed without copying any of MeshCore's source into this repo.
// One file per source, each its own translation unit, to avoid any
// unity-build symbol-collision risk across files not designed to be
// concatenated. Deliberately NOT named "verify.c" -- a quoted #include always
// checks the including file's own directory first, so naming this file
// the same as the target would just re-include itself instead of falling
// through to the -I path where the real one lives.
//
// PlatformIO's LDF discovers this library via a static #include scan
// of HasMeshCoreChat.h, which does not respect that file's own #ifdef
// HASV_MESHCORE_SUPPORT guard -- so this translation unit still gets
// compiled (to nothing, below) on every board, not just the one(s)
// that actually vendor MeshCore and have the matching -I flag for the
// real source file this #include resolves to.
#ifdef HASV_MESHCORE_SUPPORT
#include "verify.c"
#endif
