#include "GameConfig.h"
#include <random>
#undef min
#undef max

float saturate_soft(float x) {
    return tanhf(x);
}

void normalizeSamples(std::vector<float>& samples, float targetPeak) {
    float max_amp = 0.0f;
    for (float s : samples) max_amp = std::max(max_amp, std::fabs(s));
    if (max_amp <= 0.0f) return;

    float g = targetPeak / max_amp;
    for (auto& s : samples) s *= g;
}

void mixSynthSound(SynthSound& snd, float& v) {
    if (!snd.playing) return;

    for (int& pos : snd.playheads) {
        if (pos < static_cast<int>(snd.samples.size())) {
            v += snd.samples[pos++];
        }
    }

    snd.playheads.erase(
        std::remove_if(snd.playheads.begin(), snd.playheads.end(),
                       [&](int pos) { return pos >= static_cast<int>(snd.samples.size()); }),
        snd.playheads.end()
    );

    if (snd.playheads.empty()) {
        snd.playing = false;
    }
}
void request_play(SynthSound& snd) {
    std::lock_guard<std::mutex> lock(sounds_to_play_mutex); 
    if (sounds_to_play.size() > 100) return;
    sounds_to_play.push_back(&snd);
}

void audio_callback(void* userdata, Uint8* stream, int len) {
    float* fstream = reinterpret_cast<float*>(stream);
    int samples = len / sizeof(float);

    {
        std::lock_guard<std::mutex> lock(sounds_to_play_mutex);
        for (auto* sound : sounds_to_play) {
            sound->playheads.push_back(0);
            sound->playing = true;
        }
        sounds_to_play.clear();
    }
    float localEnergySum = 0.0f;
    for (int i = 0; i < samples; ++i) {
        float v = 0.0f;
        mixSynthSound(blueSound, v);
        mixSynthSound(rainbowSound, v);
        mixSynthSound(explosionSound, v);
        localEnergySum += std::abs(v);
        v = std::max(-1.0f, std::min(1.0f, v));
        fstream[i] = v * SYNTH_MASTER_GAIN;
    }
    float avg = localEnergySum / samples;
    currentMusicEnergy = currentMusicEnergy * 0.9f + avg * 5.0f * 0.1f;
}

void make_rainbow_sound(SynthSound& snd) {
    snd.samples.clear();
    snd.playheads.clear();
    snd.playing = false;

    const int rate = 44100;
    const float duration = 1.25f;
    const int N = int(rate * duration);
    snd.samples.assign(N, 0.0f);

    const float PI = 3.14159265358979323846f;
    const float TAU = 6.28318530717958647692f;

    auto ar_env = [&](float t, float a, float r)->float {
        if (t < a) {
            float k = t / a;
            return 0.5f - 0.5f * cosf(PI * k);
        } else {
            float u = (t - a);
            return expf(-u / r);
        }
    };

    auto gauss01 = [&](float x, float mu, float sigma)->float {
        float d = (x - mu) / (sigma + 1e-12f);
        return expf(-0.5f * d * d);
    };

    {
        std::minstd_rand rng(42);
        std::uniform_real_distribution<float> uni(0.0f, 1.0f);

        const float base = (220.0f / PHI) * T;
        const float ratios[] = {
            1.0f, 5.0f / 4.0f, 3.0f / 2.0f, 2.0f, PHI, 7.0f / 4.0f
        };
        const float amps[] = { 0.28f, 0.24f, 0.20f, 0.15f, 0.12f, 0.10f };
        float phases[6], lfoPhase[6];

        for (int i = 0; i < 6; ++i) {
            phases[i] = uni(rng) * TAU;
            lfoPhase[i] = uni(rng) * TAU;
        }

        for (int i = 0; i < N; ++i) {
            float t = float(i) / rate;
            float env = ar_env(t, 0.16f, 0.9f);
            float s = 0.0f;

            for (int k = 0; k < 6; ++k) {
                float f = base * ratios[k];
                float lfo = 0.75f + 0.25f * sinf(TAU * (0.05f + 0.007f * k) * t + lfoPhase[k]);
                float pm = 0.0023f * sinf(TAU * (0.21f + 0.013f * k) * t + phases[k]);
                s += sinf(TAU * f * t + pm) * amps[k] * lfo;
            }
            snd.samples[i] += s * env * 0.45f;
        }
    }

    struct Chirp {
        int start;
        int len;
        float f0, f1;
        float amp;
    };
    auto halton = [&](int idx, int base)->float {
        float f = 1.0f, r = 0.0f;
        int i = idx + 1;
        while (i > 0) {
            f /= base;
            r += f * (i % base);
            i /= base;
        }
        return r;
    };

    {
        const int numChirps = 36;
        std::vector<Chirp> chirps;
        chirps.reserve(numChirps);

        for (int c = 0; c < numChirps; ++c) {
            float t0 = halton(c, 2) * (duration - 0.22f);
            float w = 0.08f + 0.12f * halton(c, 3);
            int start = std::max(0, std::min(N - 1, int(t0 * rate)));
            int len = std::max(8, std::min(N - start, int(w * rate)));

            float fLow = (700.0f + 900.0f * halton(c, 5)) * T;
            float fHigh = (4200.0f + 5000.0f * halton(c, 7)) * T;

            bool ascend = (c % 2 == 0);
            float f0 = ascend ? fLow : fHigh;
            float f1 = ascend ? fHigh : fLow;

            float a = 0.05f + 0.09f * (0.5f + 0.5f * sinf(2.0f + 1.7f * c));
            chirps.push_back({ start, len, f0, f1, a });
        }

        for (const auto& ch : chirps) {
            float phi = 0.0f;
            for (int j = 0; j < ch.len && (ch.start + j) < N; ++j) {
                float u = float(j) / (float)ch.len;
                float f = ch.f0 * powf(ch.f1 / ch.f0, u);
                phi += TAU * f / rate;

                float w = gauss01(u, 0.5f, 0.22f);
                snd.samples[ch.start + j] += sinf(phi) * w * ch.amp * 0.6f;
            }
        }
    }

    {
        for (int i = 0; i < N; ++i) {
            float x = snd.samples[i];

            float x2 = x * x;
            float x3 = x2 * x;
            float x5 = x3 * x2;
            float t3 = 4.0f * x3 - 3.0f * x;
            float t5 = 16.0f * x5 - 20.0f * x3 + 5.0f * x;
            snd.samples[i] = x + 0.06f * t3 + 0.02f * t5;
        }
    }

    {
        const int L1 = 3463, L2 = 4289, L3 = 5419;
        const float g1 = 0.78f, g2 = 0.76f, g3 = 0.74f;
        std::vector<float> dl1(L1, 0.0f), dl2(L2, 0.0f), dl3(L3, 0.0f);
        int p1 = 0, p2 = 0, p3 = 0;
        float lp1 = 0.0f, lp2 = 0.0f, lp3 = 0.0f;

        const int A1 = 337, A2 = 431;
        const float ag = 0.60f;
        std::vector<float> ap1(A1, 0.0f), ap2(A2, 0.0f);
        int ap1p = 0, ap2p = 0;

        for (int i = 0; i < N; ++i) {
            float x = snd.samples[i];

            float y1 = dl1[p1];
            lp1 = 0.5f * lp1 + 0.5f * y1;
            dl1[p1] = x + g1 * lp1;
            if (++p1 >= L1) p1 = 0;

            float y2 = dl2[p2];
            lp2 = 0.5f * lp2 + 0.5f * y2;
            dl2[p2] = x + g2 * lp2;
            if (++p2 >= L2) p2 = 0;

            float y3 = dl3[p3];
            lp3 = 0.5f * lp3 + 0.5f * y3;
            dl3[p3] = x + g3 * lp3;
            if (++p3 >= L3) p3 = 0;

            float r = (y1 + y2 + y3) * 0.33f;

            float apin = r;
            float apo1 = -ag * apin + ap1[ap1p];
            ap1[ap1p] = apin + ag * apo1;
            if (++ap1p >= A1) ap1p = 0;

            float apo2 = -ag * apo1 + ap2[ap2p];
            ap2[ap2p] = apo1 + ag * apo2;
            if (++ap2p >= A2) ap2p = 0;

            float out = 0.65f * x + 0.35f * apo2;
            snd.samples[i] = saturate_soft(out * 0.92f);
        }
    }

	normalizeSamples(snd.samples, 0.55f);
}

void make_stellar_explosion_sound(SynthSound& snd) {
    snd.samples.clear();
    snd.playheads.clear();
    snd.playing = false;

    const int rate = 44100;
    const float duration = 1.8f;
    const int N = int(rate * duration);
    snd.samples.assign(N, 0.0f);

    const float TAU = 6.2831853f;

    for (int i = 0; i < N; ++i) {
        float t = float(i) / rate;

        float freqEnv = expf(-t * 6.0f); 
        float f = 180.0f * freqEnv; 
        float phase = TAU * f * t; 
        float boom = sinf(phase) * expf(-t * 2.5f);

        float noise = ((float)rand() / RAND_MAX - 0.5f) * 2.0f;
        float noiseEnv = expf(-t * 15.0f);
        float crackle = noise * noiseEnv;

        float rumble = sinf(t * 50.0f * TAU + sinf(t * 10.0f)) * 0.3f * expf(-t * 1.0f);

        float signal = (boom * 2.0f + crackle * 0.8f + rumble * 0.5f);

        signal = tanhf(signal * 1.5f); 

        snd.samples[i] = signal;
    }

    normalizeSamples(snd.samples, 0.85f);
}

void make_blue_sound(SynthSound& snd) {
    snd.samples.clear();
    snd.playheads.clear();
    snd.playing = false;

    const int rate = 44100;

    const float duration = 0.85f;
    const int N = int(rate * duration);
    snd.samples.assign(N, 0.0f);

    const float PI = 3.14159265358979323846f;
    const float TAU = 6.28318530717958647692f;

    float baseFreq = 320.0f; 

    float phase = 0.0f;
    float fmPhase = 0.0f;

    for (int i = 0; i < N; ++i) {
        float t = float(i) / rate;
        
        float pitchMod = 1.0f + 0.8f * expf(-t * 18.0f);
        
        float currentFreq = baseFreq * pitchMod;
        
        phase += (TAU * currentFreq) / rate;

        float fmFreq = currentFreq * 1.618f; 
        fmPhase += (TAU * fmFreq) / rate;

        float fmIndex = 1.5f * expf(-t * 8.0f);
        float modulator = sinf(fmPhase);
        
        float sample = sinf(phase + modulator * fmIndex);

        float ampEnv = (t * 15.0f) * expf(-t * 8.0f); 
        
        float sparkle = ((float)rand() / RAND_MAX - 0.5f) * 0.1f * expf(-t * 30.0f);

        snd.samples[i] = (sample + sparkle) * ampEnv;
    }

    int delaySamples = int(rate * 0.12f);
    float feedback = 0.35f;

    for (int i = delaySamples; i < N; ++i) {
        snd.samples[i] += snd.samples[i - delaySamples] * feedback;
    }

    normalizeSamples(snd.samples, 0.65f);
}

