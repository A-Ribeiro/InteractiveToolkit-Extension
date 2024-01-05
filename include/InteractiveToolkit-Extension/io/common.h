#pragma once

// #include <InteractiveToolkit/InteractiveToolkit.h>
#include <ITKWrappers/ZLIB.h>

namespace ITKExtension
{
    namespace IO
    {

        template <class _T>
        struct custom_stl_is_std_vector
        {
            static constexpr bool value = false;
        };

        template <class _T>
        struct custom_stl_is_std_vector<std::vector<_T>>
        {
            static constexpr bool value = true;
            using itemType = _T;
        };

        template <class _T1>
        struct custom_stl_is_std_map
        {
            static constexpr bool value = false;
        };

        template <class _T1, class _T2>
        struct custom_stl_is_std_map<std::map<_T1, _T2>>
        {
            static constexpr bool value = true;
            using keyType = _T1;
            using valueType = _T2;
        };

    }

}