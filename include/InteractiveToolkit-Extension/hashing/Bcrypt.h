#pragma once

#include <string>
#include <vector>
#include <stdint.h>

namespace ITKExtension
{
    namespace Hashing
    {
        // bcrypt hashing
        namespace Bcrypt
        {
            void random_salt(uint8_t salt[16]);
            std::string salt_to_string(int cost, uint8_t salt[16], char format = 'y');
            void string_to_salt(const std::string &saltStr, int *cost, uint8_t salt[16]);

            // Generate bcrypt hash with cost factor (4-31, default 10)
            // format can be 'a', 'b', or 'y' (default 'y' for PHP compatibility)
            std::string hash(const std::string &password, int cost = 10, uint8_t salt_from_parameter[16] = nullptr, char format = 'y');
            // Verify password against bcrypt hash
            bool verify(const std::string &password, const std::string &hash);
        }
    }
}
