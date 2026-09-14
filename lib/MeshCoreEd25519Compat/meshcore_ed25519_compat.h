#pragma once
// Nothing to declare here -- the real ed25519 API comes from MeshCore's own
// vendored lib/ed25519/ed_25519.h (see platformio.ini's -I flag). This
// header exists only so something in the build (HasMeshCoreChat.h) can
// #include it: PlatformIO's default LDF mode discovers lib/ folders by
// following #include graphs from src/, not by scanning lib/ wholesale, so
// without this edge the meshcore_ed25519_*.c translation units in this
// folder are silently never compiled at all -- confirmed by a real build
// (ed25519_create_keypair/ed25519_key_exchange undefined at link time)
// despite the exact same files compiling fine once this edge exists.
