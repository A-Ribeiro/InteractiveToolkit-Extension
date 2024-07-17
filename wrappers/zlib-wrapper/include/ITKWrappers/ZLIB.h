#pragma once

// #include <InteractiveToolkit/InteractiveToolkit.h>

#include <InteractiveToolkit/Platform/Core/ObjectBuffer.h>
#include <InteractiveToolkit/EventCore/Callback.h>

namespace ITKWrappers {
    namespace ZLIB {
        bool compress(
            const Platform::ObjectBuffer &input,
            Platform::ObjectBuffer *output,
            std::string *errorStr = NULL
        );
        bool uncompress(
            const Platform::ObjectBuffer &input,
            Platform::ObjectBuffer *output,
            std::string *errorStr = NULL
        );
    }
}
