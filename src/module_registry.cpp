#include "soundgrind/engine.hpp"

namespace soundgrind {
std::vector<TestModule> registered_modules() {
  return {
    {"clap.lifecycle", "1.0.0", "Load, initialize, activate, process, deactivate and destroy the CLAP target.", {"clap"}},
    {"audio.finite_output", "1.0.0", "Fail on non-finite output samples; retain channel/sample evidence.", {"audio_output"}},
    {"fixture.oracle", "1.0.0", "Compare the deterministic synthetic fixture against its documented sample oracle.", {"audio_input", "audio_output", "fixture_oracle"}},
    {"lifecycle.repeat", "1.0.0", "Repeat activate/process/deactivate cycles and record each cycle.", {"clap", "audio_output"}},
    {"params.readback", "1.0.0", "Apply a legal parameter event and compare effective readback when available.", {"params"}},
    {"params.boundaries", "1.0.0", "Exercise declared default, minimum, maximum and stepped/enum values.", {"params"}},
    {"params.normalized_mapping", "1.0.0", "Check SoundGrind's documented linear normalized/plain conversion at endpoints and midpoint.", {"params"}},
    {"processing.block_sizes", "1.0.0", "Process legal configured blocks and account for whole-block dwell rounding.", {"audio_output"}},
    {"state.roundtrip", "1.0.0", "Save and restore plugin state when CLAP state streams are advertised.", {"state"}}
  };
}
}
