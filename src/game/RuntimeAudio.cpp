#include "RuntimeAudio.h"

#include "raylib.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <memory>
#include <vector>

namespace bs3d {

namespace {

Sound makeTone(float frequency, float seconds, float volume) {
    constexpr int sampleRate = 22050;
    const int frameCount = std::max(1, static_cast<int>(seconds * static_cast<float>(sampleRate)));
    std::vector<std::int16_t> samples(static_cast<std::size_t>(frameCount));

    for (int i = 0; i < frameCount; ++i) {
        const float t = static_cast<float>(i) / static_cast<float>(sampleRate);
        const float progress = static_cast<float>(i) / static_cast<float>(std::max(1, frameCount - 1));
        const float envelope = std::max(0.0f, 1.0f - progress * 0.85f);
        const float wobble = std::sin(t * frequency * 6.2831853f) +
                             std::sin(t * frequency * 1.72f * 6.2831853f) * 0.35f;
        const float sample = std::clamp(wobble * volume * envelope, -1.0f, 1.0f);
        samples[static_cast<std::size_t>(i)] = static_cast<std::int16_t>(sample * 32767.0f);
    }

    Wave wave{};
    wave.frameCount = static_cast<unsigned int>(frameCount);
    wave.sampleRate = sampleRate;
    wave.sampleSize = 16;
    wave.channels = 1;
    wave.data = samples.data();
    return LoadSoundFromWave(wave);
}

} // namespace

struct RuntimeAudio::Impl {
    bool audioReady = false;
    Sound hornSound{};
    Sound startSound{};
    Sound bumpSound{};

    void playIfReady(Sound sound) const {
        if (audioReady) {
            PlaySound(sound);
        }
    }
};

RuntimeAudio::RuntimeAudio()
    : impl_(std::make_unique<Impl>()) {
}

RuntimeAudio::~RuntimeAudio() {
    shutdown();
}

void RuntimeAudio::setup() {
    InitAudioDevice();
    impl_->audioReady = IsAudioDeviceReady();
    if (!impl_->audioReady) {
        return;
    }

    impl_->hornSound = makeTone(220.0f, 0.34f, 0.36f);
    impl_->startSound = makeTone(82.0f, 0.48f, 0.28f);
    impl_->bumpSound = makeTone(54.0f, 0.18f, 0.42f);
}

void RuntimeAudio::shutdown() {
    if (impl_->audioReady) {
        UnloadSound(impl_->hornSound);
        UnloadSound(impl_->startSound);
        UnloadSound(impl_->bumpSound);
    }

    if (IsAudioDeviceReady()) {
        CloseAudioDevice();
    }
    impl_->audioReady = false;
}

void RuntimeAudio::playHorn() const {
    impl_->playIfReady(impl_->hornSound);
}

void RuntimeAudio::playStart() const {
    impl_->playIfReady(impl_->startSound);
}

void RuntimeAudio::playBump() const {
    impl_->playIfReady(impl_->bumpSound);
}

} // namespace bs3d
