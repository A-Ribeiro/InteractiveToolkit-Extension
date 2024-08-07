
#include <InteractiveToolkit-Extension/image/PNG.h>
// #include <InteractiveToolkit/InteractiveToolkit.h>
#include <InteractiveToolkit/ITKCommon/Memory.h>
#include <InteractiveToolkit/ITKCommon/StringUtil.h>

#include <png.h>

#include <InteractiveToolkit/ITKCommon/FileSystem/File.h>

//----------------------------------------------------------------------------------
/* The png_jmpbuf() macro, used in error handling, became available in
 * libpng version 1.0.6.  If you want to be able to run your code with older
 * versions of libpng, you must define the macro yourself (but only if it
 * is not already defined by libpng!).
 */
//----------------------------------------------------------------------------------
#ifndef png_jmpbuf
#define png_jmpbuf(png_ptr) ((png_ptr)->jmpbuf)
#endif
//----------------------------------------------------------------------------------
#ifdef _MSC_VER
#pragma warning(disable : 4996)
#endif

namespace ITKExtension
{
    namespace Image
    {
        namespace PNG
        {

            /// \private
            struct DataReadInput
            {
                const char *buffer;
                int size;
                int readed;
            };

            void user_read_data_DataReadInput(png_structp png_ptr, png_bytep data, png_size_t length)
            {
                DataReadInput *dataReadInput = (DataReadInput *)png_get_io_ptr(png_ptr);
                if (dataReadInput->readed + (int)length > dataReadInput->size)
                    png_error(png_ptr, "Read Error!");
                memcpy(data, &(dataReadInput->buffer[dataReadInput->readed]), length);
                dataReadInput->readed += (int)length;
            }

            //----------------------------------------------------------------------------------
            void user_write_data_vector(png_structp png_ptr, png_bytep data, png_size_t length)
            {
                std::vector<char> *output = (std::vector<char> *)png_get_io_ptr(png_ptr);
                output->insert(output->end(), data, data + length);
            }

            //----------------------------------------------------------------------------------
            void user_write_data(png_structp png_ptr,
                                 png_bytep data, png_size_t length)
            {
                //  fwrite((void*)data,sizeof(png_byte),length,(FILE*)png_ptr->io_ptr);
                fwrite((void *)data, sizeof(png_byte), length, (FILE *)png_get_io_ptr(png_ptr));
            }
            //----------------------------------------------------------------------------------
            void user_flush_data(png_structp png_ptr)
            {
                fflush((FILE *)png_get_io_ptr(png_ptr));
            }
            //----------------------------------------------------------------------------------
            bool writePNG(const char *file_name, int w, int h, int chann, char *buffer, bool invertY, std::string *errorStr)
            {
                FILE *fp;
                png_structp png_ptr;
                png_infop info_ptr;
                fp = ITKCommon::FileSystem::File::fopen(file_name, "wb", errorStr);
                if (!fp)
                   return false;                                           // error
                png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, 0, // png_voidp_nullptr,
                                                  0,                        // png_error_ptr_nullptr,
                                                  0                         // png_error_ptr_nullptr
                );
                if (png_ptr == nullptr)
                {
                    fclose(fp);
                    if (errorStr != nullptr)
                        *errorStr = ITKCommon::PrintfToStdString("Error on png_create_write_struct\n");
                    return false; // error
                }
                info_ptr = png_create_info_struct(png_ptr);
                if (info_ptr == nullptr)
                {
                    fclose(fp);
                    png_destroy_write_struct(&png_ptr, 0 // png_infopp_nullptr
                    );
                    if (errorStr != nullptr)
                        *errorStr = ITKCommon::PrintfToStdString("Error on png_create_info_struct\n");
                    return false; // error
                }
                if (setjmp(png_jmpbuf(png_ptr)))
                {
                    // If we get here, we had a problem reading the file
                    fclose(fp);
                    png_destroy_write_struct(&png_ptr, &info_ptr);
                    if (errorStr != nullptr)
                        *errorStr = ITKCommon::PrintfToStdString("Error on png setjmp\n");
                    return false; // error
                }
                png_set_write_fn(png_ptr, fp, user_write_data, user_flush_data);
                png_init_io(png_ptr, fp);
                int colorType;
                switch (chann)
                {
                case 1:
                    colorType = PNG_COLOR_TYPE_GRAY;
                    break;
                case 2:
                    colorType = PNG_COLOR_TYPE_GRAY_ALPHA;
                    break;
                case 3:
                    colorType = PNG_COLOR_TYPE_RGB;
                    break;
                case 4:
                    colorType = PNG_COLOR_TYPE_RGBA;
                    break;
                default:
                    fclose(fp);
                    png_destroy_write_struct(&png_ptr, &info_ptr);
                    if (errorStr != nullptr)
                        *errorStr = ITKCommon::PrintfToStdString("Invalid chann = %i.\n", chann);
                    return false; // error
                }
                png_set_IHDR(png_ptr, info_ptr, w, h,
                             8,                            // bitdepth
                             colorType,                    // color_type
                             PNG_INTERLACE_NONE,           // interlace_type
                             PNG_COMPRESSION_TYPE_DEFAULT, // compression_type
                             PNG_FILTER_TYPE_DEFAULT);     // filter_method
                png_write_info(png_ptr, info_ptr);
                if (invertY)
                {
                    for (int y = 0; y < h; y++)
                        png_write_row(png_ptr, (png_byte *)&buffer[(h - y - 1) * w * chann]);
                }
                else
                {
                    for (int y = 0; y < h; y++)
                        png_write_row(png_ptr, (png_byte *)&buffer[(y)*w * chann]);
                }
                png_write_end(png_ptr, info_ptr);
                png_destroy_write_struct(&png_ptr, &info_ptr);
                fclose(fp);
                return true;
            }
            //----------------------------------------------------------------------------------
            /* Read a PNG file.  You may want to return an error code if the read
             * fails (depending upon the failure).  There are two "prototypes" given
             * here - one where we are given the filename, and we need to open the
             * file, and the other where we are given an open file (possibly with
             * some or all of the magic bytes read - see comments above).
             */
            //----------------------------------------------------------------------------------
            char *readPNG(const char *file_name, int *w, int *h, int *chann, int *pixel_depth, bool invertY, std::string *errorStr)
            { /* We need to open the file */
                png_structp png_ptr;
                png_infop info_ptr;
                unsigned int sig_read = 0;
                // png_uint_32 width, height;
                //   int bit_depth, color_type, interlace_type;
                FILE *fp;
                
                fp = ITKCommon::FileSystem::File::fopen(file_name, "rb", errorStr);
                if (!fp)
                    return nullptr;
                /* Create and initialize the png_struct with the desired error handler
                 * functions.  If you want to use the default stderr and longjump method,
                 * you can supply nullptr for the last three parameters.  We also supply the
                 * the compiler header file version, so that we know if the application
                 * was compiled with a compatible version of the library.  REQUIRED
                 */
                png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
                if (png_ptr == nullptr)
                {
                    fclose(fp);
                    if (errorStr != nullptr)
                        *errorStr = ITKCommon::PrintfToStdString("Error on png_create_read_struct\n");
                    return nullptr;
                }
                /* Allocate/initialize the memory for image information.  REQUIRED. */
                info_ptr = png_create_info_struct(png_ptr);
                if (info_ptr == nullptr)
                {
                    fclose(fp);
                    png_destroy_read_struct(&png_ptr, 0, // png_infopp_nullptr,
                                            0            // png_infopp_nullptr
                    );
                    if (errorStr != nullptr)
                        *errorStr = ITKCommon::PrintfToStdString("Error on png_create_info_struct\n");
                    return nullptr;
                }
                /* Set error handling if you are using the setjmp/longjmp method (this is
                 * the normal method of doing things with libpng).  REQUIRED unless you
                 * set up your own error handlers in the png_create_read_struct() earlier.
                 */
                if (setjmp(png_jmpbuf(png_ptr)))
                {
                    /* Free all of the memory associated with the png_ptr and info_ptr */
                    png_destroy_read_struct(&png_ptr, &info_ptr, 0 // png_infopp_nullptr
                    );
                    fclose(fp);
                    /* If we get here, we had a problem reading the file */
                    if (errorStr != nullptr)
                        *errorStr = ITKCommon::PrintfToStdString("Error on png setjmp\n");
                    return nullptr;
                }
                /* Set up the input control if you are using standard C streams */
                png_init_io(png_ptr, fp);
                /* If we have already read some of the signature */
                png_set_sig_bytes(png_ptr, sig_read);
                /*
                png_bytepp row_pointers = (png_bytepp)png_malloc(png_ptr,info_ptr->height*png_sizeof(png_bytep));
                  row_pointers[0] = (png_bytep)png_malloc(png_ptr,info_ptr->width*info_ptr->pixel_depth);
                  for (int i=1; i<png_ptr->height; i++)
                     row_pointers[i]=(png_bytep)row_pointers[i*info_ptr->width];
                  png_set_rows(png_ptr, info_ptr, row_pointers);
                  */
                /*
                 * If you have enough memory to read in the entire image at once,
                 * and you need to specify only transforms that can be controlled
                 * with one of the PNG_TRANSFORM_* bits (this presently excludes
                 * dithering, filling, setting background, and doing gamma
                 * adjustment), then you can read the entire image (including
                 * pixels) into the info structure with this call:
                 */
                // png_set_expand(
                // PNG_TRANSFORM_SWAP_ENDIAN -- PNG_TRANSFORM_IDENTITY
                png_read_png(png_ptr, info_ptr, PNG_TRANSFORM_SWAP_ENDIAN, 0 // png_voidp_nullptr
                );
                /* At this point you have read the entire image */
                png_uint_32 width = png_get_image_width(png_ptr, info_ptr);
                png_uint_32 height = png_get_image_height(png_ptr, info_ptr);
                png_byte channels = png_get_channels(png_ptr, info_ptr);
                png_byte depth = png_get_bit_depth(png_ptr, info_ptr);
                
                printf("PNG READED (w: %i h: %i chn:%i bits: %i)\n",
                       width,
                       height,
                       channels,
                       depth);

                // char* retorno = new char[width*height*channels*(depth / 8)];
                char *retorno = (char *)ITKCommon::Memory::malloc(width * height * channels * (depth / 8));
                *w = width;
                *h = height;
                *chann = channels;
                *pixel_depth = depth;
                png_bytepp rows = png_get_rows(png_ptr, info_ptr);
                if (invertY)
                {
                    for (unsigned int i = 0; i < height; i++)
                    {
                        memcpy(&retorno[i * width * channels * (depth / 8)],
                               rows[height - 1 - i],
                               width * channels * (depth / 8));
                    }
                }
                else
                {
                    for (unsigned int i = 0; i < height; i++)
                    {
                        memcpy(&retorno[i * width * channels * (depth / 8)],
                               rows[i],
                               width * channels * (depth / 8));
                    }
                }
                /* clean up after the read, and free any memory allocated - REQUIRED */
                png_destroy_read_struct(&png_ptr, &info_ptr, 0 // png_infopp_nullptr
                );
                /* close the file */
                fclose(fp);
                /* that's it */
                return retorno;
            }
            //----------------------------------------------------------------------------------
            char *readPNGFromMemory(const char *input_buffer, int input_buffer_size, int *w, int *h, int *chann, int *pixel_depth, bool invertY)
            {
                png_structp png_ptr;
                png_infop info_ptr;
                unsigned int sig_read = 0;
                // png_uint_32 width, height;
                //   int bit_depth, color_type, interlace_type;
                /* Create and initialize the png_struct with the desired error handler
                 * functions.  If you want to use the default stderr and longjump method,
                 * you can supply nullptr for the last three parameters.  We also supply the
                 * the compiler header file version, so that we know if the application
                 * was compiled with a compatible version of the library.  REQUIRED
                 */
                png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
                if (png_ptr == nullptr)
                {
                    return (nullptr);
                }
                /* Allocate/initialize the memory for image information.  REQUIRED. */
                info_ptr = png_create_info_struct(png_ptr);
                if (info_ptr == nullptr)
                {
                    png_destroy_read_struct(&png_ptr, nullptr, nullptr);
                    return (nullptr);
                }
                /* Set error handling if you are using the setjmp/longjmp method (this is
                 * the normal method of doing things with libpng).  REQUIRED unless you
                 * set up your own error handlers in the png_create_read_struct() earlier.
                 */
                if (setjmp(png_jmpbuf(png_ptr)))
                {
                    /* Free all of the memory associated with the png_ptr and info_ptr */
                    png_destroy_read_struct(&png_ptr, &info_ptr, nullptr);
                    /* If we get here, we had a problem reading the file */
                    return (nullptr);
                }

                DataReadInput inputBuffer;
                inputBuffer.buffer = input_buffer;
                inputBuffer.size = input_buffer_size;
                inputBuffer.readed = 0;
                png_set_read_fn(png_ptr, &inputBuffer, user_read_data_DataReadInput);

                /* Set up the input control if you are using standard C streams */
                // png_init_io(png_ptr, fp);
                /* If we have already read some of the signature */
                png_set_sig_bytes(png_ptr, sig_read);
                /*
                png_bytepp row_pointers = (png_bytepp)png_malloc(png_ptr,info_ptr->height*png_sizeof(png_bytep));
                  row_pointers[0] = (png_bytep)png_malloc(png_ptr,info_ptr->width*info_ptr->pixel_depth);
                  for (int i=1; i<png_ptr->height; i++)
                     row_pointers[i]=(png_bytep)row_pointers[i*info_ptr->width];
                  png_set_rows(png_ptr, info_ptr, row_pointers);
                  */
                /*
                 * If you have enough memory to read in the entire image at once,
                 * and you need to specify only transforms that can be controlled
                 * with one of the PNG_TRANSFORM_* bits (this presently excludes
                 * dithering, filling, setting background, and doing gamma
                 * adjustment), then you can read the entire image (including
                 * pixels) into the info structure with this call:
                 */
                // png_set_expand(
                // PNG_TRANSFORM_SWAP_ENDIAN -- PNG_TRANSFORM_IDENTITY
                png_read_png(png_ptr, info_ptr, PNG_TRANSFORM_SWAP_ENDIAN, nullptr);
                /* At this point you have read the entire image */
                png_uint_32 width = png_get_image_width(png_ptr, info_ptr);
                png_uint_32 height = png_get_image_height(png_ptr, info_ptr);
                png_byte channels = png_get_channels(png_ptr, info_ptr);
                png_byte depth = png_get_bit_depth(png_ptr, info_ptr);
                printf("PNG READED (w: %i h: %i chn:%i bits: %i)\n",
                       width,
                       height,
                       channels,
                       depth);
                // char* retorno = new char[width*height*channels*(depth / 8)];
                char *retorno = (char *)ITKCommon::Memory::malloc(width * height * channels * (depth / 8));
                *w = width;
                *h = height;
                *chann = channels;
                *pixel_depth = depth;
                png_bytepp rows = png_get_rows(png_ptr, info_ptr);
                if (invertY)
                {
                    for (unsigned int i = 0; i < height; i++)
                    {
                        memcpy(&retorno[i * width * channels * (depth / 8)],
                               rows[height - 1 - i],
                               width * channels * (depth / 8));
                    }
                }
                else
                {
                    for (unsigned int i = 0; i < height; i++)
                    {
                        memcpy(&retorno[i * width * channels * (depth / 8)],
                               rows[i],
                               width * channels * (depth / 8));
                    }
                }
                /* clean up after the read, and free any memory allocated - REQUIRED */
                png_destroy_read_struct(&png_ptr, &info_ptr, 0 // png_infopp_nullptr
                );
                /* that's it */
                return retorno;
            }
            //----------------------------------------------------------------------------------
            char *writePNGToMemory(int *output_size, int w, int h, int chann, char *buffer, bool invertY)
            {
                std::vector<char> output;

                png_structp png_ptr;
                png_infop info_ptr;

                png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
                if (png_ptr == nullptr)
                {
                    // error
                    *output_size = 0;
                    return nullptr;
                }
                info_ptr = png_create_info_struct(png_ptr);
                if (info_ptr == nullptr)
                {
                    // error
                    png_destroy_write_struct(&png_ptr, nullptr);
                    *output_size = 0;
                    return nullptr;
                }
                if (setjmp(png_jmpbuf(png_ptr)))
                {
                    // error
                    png_destroy_write_struct(&png_ptr, &info_ptr);
                    *output_size = 0;
                    return nullptr;
                }
                png_set_write_fn(png_ptr, &output, user_write_data_vector, nullptr);
                // png_init_io(png_ptr, fp);
                int colorType;
                switch (chann)
                {
                case 1:
                    colorType = PNG_COLOR_TYPE_GRAY;
                    break;
                case 2:
                    colorType = PNG_COLOR_TYPE_GRAY_ALPHA;
                    break;
                case 3:
                    colorType = PNG_COLOR_TYPE_RGB;
                    break;
                case 4:
                    colorType = PNG_COLOR_TYPE_RGBA;
                    break;
                default:
                    png_destroy_write_struct(&png_ptr, &info_ptr);
                    *output_size = 0;
                    return nullptr; // error
                }
                png_set_IHDR(png_ptr, info_ptr, w, h,
                             8,                            // bitdepth
                             colorType,                    // color_type
                             PNG_INTERLACE_NONE,           // interlace_type
                             PNG_COMPRESSION_TYPE_DEFAULT, // compression_type
                             PNG_FILTER_TYPE_DEFAULT);     // filter_method
                png_write_info(png_ptr, info_ptr);
                if (invertY)
                {
                    for (int y = 0; y < h; y++)
                        png_write_row(png_ptr, (png_byte *)&buffer[(h - y - 1) * w * chann]);
                }
                else
                {
                    for (int y = 0; y < h; y++)
                        png_write_row(png_ptr, (png_byte *)&buffer[(y)*w * chann]);
                }
                png_write_end(png_ptr, info_ptr);
                png_destroy_write_struct(&png_ptr, &info_ptr);

                *output_size = (int)output.size();
                // char* outputBuffer = new char[output.size()];
                char *outputBuffer = (char *)ITKCommon::Memory::malloc(output.size());
                memcpy(outputBuffer, &output[0], output.size());

                return outputBuffer;
            }
            //----------------------------------------------------------------------------------
            void closePNG(char *&buff)
            {
                if (!buff)
                    return;
                ITKCommon::Memory::free(buff);
                // delete[]buff;
                buff = nullptr;
            }
            //----------------------------------------------------------------------------------

            bool isPNGFilename(const char *filename)
            {
                std::string file_lower = ITKCommon::StringUtil::toLower(filename);
                return ITKCommon::StringUtil::endsWith(file_lower, ".png");
            }

        }
    }
}
