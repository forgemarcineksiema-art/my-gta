#pragma once

#include <memory>

namespace bs3d {

class RuntimeAudio {
public:
    RuntimeAudio();
    ~RuntimeAudio();
    RuntimeAudio(const RuntimeAudio&) = delete;
    RuntimeAudio& operator=(const RuntimeAudio&) = delete;

    void setup();
    void shutdown();

    void playHorn() const;
    void playStart() const;
    void playBump() const;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace bs3d
