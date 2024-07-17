#pragma once

// #include <InteractiveToolkit/InteractiveToolkit.h>
#include <string>
#include <stdio.h>
#include <stdint.h>

namespace ITKWrappers {
    namespace MD5 {
        /// \brief Compute the MD5 hash bytes and return the hex string from a pointer to memory.
        ///
        /// Example:
        ///
        /// \code
        /// #include <aRibeiroData/aRibeiroData.h>
        /// #include <md5-wrapper/md5-wrapper.h>
        /// using namespace aRibeiro;
        /// using namespace md5Wrapper;
        ///
        /// ...
        ///
        /// char* buffer;
        /// int size;
        /// std::string hex_string = MD5::getHexStringHashFromBytes( buffer, size );
        ///
        /// \endcode
        ///
        /// \author Alessandro Ribeiro
        /// \param buffer input buffer to compute the MD5
        /// \param size the size of the buffer in bytes
        /// \return Hex String of the 16bytes MD hash
        ///
        std::string getHexStringHashFromBytes(const uint8_t* buffer, int64_t size);

        /// \brief Compute the MD5 hash bytes from a pointer to memory.
        ///
        /// Example:
        ///
        /// \code
        /// #include <aRibeiroData/aRibeiroData.h>
        /// #include <md5-wrapper/md5-wrapper.h>
        /// using namespace aRibeiro;
        /// using namespace md5Wrapper;
        ///
        /// ...
        ///
        /// char* buffer;
        /// int size;
        /// unsigned char outputbuffer[16];
        /// MD5::get16bytesHashFromBytes( buffer, size, outputbuffer );
        ///
        /// \endcode
        ///
        /// \author Alessandro Ribeiro
        /// \param buffer input buffer to compute the MD5
        /// \param size the size of the buffer in bytes
        /// \param[out] outBuffer the 16 byte output buffer
        ///
        void get16bytesHashFromBytes(const uint8_t* buffer, int64_t size, uint8_t outBuffer[16]);

        /// \brief Compute the MD5 hash bytes and return the hex string from a file.
        ///
        /// It will not allocate the entire file to memory to compute the md5.
        ///
        /// Example:
        ///
        /// \code
        /// #include <aRibeiroData/aRibeiroData.h>
        /// #include <md5-wrapper/md5-wrapper.h>
        /// using namespace aRibeiro;
        /// using namespace md5Wrapper;
        ///
        /// ...
        ///
        /// std::string hex_string = MD5::getHexStringHashFromFile( "input_file.bin" );
        ///
        /// \endcode
        ///
        /// \author Alessandro Ribeiro
        /// \param filename input file to compute the MD5
        /// \return Hex String of the 16bytes MD hash
        ///
        std::string getHexStringHashFromFile(const char* filename, std::string *errorStr = NULL);

        /// \brief Compute the MD5 hash bytes from a file.
        ///
        /// It will not allocate the entire file to memory to compute the md5.
        ///
        /// Example:
        ///
        /// \code
        /// #include <aRibeiroData/aRibeiroData.h>
        /// #include <md5-wrapper/md5-wrapper.h>
        /// using namespace aRibeiro;
        /// using namespace md5Wrapper;
        ///
        /// ...
        ///
        /// unsigned char outputbuffer[16];
        /// MD5::get16bytesHashFromFile( "input_file.bin", outputbuffer );
        ///
        /// \endcode
        ///
        /// \author Alessandro Ribeiro
        /// \param filename input file to compute the MD5
        /// \param[out] outBuffer the 16 byte output buffer
        ///
        bool get16bytesHashFromFile(const char* filename, uint8_t outBuffer[16], std::string *errorStr = NULL);

        /// \brief Converts the 16bytes representation to a 32 character string.
        ///
        /// Example:
        ///
        /// \code
        /// #include <aRibeiroData/aRibeiroData.h>
        /// #include <md5-wrapper/md5-wrapper.h>
        /// using namespace aRibeiro;
        /// using namespace md5Wrapper;
        ///
        /// ...
        ///
        /// unsigned char inputBuffer[16];
        /// std::string result = MD5::_16BytesToHexString( inputBuffer );
        ///
        /// \endcode
        ///
        /// \author Alessandro Ribeiro
        /// \param md5 md5 16 bytes input
        /// \return Hex String of the 16bytes MD hash
        ///
        std::string _16BytesToHexString(uint8_t md5[16]);
    }
}
