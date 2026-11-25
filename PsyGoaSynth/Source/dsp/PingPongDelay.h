#pragma once
#include "Common.h"
#include <vector>

namespace psy
{
struct DelayParams
{
    bool on = false;
    float mix = 0.3f;
    bool sync = true;
    int div = 7;                  // 1/8. (dotted eighth, the classic goa/psy delay)
    float timeMs = 400.0f;        // used when sync == false
    float fb = 0.5f;
    float cross = 1.0f;           // 0 = plain stereo delay, 1 = full ping-pong
    float lowCut = 100.0f, hiCut = 6000.0f;
};

class PingPongDelay
{
public:
    void prepare (double sr)
    {
        sampleRate = sr;
        size = (int) (4.2 * sr) + 8;
        for (auto& b : buf) b.assign ((size_t) size, 0.0f);
        wp = 0;
        timeCoef = msToCoef (60.0f, sr);
        clearState();
    }

    void setParams (const DelayParams& np)
    {
        if (p.on && ! np.on) { for (auto& b : buf) std::fill (b.begin(), b.end(), 0.0f); clearState(); }
        p = np;
        lpCoef = (float) (1.0 - std::exp (-kTwoPi * std::clamp (p.hiCut, 200.0f, 18000.0f) / sampleRate));
        hpCoef = (float) (1.0 - std::exp (-kTwoPi * std::clamp (p.lowCut, 10.0f, 4000.0f) / sampleRate));
    }

    void process (float* l, float* r, int n, double bpm)
    {
        if (! p.on) return;
        double sec = p.sync ? 60.0 / bpm * divBeats (p.div) : (double) p.timeMs * 0.001;
        sec = std::clamp (sec, 0.002, 4.0);
        const float targetS = (float) (sec * sampleRate);
        if (timeSm < 1.0f) timeSm = targetS;
        const float fb = std::clamp (p.fb, 0.0f, 0.95f);

        for (int i = 0; i < n; ++i)
        {
            timeSm += timeCoef * (targetS - timeSm);
            const float dl = readAt (0, timeSm);
            const float dr = readAt (1, timeSm);

            // feedback path: low/high cut
            const float fl = filt (0, p.cross * dr + (1.0f - p.cross) * dl);
            const float fr = filt (1, p.cross * dl + (1.0f - p.cross) * dr);

            const float inL = l[i], inR = r[i];
            const float mono = 0.5f * (inL + inR);
            const float feedL = (1.0f - p.cross) * inL + p.cross * mono;
            const float feedR = (1.0f - p.cross) * inR;

            buf[0][(size_t) wp] = flushDenormal (std::tanh (feedL + fb * fl));
            buf[1][(size_t) wp] = flushDenormal (std::tanh (feedR + fb * fr));
            if (++wp >= size) wp = 0;

            l[i] = sanitize (inL + p.mix * dl);
            r[i] = sanitize (inR + p.mix * dr);
        }
    }

private:
    float readAt (int c, float delaySamples) const
    {
        float rp = (float) wp - delaySamples;
        while (rp < 0.0f) rp += (float) size;
        const int i0 = (int) rp;
        const int i1 = (i0 + 1) % size;
        const float fr = rp - (float) i0;
        return buf[c][(size_t) (i0 % size)] * (1.0f - fr) + buf[c][(size_t) i1] * fr;
    }
    float filt (int c, float x)
    {
        lp[c] += lpCoef * (x - lp[c]);
        hp[c] += hpCoef * (lp[c] - hp[c]);
        lp[c] = flushDenormal (lp[c]);
        hp[c] = flushDenormal (hp[c]);
        return lp[c] - hp[c];
    }
    void clearState() { lp[0] = lp[1] = hp[0] = hp[1] = 0.0f; timeSm = 0.0f; }

    DelayParams p;
    std::vector<float> buf[2];
    int size = 1024, wp = 0;
    double sampleRate = 44100.0;
    float timeSm = 0.0f, timeCoef = 0.001f, lpCoef = 0.5f, hpCoef = 0.01f;
    float lp[2] = { 0, 0 }, hp[2] = { 0, 0 };
};
} // namespace psy
