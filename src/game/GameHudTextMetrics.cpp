#include "GameHudTextMetrics.h"

namespace bs3d {

std::vector<std::string> wrapHudTextToWidth(const std::string& text,
                                            int fontSize,
                                            int maxWidth,
                                            const HudTextMetrics& metrics) {
    std::vector<std::string> lines;
    std::string current;
    std::string word;

    auto flushWord = [&]() {
        if (word.empty()) {
            return;
        }
        const std::string candidate = current.empty() ? word : current + " " + word;
        if (!current.empty() && metrics.measureTextWidth(candidate, fontSize) > maxWidth) {
            lines.push_back(current);
            current = word;
        } else {
            current = candidate;
        }
        word.clear();
    };

    for (char ch : text) {
        if (ch == ' ') {
            flushWord();
        } else {
            word.push_back(ch);
        }
    }
    flushWord();
    if (!current.empty()) {
        lines.push_back(current);
    }
    if (lines.empty()) {
        lines.push_back(text);
    }
    return lines;
}

} // namespace bs3d
