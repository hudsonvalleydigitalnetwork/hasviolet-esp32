//
// Meshtastic_RadioConfig.h
//
// Plain data + two small helpers needed for Meshtastic PHY-layer parity
// (region/preset tables, frequency-slot selection). No protobuf/nanopb
// dependency -- that's Phase 2 (packet framing/encryption), not this file.
//
// Every value here is transcribed from development/meshtastic/MESHTASTIC-SPEC.md
// (itself pulled from a pinned meshtastic/firmware commit, see that doc's header
// for the exact commit/tag) -- do not hand-edit a number here without updating
// the spec doc's source-of-truth first.
//
// Only included where MESHTASTIC_PHY_TEST is defined (see main.cpp); classic-mode
// builds never see this file at all.

#pragma once

#include <Arduino.h>
#include <string.h>

// --- Regions (spec doc S2 / meshtastic/firmware RadioInterface.cpp regions[]) ---
//
// All 27 Meshtastic RegionCode entries, not just the 7 overlapping this
// project's 433-510MHz test hardware -- region is a user-set field per
// project_state.md, so the full table belongs here even though Phase 1 only
// exercises one of them.

struct MeshtasticRegion {
  const char *name;
  float freqStart;    // MHz
  float freqEnd;      // MHz
  float spacing;       // MHz, extra gap between channels (0 for all current regions)
  int8_t powerLimit;   // dBm, regulatory ceiling (0 = no limit recorded)
};

static const MeshtasticRegion MESHTASTIC_REGIONS[] = {
  {"US",        902.0f,  928.0f,  0.0f, 30},
  {"EU_433",    433.0f,  434.0f,  0.0f, 10},
  {"EU_868",    869.4f,  869.65f, 0.0f, 27},
  {"CN",        470.0f,  510.0f,  0.0f, 19},
  {"JP",        920.5f,  923.5f,  0.0f, 13},
  {"ANZ",       915.0f,  928.0f,  0.0f, 30},
  {"ANZ_433",   433.05f, 434.79f, 0.0f, 14},
  {"RU",        868.7f,  869.2f,  0.0f, 20},
  {"KR",        920.0f,  923.0f,  0.0f, 23},
  {"TW",        920.0f,  925.0f,  0.0f, 27},
  {"IN",        865.0f,  867.0f,  0.0f, 30},
  {"NZ_865",    864.0f,  868.0f,  0.0f, 36},
  {"TH",        920.0f,  925.0f,  0.0f, 27},
  {"UA_433",    433.0f,  434.7f,  0.0f, 10},
  {"UA_868",    868.0f,  868.6f,  0.0f, 14},
  {"MY_433",    433.0f,  435.0f,  0.0f, 20},
  {"MY_919",    919.0f,  924.0f,  0.0f, 27},
  {"SG_923",    917.0f,  925.0f,  0.0f, 20},
  {"PH_433",    433.0f,  434.7f,  0.0f, 10},
  {"PH_868",    868.0f,  869.4f,  0.0f, 14},
  {"PH_915",    915.0f,  918.0f,  0.0f, 24},
  {"KZ_433",    433.075f,434.775f,0.0f, 10},
  {"KZ_863",    863.0f,  868.0f,  0.0f, 30},
  {"NP_865",    865.0f,  868.0f,  0.0f, 30},
  {"BR_902",    902.0f,  907.5f,  0.0f, 30},
  {"LORA_24",   2400.0f, 2483.5f, 0.0f, 10},
  {"UNSET",     902.0f,  928.0f,  0.0f, 30},
};
static const size_t MESHTASTIC_REGION_COUNT = sizeof(MESHTASTIC_REGIONS) / sizeof(MESHTASTIC_REGIONS[0]);

// --- Modem presets (spec doc S3 / MeshRadio.h modemPresetToParams(), non-wideLora column) ---
//
// wideLora (500kHz-class LR11xx hardware) doesn't apply to this project's
// SX127x/SX126x boards, so only the standard column is reproduced here.

struct MeshtasticPreset {
  const char *name;
  float bwKHz;
  uint8_t sf;
  uint8_t cr;   // denominator: 4/cr
};

static const MeshtasticPreset MESHTASTIC_PRESETS[] = {
  {"ShortTurbo",    500.0f,  7,  5},
  {"ShortFast",     250.0f,  7,  5},
  {"ShortSlow",     250.0f,  8,  5},
  {"MediumFast",    250.0f,  9,  5},
  {"MediumSlow",    250.0f, 10,  5},
  {"LongTurbo",     500.0f, 11,  8},
  {"LongModerate",  125.0f, 11,  8},
  {"LongSlow",      125.0f, 12,  8},
  {"LongFast",      250.0f, 11,  5},   // default
};
static const size_t MESHTASTIC_PRESET_COUNT = sizeof(MESHTASTIC_PRESETS) / sizeof(MESHTASTIC_PRESETS[0]);

// --- Lookups by name, so call sites don't hardcode array indices ---

static inline const MeshtasticRegion &meshtasticFindRegion(const char *name) {
  for (size_t i = 0; i < MESHTASTIC_REGION_COUNT; i++)
    if (strcmp(MESHTASTIC_REGIONS[i].name, name) == 0) return MESHTASTIC_REGIONS[i];
  return MESHTASTIC_REGIONS[MESHTASTIC_REGION_COUNT - 1];   // "UNSET"
}

static inline const MeshtasticPreset &meshtasticFindPreset(const char *name) {
  for (size_t i = 0; i < MESHTASTIC_PRESET_COUNT; i++)
    if (strcmp(MESHTASTIC_PRESETS[i].name, name) == 0) return MESHTASTIC_PRESETS[i];
  return MESHTASTIC_PRESETS[MESHTASTIC_PRESET_COUNT - 1];   // "LongFast"
}

// --- djb2 hash (spec doc S4 / RadioInterface.cpp hash()) ---
//
// Picks the frequency slot from a channel's display name. NOT the same as
// the xorHash channel-hash byte in the packet header (Phase 2 concern) --
// don't conflate the two, see spec doc S4's warning.

static inline uint32_t meshtasticDjb2Hash(const char *str) {
  uint32_t hash = 5381;
  int c;
  while ((c = *str++) != 0)
    hash = ((hash << 5) + hash) + (unsigned char)c;   // hash * 33 + c
  return hash;
}

// --- Frequency selection (spec doc S4 / RadioInterface.cpp setupHardware()) ---
//
// channelName should be the channel's *display* name -- for the default/empty
// channel with use_preset true, that's the preset's own display name (e.g.
// "LongFast"), per spec doc S6, not literally "".

static inline float meshtasticFrequency(const MeshtasticRegion &region, float bwKHz, const char *channelName) {
  uint32_t numChannels = (uint32_t)floor((region.freqEnd - region.freqStart) / (region.spacing + bwKHz / 1000.0f));
  uint32_t channelNum = meshtasticDjb2Hash(channelName) % numChannels;
  return region.freqStart + (bwKHz / 2000.0f) + channelNum * (bwKHz / 1000.0f);
}
