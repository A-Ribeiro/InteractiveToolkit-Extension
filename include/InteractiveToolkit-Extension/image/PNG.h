#pragma once

#include <stdio.h>
#include <string>

namespace ITKExtension
{
    namespace Image
    {
        namespace PNG
        {

            /// \brief Read PNG format from file
            ///
            /// It returns the raw imagem buffer and: width, height, number of channels, pixel_depth.
            ///
            /// \code
            /// #include <aRibeiroData/aRibeiroData.h>
            /// using namespace aRibeiro;
            ///
            /// int w, h, chn, depth;
            /// char*bufferChar;
            ///
            /// bufferChar = PNGHelper::readPNG( "file.png", &w, &h, &chn, &depth);
            /// if (bufferChar != nullptr) {
            ///     ...
            ///     PNGHelper::closePNG(bufferChar);
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
            /// \return The raw image buffer or nullptr if cannot open file.
            ///
            char *readPNG(const char *file_name, int *w, int *h, int *chann, int *pixel_depth, bool invertY = false, std::string *errorStr = nullptr);

            /// \brief Write PNG format to file
            ///
            /// \code
            /// #include <aRibeiroData/aRibeiroData.h>
            /// using namespace aRibeiro;
            ///
            /// int w, h;
            ///
            /// // write RGBA
            /// char* buffer_rgba;
            /// PNGHelper::writePNG("output_rgb.png", w, h, 4, (char*)buffer_rgba);
            ///
            /// // write RGB
            /// char* buffer_rgb;
            /// PNGHelper::writePNG("output_rgb.png", w, h, 3, (char*)buffer_rgb);
            ///
            /// // write Gray Scale
            /// char* buffer_gray;
            /// PNGHelper::writePNG("output_rgb.png", w, h, 1, (char*)buffer_gray);
            ///
            /// \endcode
            ///
            /// \author Alessandro Ribeiro
            /// \param file_name Filename to load
            /// \param w width
            /// \param h height
            /// \param chann channels
            /// \param buffer input image buffer
            /// \param invertY should invert the loaded image vertically
            ///
            bool writePNG(const char *file_name, int w, int h, int chann, char *buffer, bool invertY = false, std::string *errorStr = nullptr);

            /// \brief Closes the image buffer after a read or memory write.
            ///
            /// Should be called after any success read or memory write PNG image.
            ///
            /// \code
            /// #include <aRibeiroData/aRibeiroData.h>
            /// using namespace aRibeiro;
            ///
            /// int w, h, chn, depth;
            /// char*bufferChar;
            ///
            /// bufferChar = PNGHelper::readPNG( "file.png", &w, &h, &chn, &depth);
            /// if (bufferChar != nullptr) {
            ///     ...
            ///     PNGHelper::closePNG(bufferChar);
            /// }
            /// \endcode
            ///
            /// \author Alessandro Ribeiro
            /// \param buff pointer to a valid readed buffer
            ///
            void closePNG(char *&buff);

            /// \brief Read PNG format from memory stream
            ///
            /// It returns the raw imagem buffer and: width, height, number of channels, pixel_depth.
            ///
            /// \code
            /// #include <aRibeiroData/aRibeiroData.h>
            /// using namespace aRibeiro;
            ///
            /// const char *input_buffer;
            /// int input_buffer_size;
            ///
            /// int w, h, chn, depth;
            /// char*bufferChar;
            ///
            /// bufferChar = PNGHelper::readPNGFromMemory( input_buffer, input_buffer_size, &w, &h, &chn, &depth);
            /// if (bufferChar != nullptr) {
            ///     ...
            ///     PNGHelper::closePNG(bufferChar);
            /// }
            /// \endcode
            ///
            /// \author Alessandro Ribeiro
            /// \param input_buffer Input raw PNG compressed buffer
            /// \param input_buffer_size Buffer size
            /// \param[out] w width
            /// \param[out] h height
            /// \param[out] chann channels
            /// \param[out] pixel_depth pixel depth
            /// \param invertY should invert the loaded image vertically
            /// \return The raw image buffer or nullptr if cannot open file.
            ///
            char *readPNGFromMemory(const char *input_buffer, int input_buffer_size, int *w, int *h, int *chann, int *pixel_depth, bool invertY = false);

            /// \brief Write PNG format to a memory stream
            ///
            /// \code
            /// #include <aRibeiroData/aRibeiroData.h>
            /// using namespace aRibeiro;
            ///
            /// int w, h;
            /// int output_size;
            /// char* output_buffer;
            ///
            /// // write RGBA
            /// char* buffer_rgba;
            /// output_buffer = PNGHelper::writePNGToMemory( &output_size, w, h, 4, (char*)buffer_rgba);
            /// if ( output_buffer ) {
            ///     ...
            ///     PNGHelper::closePNG(output_buffer);
            /// }
            ///
            /// // write RGB
            /// char* buffer_rgb;
            /// output_buffer = PNGHelper::writePNGToMemory( &output_size, w, h, 3, (char*)buffer_rgb);
            /// if ( output_buffer ) {
            ///     ...
            ///     PNGHelper::closePNG(output_buffer);
            /// }
            ///
            /// // write Gray Scale
            /// char* buffer_gray;
            /// output_buffer = PNGHelper::writePNGToMemory( &output_size, w, h, 1, (char*)buffer_gray);
            /// if ( output_buffer ) {
            ///     ...
            ///     PNGHelper::closePNG(output_buffer);
            /// }
            ///
            /// \endcode
            ///
            /// \author Alessandro Ribeiro
            /// \param[out] output_size the writen buffer size
            /// \param w width
            /// \param h height
            /// \param chann channels
            /// \param buffer input image buffer
            /// \param invertY should invert the loaded image vertically
            /// \return The compressed PNG buffer
            ///
            char *writePNGToMemory(int *output_size, int w, int h, int chann, char *buffer, bool invertY = false);

            bool isPNGFilename(const char *filename);
        }
    }
}
