#pragma once

#include <InteractiveToolkit/InteractiveToolkit.h>

namespace ITKWrappers {
    namespace ZLIB {
        void compress(
            const Platform::ObjectBuffer &input,
            Platform::ObjectBuffer *output,
            EventCore::Callback<void(const std::string &)> onError = nullptr
        );
        void uncompress(
            const Platform::ObjectBuffer &input,
            Platform::ObjectBuffer *output,
            EventCore::Callback<void(std::string)> onError = nullptr
        );
    }
}
