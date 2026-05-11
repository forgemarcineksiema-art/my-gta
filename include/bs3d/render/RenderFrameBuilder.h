#pragma once

#include "bs3d/render/RenderFrame.h"
#include "bs3d/render/RenderFrameValidation.h"

#include <vector>

namespace bs3d {

/// Backend-neutral helper for constructing a RenderFrame with primitives
/// accumulated by bucket and emitted in production render order.
///
/// This is a contract/test utility.  It does not draw anything, does not depend
/// on any GPU backend, and does not replace runtime rendering.
class RenderFrameBuilder {
public:
    RenderFrameBuilder() = default;

    RenderFrameBuilder& setCamera(RenderCamera camera) {
        camera_ = camera;
        return *this;
    }

    RenderFrameBuilder& setWorldStyle(WorldPresentationStyle style) {
        worldStyle_ = style;
        return *this;
    }

    RenderFrameBuilder& addPrimitive(RenderPrimitiveCommand command) {
        switch (command.bucket) {
        case RenderBucket::Sky:
            sky_.push_back(std::move(command));
            break;
        case RenderBucket::Ground:
            ground_.push_back(std::move(command));
            break;
        case RenderBucket::Opaque:
            opaque_.push_back(std::move(command));
            break;
        case RenderBucket::Vehicle:
            vehicle_.push_back(std::move(command));
            break;
        case RenderBucket::Decal:
            decal_.push_back(std::move(command));
            break;
        case RenderBucket::Glass:
            glass_.push_back(std::move(command));
            break;
        case RenderBucket::Translucent:
            translucent_.push_back(std::move(command));
            break;
        case RenderBucket::Debug:
            debug_.push_back(std::move(command));
            break;
        case RenderBucket::Hud:
            hud_.push_back(std::move(command));
            break;
        }
        return *this;
    }

    RenderFrameBuilder& addDebugLine(RenderLineCommand command) {
        debugLines_.push_back(std::move(command));
        return *this;
    }

    RenderFrameBuilder& addDebugLine(Vec3 start, Vec3 end, RenderColor tint) {
        debugLines_.push_back({start, end, tint});
        return *this;
    }

    RenderFrame build() const {
        RenderFrame frame;
        frame.camera = camera_;
        frame.worldStyle = worldStyle_;

        // Reserve to avoid repeated allocations.
        const std::size_t total = sky_.size() + ground_.size() + opaque_.size() +
                                  vehicle_.size() + decal_.size() + glass_.size() +
                                  translucent_.size() + debug_.size() + hud_.size();
        frame.primitives.reserve(total);

        // Append in production render order, preserving insertion order within
        // each bucket.
        appendBucket(frame.primitives, sky_);
        appendBucket(frame.primitives, ground_);
        appendBucket(frame.primitives, opaque_);
        appendBucket(frame.primitives, vehicle_);
        appendBucket(frame.primitives, decal_);
        appendBucket(frame.primitives, glass_);
        appendBucket(frame.primitives, translucent_);
        appendBucket(frame.primitives, debug_);
        appendBucket(frame.primitives, hud_);

        frame.debugLines = debugLines_;
        return frame;
    }

    RenderFrameValidationResult validate() const {
        return validateRenderFrame(build());
    }

    RenderFrameStats stats() const {
        return summarizeRenderFrame(build());
    }

private:
    static void appendBucket(std::vector<RenderPrimitiveCommand>& out,
                             const std::vector<RenderPrimitiveCommand>& bucket) {
        out.insert(out.end(), bucket.begin(), bucket.end());
    }

    RenderCamera camera_{};
    WorldPresentationStyle worldStyle_{};

    std::vector<RenderPrimitiveCommand> sky_;
    std::vector<RenderPrimitiveCommand> ground_;
    std::vector<RenderPrimitiveCommand> opaque_;
    std::vector<RenderPrimitiveCommand> vehicle_;
    std::vector<RenderPrimitiveCommand> decal_;
    std::vector<RenderPrimitiveCommand> glass_;
    std::vector<RenderPrimitiveCommand> translucent_;
    std::vector<RenderPrimitiveCommand> debug_;
    std::vector<RenderPrimitiveCommand> hud_;

    std::vector<RenderLineCommand> debugLines_;
};

} // namespace bs3d
