#pragma once
#include "Common.h"

namespace psy
{
struct GateParams
{
    bool on = false;
    int div = 2;                  // 1/16
    int pattern = 0xFFFF;         // bit n = step n open
    int len = 16;
    float depth = 1.0f;           // 1 = closed steps fully silent
    float smoothMs = 2.0f;
};

class TranceGate
{
public:
    void prepare (double sr) { sampleRate = sr; gain = 1.0f; }
    void setParams (const GateParams& np)
    {
        p = np;
        p.len = std::clamp (p.len, 1, 16);
        coef = msToCoef (p.smoothMs, sampleRate);
    }

    // returns the currently active step (for the GUI)
    int stepAt (double beat) const
    {
        const long long idx = (long long) std::floor (beat / gateBeats (p.div));
        return (int) (((idx % p.len) + p.len) % p.len);
    }

    void process (float* l, float* r, int n, double beat0, double beatInc)
    {
        if (! p.on && gain > 0.9999f) { gain = 1.0f; return; }
        const double stepBeats = gateBeats (p.div);
        for (int i = 0; i < n; ++i)
        {
            float target = 1.0f;
            if (p.on)
            {
                const long long idx = (long long) std::floor ((beat0 + i * beatInc) / stepBeats);
                const int s = (int) (((idx % p.len) + p.len) % p.len);
                target = ((p.pattern >> s) & 1) ? 1.0f : 1.0f - p.depth;
            }
            gain += coef * (target - gain);
            l[i] *= gain;
            r[i] *= gain;
        }
    }

private:
    GateParams p;
    double sampleRate = 44100.0;
    float gain = 1.0f, coef = 0.1f;
};
} // namespace psy
