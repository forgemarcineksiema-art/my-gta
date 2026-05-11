#include "bs3d/core/DialogueSystem.h"

#include <stdexcept>
#include <utility>

namespace bs3d {

void DialogueSystem::push(DialogueLine line) {
    if (line.durationSeconds <= 0.0f) {
        line.durationSeconds = 2.5f;
    }

    queueFor(line.channel).push_back(std::move(line));
}

void DialogueSystem::clear() {
    reactionLines_.clear();
    systemHintLines_.clear();
    missionCriticalLines_.clear();
    failChaseLines_.clear();
    lineAgeSeconds_ = 0.0f;
}

void DialogueSystem::update(float deltaSeconds) {
    if (!hasLine()) {
        lineAgeSeconds_ = 0.0f;
        return;
    }

    const DialogueChannel channel = activeChannel();
    std::deque<DialogueLine>& activeQueue = queueFor(channel);
    lineAgeSeconds_ += deltaSeconds;

    while (!activeQueue.empty() && lineAgeSeconds_ >= activeQueue.front().durationSeconds) {
        lineAgeSeconds_ -= activeQueue.front().durationSeconds;
        activeQueue.pop_front();
    }

    if (!hasLine() || activeChannel() != channel) {
        lineAgeSeconds_ = 0.0f;
    }
}

bool DialogueSystem::hasLine() const {
    return !reactionLines_.empty() ||
           !systemHintLines_.empty() ||
           !missionCriticalLines_.empty() ||
           !failChaseLines_.empty();
}

bool DialogueSystem::hasBlockingLine() const {
    if (!hasLine()) {
        return false;
    }

    const DialogueChannel channel = activeChannel();
    return channel == DialogueChannel::MissionCritical || channel == DialogueChannel::FailChase;
}

const DialogueLine& DialogueSystem::currentLine() const {
    if (!hasLine()) {
        throw std::logic_error("DialogueSystem has no current line");
    }

    return queueFor(activeChannel()).front();
}

std::size_t DialogueSystem::queuedLineCount() const {
    return reactionLines_.size() +
           systemHintLines_.size() +
           missionCriticalLines_.size() +
           failChaseLines_.size();
}

std::deque<DialogueLine>& DialogueSystem::queueFor(DialogueChannel channel) {
    switch (channel) {
    case DialogueChannel::Reaction:
        return reactionLines_;
    case DialogueChannel::SystemHint:
        return systemHintLines_;
    case DialogueChannel::MissionCritical:
        return missionCriticalLines_;
    case DialogueChannel::FailChase:
        return failChaseLines_;
    }

    return missionCriticalLines_;
}

const std::deque<DialogueLine>& DialogueSystem::queueFor(DialogueChannel channel) const {
    switch (channel) {
    case DialogueChannel::Reaction:
        return reactionLines_;
    case DialogueChannel::SystemHint:
        return systemHintLines_;
    case DialogueChannel::MissionCritical:
        return missionCriticalLines_;
    case DialogueChannel::FailChase:
        return failChaseLines_;
    }

    return missionCriticalLines_;
}

DialogueChannel DialogueSystem::activeChannel() const {
    if (!failChaseLines_.empty()) {
        return DialogueChannel::FailChase;
    }
    if (!missionCriticalLines_.empty()) {
        return DialogueChannel::MissionCritical;
    }
    if (!systemHintLines_.empty()) {
        return DialogueChannel::SystemHint;
    }
    return DialogueChannel::Reaction;
}

} // namespace bs3d
