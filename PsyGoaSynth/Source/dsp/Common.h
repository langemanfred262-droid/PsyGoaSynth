#pragma once
// Plain C++ DSP helpers (no JUCE dependency, so they can be unit-tested anywhere).
#include <algorithm>
#include <cmath>
#include <cstdint>

namespace psy
{
constexpr double kPi    = 3.14159265358979323846;
constexpr double kTwoPi = 2.0 * kPi;

// ---- Tempo divisions (shared by LFOs, delay) -------------------------------------------------
// Order MUST match the "DIV" choice list in tools/generate.py
//   0:4/1 1:2/1 2:1/1 3:1/2 4:1/4 5:1/4. 6:1/8 7:1/8. 8:1/8T 9:1/16 10:1/16. 11:1/16T 12:1/32
constexpr int kNumDivs = 13;
inline double divBeats (int i)
{
    static const double t[kNumDivs] = { 16.0, 8.0, 4.0, 2.0, 1.0, 1.5, 0.5, 0.75,
                                        1.0 / 3.0, 0.25, 0.375, 1.0 / 6.0, 0.125 };
    return t[std::clamp (i, 0, kNumDivs - 1)];
}

// Trance gate step lengths. 0:1/4 1:1/8 2:1/16 3:1/32 4:1/8T 5:1/16T
constexpr int kNumGateDivs = 6;
inline double gateBeats (int i)
{
    static const double t[kNumGateDivs] = { 1.0, 0.5, 0.25, 0.125, 1.0 / 3.0, 1.0 / 6.0 };
    return t[std::clamp (i, 0, kNumGateDivs - 1)];
}

// ---- Scales (psy / goa / oriental) -------------------------------------------------------------
// 0 Chromatic, 1 Phrygian, 2 Phrygian Dominant, 3 Double Harmonic, 4 Harmonic Minor,
// 5 Natural Minor, 6 Persian, 7 Minor Pentatonic
constexpr int kNumScales = 8;
inline int scaleMask (int idx)
{
    static const int iv[kNumScales][12] = {
        { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11 },
        { 0, 1, 3, 5, 7, 8, 10, -1, -1, -1, -1, -1 },
        { 0, 1, 4, 5, 7, 8, 10, -1, -1, -1, -1, -1 },
        { 0, 1, 4, 5, 7, 8, 11, -1, -1, -1, -1, -1 },
        { 0, 2, 3, 5, 7, 8, 11, -1, -1, -1, -1, -1 },
        { 0, 2, 3, 5, 7, 8, 10, -1, -1, -1, -1, -1 },
        { 0, 1, 4, 5, 6, 8, 11, -1, -1, -1, -1, -1 },
        { 0, 3, 5, 7, 10, -1, -1, -1, -1, -1, -1, -1 } };
    int m = 0;
    for (int k = 0; k < 12; ++k)
        if (iv[std::clamp (idx, 0, kNumScales - 1)][k] >= 0)
            m |= 1 << iv[std::clamp (idx, 0, kNumScales - 1)][k];
    return m;
}

// Snap a MIDI note to the nearest note in the scale (ties go downward).
inline int quantizeToScale (int note, int root, int scaleIdx)
{
    const int mask = scaleMask (scaleIdx);
    if (mask == 0xFFF) return note;
    for (int d = 0; d <= 6; ++d)
        for (int sgn : { -1, 1 })
        {
            const int n   = note + sgn * d;
            const int rel = (((n - root) % 12) + 12) % 12;
            if (mask & (1 << rel)) return n;
        }
    return note;
}

// ---- Misc ---------------------------------------------------------------------------------------
inline double frac (double x) { return x - std::floor (x); }

// LFO shapes: 0 Sine, 1 Triangle, 2 Saw Down, 3 Square. phase in [0,1) -> [-1,1]
inline float lfoShape (int shape, double ph)
{
    switch (shape)
    {
        case 1:  return (float) (4.0 * std::fabs (ph - 0.5) - 1.0);
        case 2:  return (float) (1.0 - 2.0 * ph);
        case 3:  return ph < 0.5 ? 1.0f : -1.0f;
        default: return (float) std::sin (kTwoPi * ph);
    }
}

inline float msToCoef (float ms, double sr)          // one-pole smoothing coefficient
{
    return (float) (1.0 - std::exp (-1.0 / (std::max (ms, 0.01f) * 0.001 * sr)));
}
inline float decayCoef (float ms, double sr)         // exponential decay, -60 dB after `ms`
{
    return (float) std::exp (std::log (0.001) / (std::max (ms, 1.0f) * 0.001 * sr));
}
inline float flushDenormal (float x) { return std::fabs (x) < 1.0e-20f ? 0.0f : x; }
inline float sanitize (float x) { return std::isfinite (x) ? x : 0.0f; }

// ---- ADSR ---------------------------------------------------------------------------------------
class Adsr
{
public:
    void setSampleRate (double sr) { sampleRate = sr; }

    void set (float aMs, float dMs, float s, float rMs)
    {
        attackInc = (float) (1.0 / (std::max (aMs, 0.2f) * 0.001 * sampleRate));
        decayCoefV = (float) std::exp (-4.6 / (std::max (dMs, 1.0f) * 0.001 * sampleRate));
        relCoef = (float) std::exp (-4.6 / (std::max (rMs, 1.0f) * 0.001 * sampleRate));
        sustain = std::clamp (s, 0.0f, 1.0f);
    }
    void noteOn()  { state = Attack; }
    void noteOff() { if (state != Idle) state = Release; }
    void reset()   { state = Idle; level = 0.0f; }
    bool isActive() const { return state != Idle; }

    float next()
    {
        switch (state)
        {
            case Attack:
                level += attackInc;
                if (level >= 1.0f) { level = 1.0f; state = Decay; }
                break;
            case Decay:
                level = sustain + (level - sustain) * decayCoefV;
                if (std::fabs (level - sustain) < 1.0e-4f) { level = sustain; state = Sustain; }
                break;
            case Sustain:
                level = sustain;
                break;
            case Release:
                level *= relCoef;
                if (level < 1.0e-4f) { level = 0.0f; state = Idle; }
                break;
            default: break;
        }
        return level;
    }

private:
    enum State { Idle, Attack, Decay, Sustain, Release };
    State state = Idle;
    double sampleRate = 44100.0;
    float level = 0.0f, attackInc = 0.01f, decayCoefV = 0.999f, relCoef = 0.999f, sustain = 0.7f;
};
} // namespace psy
