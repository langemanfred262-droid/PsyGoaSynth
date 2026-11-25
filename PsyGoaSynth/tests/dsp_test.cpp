// Standalone DSP sanity test (no JUCE needed):
//   g++ -std=c++17 -O2 -fsanitize=address,undefined tests/dsp_test.cpp -o dsp_test && ./dsp_test
#include "../Source/dsp/FMSynth.h"
#include "../Source/dsp/AcidFilter.h"
#include "../Source/dsp/TranceGate.h"
#include "../Source/dsp/ModFx.h"
#include "../Source/dsp/PingPongDelay.h"
#include <cstdio>
#include <vector>
using namespace psy;

static int fails = 0;
#define CHECK(c, msg) do { if (!(c)) { std::printf("FAIL: %s\n", msg); ++fails; } } while (0)

int main()
{
    const double sr = 48000.0; const int block = 256; const int blocks = 600;
    const double bpm = 145.0, bps = bpm / 60.0 / sr;

    CHECK(quantizeToScale(61, 0, 2) == 61, "phrygian dominant keeps C#");
    CHECK(quantizeToScale(62, 0, 2) == 61, "D snaps down to C# in C phryg-dom");
    CHECK(quantizeToScale(60, 0, 0) == 60, "chromatic untouched");

    for (int mode = 0; mode < 4; ++mode)
    {
        Synth syn; AcidFilter filt; TranceGate gate; Phaser ph; Flanger fl; PingPongDelay dl;
        syn.prepare(sr); filt.prepare(sr); gate.prepare(sr); ph.prepare(sr); fl.prepare(sr); dl.prepare(sr);

        SynthParams sp; sp.sawMix = 0.6f; sp.subMix = 0.4f; sp.fmFb = 0.5f; sp.index = 8; sp.mono = (mode & 1); sp.glideMs = 60; sp.scale = 2;
        syn.setParams(sp);
        AcidParams ap; ap.type = mode; ap.reso = 1.0f; ap.drive = 1.0f; ap.envAmt = 1.0f; ap.lfoDepth = 1.0f; ap.cutoff = 300; filt.setParams(ap);
        GateParams gp; gp.on = true; gp.pattern = 0x5555; gp.depth = 1.0f; gate.setParams(gp);
        PhaserParams pp; pp.on = true; pp.fb = 0.95f; pp.sync = true; ph.setParams(pp);
        FlangerParams fp; fp.on = true; fp.fb = 0.95f; fl.setParams(fp);
        DelayParams dp; dp.on = true; dp.fb = 0.95f; dp.mix = 1.0f; dl.setParams(dp);

        std::vector<float> L(block), R(block);
        double beat = 0; float peak = 0; double energy = 0; bool finite = true;
        int notes[4] = { 36, 48, 51, 55 };
        for (int b = 0; b < blocks; ++b)
        {
            if (b % 40 == 0) { syn.noteOn(notes[(b / 40) % 4], 0.9f); filt.triggerEnv(0.9f); }
            if (b % 40 == 30) syn.noteOff(notes[(b / 40) % 4]);
            std::fill(L.begin(), L.end(), 0.f); std::fill(R.begin(), R.end(), 0.f);
            syn.render(L.data(), R.data(), block);
            filt.process(L.data(), R.data(), block, beat, bps);
            gate.process(L.data(), R.data(), block, beat, bps);
            ph.process(L.data(), R.data(), block, beat, bps);
            fl.process(L.data(), R.data(), block, beat, bps);
            dl.process(L.data(), R.data(), block, bpm);
            for (int i = 0; i < block; ++i) {
                if (!std::isfinite(L[i]) || !std::isfinite(R[i])) finite = false;
                peak = std::max(peak, std::max(std::fabs(L[i]), std::fabs(R[i])));
                energy += L[i] * L[i] + R[i] * R[i];
            }
            beat += block * bps;
        }
        std::printf("filter type %d, mono=%d: peak %.3f  rms %.4f  finite=%d\n",
                    mode, mode & 1, peak, std::sqrt(energy / (2.0 * block * blocks)), (int)finite);
        CHECK(finite, "output must be finite");
        CHECK(peak < 12.0f, "output must stay bounded");
        CHECK(energy > 1e-3, "output must not be silent");
    }

    {   // gate: closed step silent, open step passes
        TranceGate g; g.prepare(sr);
        GateParams gp; gp.on = true; gp.div = 2; gp.pattern = 0b10; gp.len = 2; gp.depth = 1; gp.smoothMs = 0.2f; g.setParams(gp);
        const int n = (int)(sr * 60.0 / 120.0 * 0.25);
        std::vector<float> a(n * 2, 1.f), b(n * 2, 1.f);
        g.process(a.data(), b.data(), n * 2, 0.0, 120.0 / 60.0 / sr);
        CHECK(std::fabs(a[n / 2]) < 0.05f, "step 0 closed");
        CHECK(std::fabs(a[n + n / 2]) > 0.95f, "step 1 open");
    }
    std::printf(fails ? "\nTESTS FAILED\n" : "\nALL DSP TESTS PASSED\n");
    return fails ? 1 : 0;
}
