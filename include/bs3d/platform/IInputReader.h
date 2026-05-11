#pragma once

#include "bs3d/core/ControlContext.h"

namespace bs3d {

class IInputReader {
public:
    virtual ~IInputReader() = default;

    virtual RawInputState read(bool mouseCaptured) const = 0;
};

} // namespace bs3d

