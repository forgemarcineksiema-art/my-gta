#pragma once

namespace bs3d {

struct FixedStepConfig {
    float fixedDeltaSeconds = 1.0f / 60.0f;
    float maxFrameDeltaSeconds = 0.10f;
    int maxStepsPerFrame = 5;
};

struct FixedStepFrame {
    int steps = 0;
    float fixedDeltaSeconds = 1.0f / 60.0f;
    float consumedSeconds = 0.0f;
    float alpha = 0.0f;
    bool clamped = false;
};

class FixedStepClock {
public:
    explicit FixedStepClock(FixedStepConfig config = {});

    FixedStepFrame advance(float frameDeltaSeconds);
    void reset();
    float accumulatorSeconds() const;

private:
    FixedStepConfig config_{};
    float accumulatorSeconds_ = 0.0f;
};

} // namespace bs3d
