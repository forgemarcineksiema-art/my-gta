#pragma once

#include "bs3d/render/RenderFrame.h"

#include <string>

namespace bs3d {

struct RenderFrameStats {
    int totalPrimitives = 0;
    int debugLines = 0;

    int sky = 0;
    int ground = 0;
    int opaque = 0;
    int vehicle = 0;
    int decal = 0;
    int glass = 0;
    int translucent = 0;
    int debug = 0;
    int hud = 0;
};

struct RenderFrameValidationResult {
    bool valid = true;
    std::string message;
};

inline int renderBucketOrderKey(RenderBucket bucket) {
    switch (bucket) {
    case RenderBucket::Sky:
        return 0;
    case RenderBucket::Ground:
        return 1;
    case RenderBucket::Opaque:
        return 2;
    case RenderBucket::Vehicle:
        return 3;
    case RenderBucket::Decal:
        return 4;
    case RenderBucket::Glass:
        return 5;
    case RenderBucket::Translucent:
        return 6;
    case RenderBucket::Debug:
        return 7;
    case RenderBucket::Hud:
        return 8;
    }
    return 9;
}

inline RenderFrameStats summarizeRenderFrame(const RenderFrame& frame) {
    RenderFrameStats stats;
    stats.totalPrimitives = static_cast<int>(frame.primitives.size());
    stats.debugLines = static_cast<int>(frame.debugLines.size());

    for (const RenderPrimitiveCommand& command : frame.primitives) {
        switch (command.bucket) {
        case RenderBucket::Sky:
            ++stats.sky;
            break;
        case RenderBucket::Ground:
            ++stats.ground;
            break;
        case RenderBucket::Opaque:
            ++stats.opaque;
            break;
        case RenderBucket::Vehicle:
            ++stats.vehicle;
            break;
        case RenderBucket::Decal:
            ++stats.decal;
            break;
        case RenderBucket::Glass:
            ++stats.glass;
            break;
        case RenderBucket::Translucent:
            ++stats.translucent;
            break;
        case RenderBucket::Debug:
            ++stats.debug;
            break;
        case RenderBucket::Hud:
            ++stats.hud;
            break;
        }
    }

    return stats;
}

inline bool isRenderFrameBucketOrderValid(const RenderFrame& frame) {
    int previousKey = -1;
    for (const RenderPrimitiveCommand& command : frame.primitives) {
        const int key = renderBucketOrderKey(command.bucket);
        if (key < previousKey) {
            return false;
        }
        previousKey = key;
    }
    return true;
}

inline RenderFrameValidationResult validateRenderFrame(const RenderFrame& frame) {
    int previousKey = -1;
    RenderBucket previousBucket = RenderBucket::Sky;
    for (const RenderPrimitiveCommand& command : frame.primitives) {
        const int key = renderBucketOrderKey(command.bucket);
        if (key < previousKey) {
            RenderFrameValidationResult result;
            result.valid = false;
            result.message = std::string("RenderFrame primitive bucket order is invalid: ") +
                             renderBucketName(command.bucket) + " appears after " + renderBucketName(previousBucket);
            return result;
        }
        previousKey = key;
        previousBucket = command.bucket;
    }

    return {};
}

} // namespace bs3d
