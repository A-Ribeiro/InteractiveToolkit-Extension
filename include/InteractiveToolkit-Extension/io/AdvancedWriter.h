#pragma once

#include "Writer.h"

namespace ITKExtension
{
    namespace IO
    {
        class AdvancedWriter : public Writer
        {

            // vector
            template <typename _vec_type>
            ITK_INLINE void writeVector(const std::vector<_vec_type> &v)
            {
                writeUInt32((uint32_t)v.size());
                for (const auto &item : v)
                    write<_vec_type>(item);
            }

            // map
            template <typename _map_key, typename _map_type>
            ITK_INLINE void writeMap(const std::map<_map_key, _map_type> &v)
            {
                writeUInt32((uint32_t)v.size());
                for (const auto &it : v)
                {
                    write<_map_key>(it.first);
                    write<_map_type>(it.second);
                }
            }

        public:
            AdvancedWriter()
            {
            }

            template <typename _math_type,
                      typename std::enable_if<
                          MathCore::MathTypeInfo<_math_type>::_is_valid::value &&
                              MathCore::MathTypeInfo<_math_type>::_is_vec::value,
                          bool>::type = true>
            ITK_INLINE void write(const _math_type &v)
            {
                for (int i = 0; i < (int)_math_type::array_count; i++)
                    write<typename MathCore::MathTypeInfo<_math_type>::_type>(v[i]);
            }

            template <typename _math_type,
                      typename std::enable_if<
                          MathCore::MathTypeInfo<_math_type>::_is_valid::value &&
                              !MathCore::MathTypeInfo<_math_type>::_is_vec::value,
                          bool>::type = true>
            ITK_INLINE void write(const _math_type &v)
            {
                for (int c = 0; c < (int)_math_type::cols; c++)
                    for (int r = 0; r < (int)_math_type::rows; r++)
                        write<typename MathCore::MathTypeInfo<_math_type>::_type>(v(r, c));
            }

#define CREATE_WRITE_PATTERN(_Type, _Fnc)                                  \
    template <typename _math_type,                                         \
              typename std::enable_if<                                     \
                  !MathCore::MathTypeInfo<_math_type>::_is_valid::value && \
                      std::is_same<_math_type, _Type>::value,              \
                  bool>::type = true>                                      \
    ITK_INLINE void write(const _math_type &v)                             \
    {                                                                      \
        _Fnc(v);                                                           \
    }

            CREATE_WRITE_PATTERN(uint8_t, writeUInt8)
            CREATE_WRITE_PATTERN(int8_t, writeInt8)

            CREATE_WRITE_PATTERN(uint16_t, writeUInt16)
            CREATE_WRITE_PATTERN(int16_t, writeInt16)

            CREATE_WRITE_PATTERN(uint32_t, writeUInt32)
            CREATE_WRITE_PATTERN(int32_t, writeInt32)

            CREATE_WRITE_PATTERN(uint64_t, writeUInt64)
            CREATE_WRITE_PATTERN(int64_t, writeInt64)

            CREATE_WRITE_PATTERN(float, writeFloat)
            CREATE_WRITE_PATTERN(double, writeDouble)

            CREATE_WRITE_PATTERN(bool, writeBool)
            CREATE_WRITE_PATTERN(std::string, writeString)

#undef CREATE_WRITE_PATTERN

            template <typename _vec_type,
                      typename std::enable_if<
                          !MathCore::MathTypeInfo<_vec_type>::_is_valid::value &&
                              custom_stl_is_std_vector<_vec_type>::value,
                          bool>::type = true>
            ITK_INLINE void write(const _vec_type &_vec)
            {
                writeVector<typename custom_stl_is_std_vector<_vec_type>::itemType>(_vec);
            }

            template <typename _map_type,
                      typename std::enable_if<
                          !MathCore::MathTypeInfo<_map_type>::_is_valid::value &&
                              custom_stl_is_std_map<_map_type>::value,
                          bool>::type = true>
            ITK_INLINE void write(const _map_type &_map)
            {
                writeMap<typename custom_stl_is_std_map<_map_type>::keyType,
                         typename custom_stl_is_std_map<_map_type>::valueType>(_map);
            }
        };

    }

}