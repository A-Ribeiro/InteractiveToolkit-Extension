#pragma once

#include <stdlib.h> // nullptr
#include <string>

namespace ITKExtension
{
    namespace Image
    {
        namespace JPG
        {

            /// \brief Read JPG format from file
            ///
            /// It returns the raw imagem buffer and: width, height, number of channels, pixel_depth.
            ///
            /// \code
                    ///
            /// int w, h, chn, depth;
            /// char*bufferChar;
            ///
            /// bufferChar = JPGHelper::readJPG( "file.jpg", &w, &h, &chn, &depth);
            /// if (bufferChar != nullptr) {
            ///     ...
            ///     JPGHelper::closeJPG(bufferChar);
            /// }
            /// \endcode
            ///
            /// \author Alessandro Ribeiro
            /// \param file_name Filename to load
            /// \param[out] w width
            /// \param[out] h height
            /// \param[out] chann channels
            /// \param[out] pixel_depth pixel depth
            /// \param invertY should invert the loaded image vertically
            /// \param[out] gamma the gamma value stored in JPG file
            /// \return The raw image buffer or nullptr if cannot open file.
            ///
            char *readJPG(const char *file_name, int *w, int *h, int *chann, int *pixel_depth, bool invertY = false, float *gamma = nullptr, std::string *errorStr = nullptr);

            bool writeJPG(const char *file_name, int w, int h, int chann, char *buffer, int quality = 90, bool invertY = false, std::string *errorStr = nullptr);


            /// \brief Read JPG format from memory stream
            ///
            /// It returns the raw imagem buffer and: width, height, number of channels, pixel_depth.
            ///
            /// \code
                    ///
            /// const char *input_buffer;
            /// int input_buffer_size;
            ///
            /// int w, h, chn, depth;
            /// char*bufferChar;
            ///
            /// bufferChar = JPGHelper::readJPGFromMemory( input_buffer, input_buffer_size, &w, &h, &chn, &depth);
            /// if (bufferChar != nullptr) {
            ///     ...
            ///     JPGHelper::closeJPG(bufferChar);
            /// }
            /// \endcode
            ///
            /// \author Alessandro Ribeiro
            /// \param input_buffer Input raw JPG compressed buffer
            /// \param input_buffer_size Buffer size
            /// \param[out] w width
            /// \param[out] h height
            /// \param[out] chann channels
            /// \param[out] pixel_depth pixel depth
            /// \param invertY should invert the loaded image vertically
            /// \param[out] gamma the gamma value stored in JPG file
            /// \return The raw image buffer or nullptr if cannot open file.
            ///
            char *readJPGFromMemory(const char *input_buffer, int input_buffer_size, int *w, int *h, int *chann, int *pixel_depth, bool invertY = false, float *gamma = nullptr);

            char *writeJPGToMemory(int *output_size, int w, int h, int chann, char *buffer, int quality = 90, bool invertY = false);


            /// \brief Closes the image buffer after a read or memory write.
            ///
            /// Should be called after any success read or memory write JPG image.
            ///
            /// \code
                    ///
            /// int w, h, chn, depth;
            /// char*bufferChar;
            ///
            /// bufferChar = JPGHelper::readJPG( "file.jpg", &w, &h, &chn, &depth);
            /// if (bufferChar != nullptr) {
            ///     ...
            ///     JPGHelper::closeJPG(bufferChar);
            /// }
            /// \endcode
            ///
            /// \author Alessandro Ribeiro
            /// \param buff pointer to a valid readed buffer
            ///
            void closeJPG(char *&buff);

            bool isJPGFilename(const char *filename);
        }
    }
}