#pragma once

#include "Reader.h"

namespace ITKExtension
{
    namespace IO
    {

        class AdvancedReader : public Reader
        {

            // vector
            template <typename _vec_type>
            ITK_INLINE void readVector(std::vector<_vec_type> *v)
            {
                uint32_t size = readUInt32();
                v->resize(size);
                for (uint32_t i = 0; i < size; i++)
                    (*v)[i] = read<_vec_type>();
            }

            // map
            template <typename _map_key, typename _map_type>
            ITK_INLINE void readMap(std::unordered_map<_map_key, _map_type> *v)
            {
                uint32_t size = readUInt32();
                v->clear();
                for (uint32_t i = 0; i < size; i++)
                {
                    _map_key key = read<_map_key>();
                    _map_type value = read<_map_type>();
                    (*v)[key] = value;
                }
            }

        public:
            AdvancedReader()
            {
            }

            template <typename _math_type,
                      typename std::enable_if<
                          MathCore::MathTypeInfo<_math_type>::_is_valid::value &&
                              MathCore::MathTypeInfo<_math_type>::_is_vec::value,
                          bool>::type = true>
            ITK_INLINE _math_type read()
            {
                _math_type result = _math_type();
                for (int i = 0; i < (int)_math_type::array_count; i++)
                    result[i] = read<typename MathCore::MathTypeInfo<_math_type>::_type>();
                return result;
            }

            template <typename _math_type,
                      typename std::enable_if<
                          MathCore::MathTypeInfo<_math_type>::_is_valid::value &&
                              !MathCore::MathTypeInfo<_math_type>::_is_vec::value,
                          bool>::type = true>
            ITK_INLINE _math_type read()
            {
                _math_type result = _math_type();
                for (int c = 0; c < (int)_math_type::cols; c++)
                    for (int r = 0; r < (int)_math_type::rows; r++)
                        result(r, c) = read<typename MathCore::MathTypeInfo<_math_type>::_type>();
                return result;
            }

#define CREATE_READ_PATTERN(_Type, _Fnc)                                   \
    template <typename _math_type,                                         \
              typename std::enable_if<                                     \
                  !MathCore::MathTypeInfo<_math_type>::_is_valid::value && \
                      std::is_same<_math_type, _Type>::value,              \
                  bool>::type = true>                                      \
    ITK_INLINE _math_type read()                                           \
    {                                                                      \
        return _Fnc();                                                     \
    }

            CREATE_READ_PATTERN(uint8_t, readUInt8)
            CREATE_READ_PATTERN(int8_t, readInt8)

            CREATE_READ_PATTERN(uint16_t, readUInt16)
            CREATE_READ_PATTERN(int16_t, readInt16)

            CREATE_READ_PATTERN(uint32_t, readUInt32)
            CREATE_READ_PATTERN(int32_t, readInt32)

            CREATE_READ_PATTERN(uint64_t, readUInt64)
            CREATE_READ_PATTERN(int64_t, readInt64)

            CREATE_READ_PATTERN(float, readFloat)
            CREATE_READ_PATTERN(double, readDouble)

            CREATE_READ_PATTERN(bool, readBool)
            CREATE_READ_PATTERN(std::string, readString)

#undef CREATE_READ_PATTERN

            template <typename _vec_type,
                      typename std::enable_if<
                          !MathCore::MathTypeInfo<_vec_type>::_is_valid::value &&
                              custom_stl_is_std_vector<_vec_type>::value,
                          bool>::type = true>
            ITK_INLINE _vec_type read()
            {
                _vec_type result(0);
                readVector<typename custom_stl_is_std_vector<_vec_type>::itemType>(&result);
                return result;
            }

            template <typename _map_type,
                      typename std::enable_if<
                          !MathCore::MathTypeInfo<_map_type>::_is_valid::value &&
                              custom_stl_is_std_map<_map_type>::value,
                          bool>::type = true>
            ITK_INLINE _map_type read()
            {
                _map_type result;
                readMap<typename custom_stl_is_std_map<_map_type>::keyType,
                        typename custom_stl_is_std_map<_map_type>::valueType>(&result);
                return result;
            }
        };

    }

}