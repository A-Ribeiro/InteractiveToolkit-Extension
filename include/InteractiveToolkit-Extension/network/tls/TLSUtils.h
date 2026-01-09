#pragma once

// #include <InteractiveToolkit/Platform/SocketTCP.h>
#include <InteractiveToolkit/common.h>

namespace TLS
{
    class TLSUtils
    {
    public:
        static std::string errorMessageFromReturnCode(int errnum);
    };
}

#define TLS_DECLARE_CREATE_SHARED(ClassName)                       \
private:                                                           \
    std::weak_ptr<ClassName> mSelf;                                \
                                                                   \
public:                                                            \
    static inline std::shared_ptr<ClassName> CreateShared()        \
    {                                                              \
        auto result = std::shared_ptr<ClassName>(new ClassName()); \
        result->mSelf = std::weak_ptr<ClassName>(result);          \
        return result;                                             \
    }                                                              \
    inline std::shared_ptr<ClassName> self()                       \
    {                                                              \
        return std::shared_ptr<ClassName>(mSelf);                  \
    }
