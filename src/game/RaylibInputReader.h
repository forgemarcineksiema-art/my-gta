#pragma once

#include "bs3d/core/ControlContext.h"
#include "bs3d/platform/IInputReader.h"

namespace bs3d {

class RaylibInputReader final : public IInputReader {
public:
    RawInputState read(bool mouseCaptured) const override;
};

// Tymczasowy wrapper dla bezpiecznego refaktoru; runtime ma korzystać z IInputReader.
RawInputState readRaylibRawInput(bool mouseCaptured);

} // namespace bs3d
