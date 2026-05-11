#include "bs3d/core/FixedTimestep.h"

#include <algorithm>
#include <cmath>

namespace bs3d {

FixedStepClock::FixedStepClock(FixedStepConfig config) : config_(config) {
    config_.fixedDeltaSeconds = std::max(0.001f, config_.fixedDeltaSeconds);
    config_.maxFrameDeltaSeconds = std::max(config_.fixedDeltaSeconds, config_.maxFrameDeltaSeconds);
    config_.maxStepsPerFrame = std::max(1, config_.maxStepsPerFrame);
}

FixedStepFrame FixedStepClock::advance(float frameDeltaSeconds) {
    FixedStepFrame frame;
    frame.fixedDeltaSeconds = config_.fixedDeltaSeconds;

    const float clampedDelta = std::clamp(frameDeltaSeconds, 0.0f, config_.maxFrameDeltaSeconds);
    frame.clamped = frameDeltaSeconds > config_.maxFrameDeltaSeconds;
    accumulatorSeconds_ += clampedDelta;

    while (accumulatorSeconds_ + 0.000001f >= config_.fixedDeltaSeconds &&
           frame.steps < config_.maxStepsPerFrame) {
        accumulatorSeconds_ -= config_.fixedDeltaSeconds;
        ++frame.steps;
        frame.consumedSeconds += config_.fixedDeltaSeconds;
    }

    if (frame.steps == config_.maxStepsPerFrame &&
        accumulatorSeconds_ >= config_.fixedDeltaSeconds) {
        accumulatorSeconds_ = std::fmod(accumulatorSeconds_, config_.fixedDeltaSeconds);
        frame.clamped = true;
    }

    frame.alpha = std::clamp(accumulatorSeconds_ / config_.fixedDeltaSeconds, 0.0f, 1.0f);
    return frame;
}

void FixedStepClock::reset() {
    accumulatorSeconds_ = 0.0f;
}

float FixedStepClock::accumulatorSeconds() const {
    return accumulatorSeconds_;
}

} // namespace bs3d
