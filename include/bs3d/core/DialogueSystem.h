#pragma once

#include <cstddef>
#include <deque>
#include <string>

namespace bs3d {

enum class DialogueChannel {
    Reaction,
    SystemHint,
    MissionCritical,
    FailChase
};

struct DialogueLine {
    std::string speaker;
    std::string text;
    float durationSeconds = 2.5f;
    DialogueChannel channel = DialogueChannel::MissionCritical;
};

class DialogueSystem {
public:
    void push(DialogueLine line);
    void clear();
    void update(float deltaSeconds);

    bool hasLine() const;
    bool hasBlockingLine() const;
    const DialogueLine& currentLine() const;
    std::size_t queuedLineCount() const;

private:
    std::deque<DialogueLine> reactionLines_;
    std::deque<DialogueLine> systemHintLines_;
    std::deque<DialogueLine> missionCriticalLines_;
    std::deque<DialogueLine> failChaseLines_;
    float lineAgeSeconds_ = 0.0f;

    std::deque<DialogueLine>& queueFor(DialogueChannel channel);
    const std::deque<DialogueLine>& queueFor(DialogueChannel channel) const;
    DialogueChannel activeChannel() const;
};

} // namespace bs3d
