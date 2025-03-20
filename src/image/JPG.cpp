#pragma warning(disable : 4996)

#include <InteractiveToolkit-Extension/image/JPG.h>
// #include <InteractiveToolkit/InteractiveToolkit.h>
#include <InteractiveToolkit/ITKCommon/Memory.h>
#include <InteractiveToolkit/ITKCommon/StringUtil.h>
#include <jpeglib.h>
#include <setjmp.h>

#include <InteractiveToolkit/ITKCommon/FileSystem/File.h>

#include <stdint.h>

namespace ITKExtension
{
	namespace Image
	{
		namespace JPG
		{

			/*
			 * ERROR HANDLING:
			 *
			 * The JPEG library's standard error handler (jerror.c) is divided into
			 * several "methods" which you can override individually.  This lets you
			 * adjust the behavior without duplicating a lot of code, which you might
			 * have to update with each future release.
			 *
			 * Our example here shows how to override the "error_exit" method so that
			 * control is returned to the library's caller when a fatal error occurs,
			 * rather than calling exit() as the standard error_exit method does.
			 *
			 * We use C's setjmp/longjmp facility to return control.  This means that the
			 * routine which calls the JPEG library must first execute a setjmp() call to
			 * establish the return point.  We want the replacement error_exit to do a
			 * longjmp().  But we need to make the setjmp buffer accessible to the
			 * error_exit routine.  To do this, we make a private extension of the
			 * standard JPEG error handler object.  (If we were using C++, we'd say we
			 * were making a subclass of the regular error handler.)
			 *
			 * Here's the extended error handler struct:
			 */

			struct my_error_mgr
			{
				struct jpeg_error_mgr pub; /* "public" fields */

				jmp_buf setjmp_buffer; /* for return to caller */
			};

			typedef struct my_error_mgr *my_error_ptr;

			METHODDEF(void)
			my_error_exit(j_common_ptr cinfo)
			{
				/* cinfo->err really points to a my_error_mgr struct, so coerce pointer */
				my_error_ptr myerr = (my_error_ptr)cinfo->err;

				/* Always display the message. */
				/* We could postpone this until after returning, if we chose. */
				(*cinfo->err->output_message)(cinfo);

				/* Return control to the setjmp point */
				longjmp(myerr->setjmp_buffer, 1);
			}

			char *readJPG(const char *filename, int *w, int *h, int *chann, int *pixel_depth, bool invertY, float *gamma, std::string *errorStr)
			{

				char *result = nullptr;

				/* This struct contains the JPEG decompression parameters and pointers to
				 * working space (which is allocated as needed by the JPEG library).
				 */
				struct jpeg_decompress_struct cinfo;
				/* We use our private extension JPEG error handler.
				 * Note that this struct must live as long as the main JPEG parameter
				 * struct, to avoid dangling-pointer problems.
				 */
				struct my_error_mgr jerr;
				/* More stuff */
				FILE *infile;	   /* source file */
				JSAMPARRAY buffer; /* Output row buffer */
				int row_stride;	   /* physical row width in output buffer */

				/* In this example we want to open the input file before doing anything else,
				 * so that the setjmp() error recovery below can assume the file is open.
				 * VERY IMPORTANT: use "b" option to fopen() if you are on a machine that
				 * requires it in order to read binary files.
				 */

				infile = ITKCommon::FileSystem::File::fopen(filename, "rb", errorStr);
				if (!infile)
				{
					fprintf(stderr, "can't open %s\n", filename);
					return nullptr;
				}

				/* Step 1: allocate and initialize JPEG decompression object */

				/* We set up the normal JPEG error routines, then override error_exit. */
				cinfo.err = jpeg_std_error(&jerr.pub);
				jerr.pub.error_exit = my_error_exit;
				/* Establish the setjmp return context for my_error_exit to use. */
				if (setjmp(jerr.setjmp_buffer))
				{
					/* If we get here, the JPEG code has signaled an error.
					 * We need to clean up the JPEG object, close the input file, and return.
					 */
					jpeg_destroy_decompress(&cinfo);
					fclose(infile);

					if (result != nullptr)
						ITKCommon::Memory::free(result);
					// delete[] result;

					if (errorStr != nullptr)
						*errorStr = "JPG Signaled an Error.\n";
					return nullptr;
				}
				/* Now we can initialize the JPEG decompression object. */
				jpeg_create_decompress(&cinfo);

				/* Step 2: specify data source (eg, a file) */

				jpeg_stdio_src(&cinfo, infile);

				/* Step 3: read file parameters with jpeg_read_header() */

				(void)jpeg_read_header(&cinfo, TRUE);
				/* We can ignore the return value from jpeg_read_header since
				 *   (a) suspension is not possible with the stdio data source, and
				 *   (b) we passed TRUE to reject a tables-only JPEG file as an error.
				 * See libjpeg.txt for more info.
				 */

				/* Step 4: set parameters for decompression */

				/* In this example, we don't need to change any of the defaults set by
				 * jpeg_read_header(), so we do nothing here.
				 */

				/* Step 5: Start decompressor */

				(void)jpeg_start_decompress(&cinfo);
				/* We can ignore the return value since suspension is not possible
				 * with the stdio data source.
				 */

				/* We may need to do some setup of our own at this point before reading
				 * the data.  After jpeg_start_decompress() we have the correct scaled
				 * output image dimensions available, as well as the output colormap
				 * if we asked for color quantization.
				 * In this example, we need to make an output work buffer of the right size.
				 */
				/* JSAMPLEs per row in output buffer */
				row_stride = cinfo.output_width * cinfo.output_components;
				/* Make a one-row-high sample array that will go away when done with image */
				buffer = (*cinfo.mem->alloc_sarray)((j_common_ptr)&cinfo, JPOOL_IMAGE, row_stride, 1);

				// result = new char[cinfo.output_width * cinfo.output_height * cinfo.output_components];
				result = (char *)ITKCommon::Memory::malloc(cinfo.output_width * cinfo.output_height * cinfo.output_components);
				*w = cinfo.output_width;
				*h = cinfo.output_height;
				*chann = cinfo.output_components;
				*pixel_depth = 8;
				if (gamma != nullptr)
					*gamma = (float)cinfo.output_gamma;

				/* Step 6: while (scan lines remain to be read) */
				/*           jpeg_read_scanlines(...); */

				/* Here we use the library's state variable cinfo.output_scanline as the
				 * loop counter, so that we don't have to keep track ourselves.
				 */
				while (cinfo.output_scanline < cinfo.output_height)
				{
					/* jpeg_read_scanlines expects an array of pointers to scanlines.
					 * Here the array is only one element long, but you could ask for
					 * more than one scanline at a time if that's more convenient.
					 */
					(void)jpeg_read_scanlines(&cinfo, buffer, 1);
					/* Assume put_scanline_someplace wants a pointer and sample count. */
					// put_scanline_someplace(buffer[0], row_stride);
					if (invertY)
						memcpy(&result[(cinfo.output_height - 1 - (cinfo.output_scanline - 1)) * row_stride], buffer[0], row_stride);
					else
						memcpy(&result[(cinfo.output_scanline - 1) * row_stride], buffer[0], row_stride);
				}

				/* Step 7: Finish decompression */

				(void)jpeg_finish_decompress(&cinfo);
				/* We can ignore the return value since suspension is not possible
				 * with the stdio data source.
				 */

				/* Step 8: Release JPEG decompression object */

				/* This is an important step since it will release a good deal of memory. */
				jpeg_destroy_decompress(&cinfo);

				/* After finish_decompress, we can close the input file.
				 * Here we postpone it until after no more JPEG errors are possible,
				 * so as to simplify the setjmp error logic above.  (Actually, I don't
				 * think that jpeg_destroy can do an error exit, but why assume anything...)
				 */
				fclose(infile);

				/* At this point you may want to check to see whether any corrupt-data
				 * warnings occurred (test whether jerr.pub.num_warnings is nonzero).
				 */

				/* And we're done! */

				return result;
			}

			bool writeJPG(const char *file_name, int w, int h, int chann, char *buffer, int quality, bool invertY, std::string *errorStr)
			{
				/* This struct contains the JPEG compression parameters and pointers to
				 * working space (which is allocated as needed by the JPEG library).
				 * It is possible to have several such structures, representing multiple
				 * compression/decompression processes, in existence at once.  We refer
				 * to any one struct (and its associated working data) as a "JPEG object".
				 */
				struct jpeg_compress_struct cinfo;
				/* This struct represents a JPEG error handler.  It is declared separately
				 * because applications often want to supply a specialized error handler
				 * (see the second half of this file for an example).  But here we just
				 * take the easy way out and use the standard error handler, which will
				 * print a message on stderr and call exit() if compression fails.
				 * Note that this struct must live as long as the main JPEG parameter
				 * struct, to avoid dangling-pointer problems.
				 */
				struct jpeg_error_mgr jerr;
				/* More stuff */
				FILE *outfile;			 /* target file */
				JSAMPROW row_pointer[1]; /* pointer to JSAMPLE row[s] */
				int row_stride;			 /* physical row width in image buffer */

				/* Step 1: allocate and initialize JPEG compression object */

				/* We have to set up the error handler first, in case the initialization
				 * step fails.  (Unlikely, but it could happen if you are out of memory.)
				 * This routine fills in the contents of struct jerr, and returns jerr's
				 * address which we place into the link field in cinfo.
				 */
				cinfo.err = jpeg_std_error(&jerr);
				/* Now we can initialize the JPEG compression object. */
				jpeg_create_compress(&cinfo);

				/* Step 2: specify data destination (eg, a file) */
				/* Note: steps 2 and 3 can be done in either order. */

				/* Here we use the library-supplied code to send compressed data to a
				 * stdio stream.  You can also write your own code to do something else.
				 * VERY IMPORTANT: use "b" option to fopen() if you are on a machine that
				 * requires it in order to write binary files.
				 */
				outfile = ITKCommon::FileSystem::File::fopen(file_name, "wb", errorStr);
				if (!outfile)
				{
					fprintf(stderr, "can't open %s\n", file_name);
					return false;
				}
				jpeg_stdio_dest(&cinfo, outfile);

				/* Step 3: set parameters for compression */

				/* First we supply a description of the input image.
				 * Four fields of the cinfo struct must be filled in:
				 */
				cinfo.image_width = w; /* image width and height, in pixels */
				cinfo.image_height = h;
				cinfo.input_components = chann; /* # of color components per pixel */
				if (chann == 3)
					cinfo.in_color_space = JCS_RGB; /* colorspace of input image */
				else if (chann == 1)
					cinfo.in_color_space = JCS_GRAYSCALE;
				else
				{
					if (errorStr != nullptr)
						*errorStr = ITKCommon::PrintfToStdString("JPEG invalid number of channels: %i\n", chann);
					fprintf(stderr, "JPEG invalid number of channels: %i\n", chann);
					return false;
				}
				/* Now use the library's routine to set default compression parameters.
				 * (You must set at least cinfo.in_color_space before calling this,
				 * since the defaults depend on the source color space.)
				 */
				jpeg_set_defaults(&cinfo);
				/* Now you can set any non-default parameters you wish to.
				 * Here we just illustrate the use of quality (quantization table) scaling:
				 */
				jpeg_set_quality(&cinfo, quality, TRUE /* limit to baseline-JPEG values */);

				/* Step 4: Start compressor */

				/* TRUE ensures that we will write a complete interchange-JPEG file.
				 * Pass TRUE unless you are very sure of what you're doing.
				 */
				jpeg_start_compress(&cinfo, TRUE);

				/* Step 5: while (scan lines remain to be written) */
				/*           jpeg_write_scanlines(...); */

				/* Here we use the library's state variable cinfo.next_scanline as the
				 * loop counter, so that we don't have to keep track ourselves.
				 * To keep things simple, we pass one scanline per call; you can pass
				 * more if you wish, though.
				 */
				row_stride = w * chann; /* JSAMPLEs per row in image_buffer */

				while (cinfo.next_scanline < cinfo.image_height)
				{
					/* jpeg_write_scanlines expects an array of pointers to scanlines.
					 * Here the array is only one element long, but you could pass
					 * more than one scanline at a time if that's more convenient.
					 */
					if (invertY)
						row_pointer[0] = (JSAMPROW)&buffer[(h - 1 - cinfo.next_scanline) * row_stride];
					else
						row_pointer[0] = (JSAMPROW)&buffer[cinfo.next_scanline * row_stride];
					(void)jpeg_write_scanlines(&cinfo, row_pointer, 1);
				}

				/* Step 6: Finish compression */

				jpeg_finish_compress(&cinfo);
				/* After finish_compress, we can close the output file. */
				fclose(outfile);

				/* Step 7: release JPEG compression object */

				/* This is an important step since it will release a good deal of memory. */
				jpeg_destroy_compress(&cinfo);

				/* And we're done! */
				return true;
			}

			char *readJPGFromMemory(const char *input_buffer, int input_buffer_size, int *w, int *h, int *chann, int *pixel_depth, bool invertY, float *gamma)
			{

				char *result = nullptr;

				/* This struct contains the JPEG decompression parameters and pointers to
				 * working space (which is allocated as needed by the JPEG library).
				 */
				struct jpeg_decompress_struct cinfo;
				/* We use our private extension JPEG error handler.
				 * Note that this struct must live as long as the main JPEG parameter
				 * struct, to avoid dangling-pointer problems.
				 */
				struct my_error_mgr jerr;
				/* More stuff */
				JSAMPARRAY buffer; /* Output row buffer */
				int row_stride;	   /* physical row width in output buffer */

				/* In this example we want to open the input file before doing anything else,
				 * so that the setjmp() error recovery below can assume the file is open.
				 * VERY IMPORTANT: use "b" option to fopen() if you are on a machine that
				 * requires it in order to read binary files.
				 */

				// if ((infile = fopen(filename, "rb")) == nullptr) {
				// fprintf(stderr, "can't open %s\n", filename);
				// return 0;
				//}

				/* Step 1: allocate and initialize JPEG decompression object */

				/* We set up the normal JPEG error routines, then override error_exit. */
				cinfo.err = jpeg_std_error(&jerr.pub);
				jerr.pub.error_exit = my_error_exit;
				/* Establish the setjmp return context for my_error_exit to use. */
				if (setjmp(jerr.setjmp_buffer))
				{
					/* If we get here, the JPEG code has signaled an error.
					 * We need to clean up the JPEG object, close the input file, and return.
					 */
					jpeg_destroy_decompress(&cinfo);
					// fclose(infile);

					if (result != nullptr)
						ITKCommon::Memory::free(result);
					// delete[] result;

					return nullptr;
				}
				/* Now we can initialize the JPEG decompression object. */
				jpeg_create_decompress(&cinfo);

				/* Step 2: specify data source (eg, a file) */
				jpeg_mem_src(&cinfo, (unsigned char *)input_buffer, input_buffer_size);

				/* Step 3: read file parameters with jpeg_read_header() */

				(void)jpeg_read_header(&cinfo, TRUE);
				/* We can ignore the return value from jpeg_read_header since
				 *   (a) suspension is not possible with the stdio data source, and
				 *   (b) we passed TRUE to reject a tables-only JPEG file as an error.
				 * See libjpeg.txt for more info.
				 */

				/* Step 4: set parameters for decompression */

				/* In this example, we don't need to change any of the defaults set by
				 * jpeg_read_header(), so we do nothing here.
				 */

				/* Step 5: Start decompressor */

				(void)jpeg_start_decompress(&cinfo);
				/* We can ignore the return value since suspension is not possible
				 * with the stdio data source.
				 */

				/* We may need to do some setup of our own at this point before reading
				 * the data.  After jpeg_start_decompress() we have the correct scaled
				 * output image dimensions available, as well as the output colormap
				 * if we asked for color quantization.
				 * In this example, we need to make an output work buffer of the right size.
				 */
				/* JSAMPLEs per row in output buffer */
				row_stride = cinfo.output_width * cinfo.output_components;
				/* Make a one-row-high sample array that will go away when done with image */
				buffer = (*cinfo.mem->alloc_sarray)((j_common_ptr)&cinfo, JPOOL_IMAGE, row_stride, 1);

				// result = new char[cinfo.output_width * cinfo.output_height * cinfo.output_components];
				result = (char *)ITKCommon::Memory::malloc(cinfo.output_width * cinfo.output_height * cinfo.output_components);
				*w = cinfo.output_width;
				*h = cinfo.output_height;
				*chann = cinfo.output_components;
				*pixel_depth = 8;
				if (gamma != nullptr)
					*gamma = (float)cinfo.output_gamma;

				/* Step 6: while (scan lines remain to be read) */
				/*           jpeg_read_scanlines(...); */

				/* Here we use the library's state variable cinfo.output_scanline as the
				 * loop counter, so that we don't have to keep track ourselves.
				 */
				while (cinfo.output_scanline < cinfo.output_height)
				{
					/* jpeg_read_scanlines expects an array of pointers to scanlines.
					 * Here the array is only one element long, but you could ask for
					 * more than one scanline at a time if that's more convenient.
					 */
					(void)jpeg_read_scanlines(&cinfo, buffer, 1);
					/* Assume put_scanline_someplace wants a pointer and sample count. */
					// put_scanline_someplace(buffer[0], row_stride);
					if (invertY)
						memcpy(&result[(cinfo.output_height - 1 - (cinfo.output_scanline - 1)) * row_stride], buffer[0], row_stride);
					else
						memcpy(&result[(cinfo.output_scanline - 1) * row_stride], buffer[0], row_stride);
				}

				/* Step 7: Finish decompression */

				(void)jpeg_finish_decompress(&cinfo);
				/* We can ignore the return value since suspension is not possible
				 * with the stdio data source.
				 */

				/* Step 8: Release JPEG decompression object */

				/* This is an important step since it will release a good deal of memory. */
				jpeg_destroy_decompress(&cinfo);

				/* After finish_decompress, we can close the input file.
				 * Here we postpone it until after no more JPEG errors are possible,
				 * so as to simplify the setjmp error logic above.  (Actually, I don't
				 * think that jpeg_destroy can do an error exit, but why assume anything...)
				 */
				// fclose(infile);

				/* At this point you may want to check to see whether any corrupt-data
				 * warnings occurred (test whether jerr.pub.num_warnings is nonzero).
				 */

				/* And we're done! */

				return result;
			}

			char *writeJPGToMemory(int *output_size, int w, int h, int chann, char *buffer, int quality, bool invertY)
			{
				/* This struct contains the JPEG compression parameters and pointers to
				 * working space (which is allocated as needed by the JPEG library).
				 * It is possible to have several such structures, representing multiple
				 * compression/decompression processes, in existence at once.  We refer
				 * to any one struct (and its associated working data) as a "JPEG object".
				 */
				struct jpeg_compress_struct cinfo;
				/* This struct represents a JPEG error handler.  It is declared separately
				 * because applications often want to supply a specialized error handler
				 * (see the second half of this file for an example).  But here we just
				 * take the easy way out and use the standard error handler, which will
				 * print a message on stderr and call exit() if compression fails.
				 * Note that this struct must live as long as the main JPEG parameter
				 * struct, to avoid dangling-pointer problems.
				 */
				struct jpeg_error_mgr jerr;
				/* More stuff */
				// FILE *outfile;           /* target file */
				JSAMPROW row_pointer[1]; /* pointer to JSAMPLE row[s] */
				int row_stride;			 /* physical row width in image buffer */

				/* Step 1: allocate and initialize JPEG compression object */

				/* We have to set up the error handler first, in case the initialization
				 * step fails.  (Unlikely, but it could happen if you are out of memory.)
				 * This routine fills in the contents of struct jerr, and returns jerr's
				 * address which we place into the link field in cinfo.
				 */
				cinfo.err = jpeg_std_error(&jerr);
				/* Now we can initialize the JPEG compression object. */
				jpeg_create_compress(&cinfo);

				/* Step 2: specify data destination (eg, a file) */
				/* Note: steps 2 and 3 can be done in either order. */

				/* Here we use the library-supplied code to send compressed data to a
				 * stdio stream.  You can also write your own code to do something else.
				 * VERY IMPORTANT: use "b" option to fopen() if you are on a machine that
				 * requires it in order to write binary files.
				 */
				// if ((outfile = fopen(file_name, "wb")) == nullptr)
				// {
				//     fprintf(stderr, "can't open %s\n", file_name);
				//     exit(1);
				// }
				// jpeg_stdio_dest(&cinfo, outfile);
				// std::vector<unsigned char> result_buff(w * h * chann);

#if JPEG_LIB_VERSION >= 90
				size_t result_size = 0;
#elif JPEG_LIB_VERSION >= 80
				unsigned long result_size = 0;
#else
				unsigned long result_size = 0;
#endif

				// unsigned char *first_prt = result_buff.data();
				// result_size = 4096;
				// unsigned char *initial_buffer = (unsigned char *) malloc(result_size);
				unsigned char *result_ptr = nullptr; // = initial_buffer;
				jpeg_mem_dest(&cinfo,
							  &result_ptr,
							  &result_size);

				/* Step 3: set parameters for compression */

				/* First we supply a description of the input image.
				 * Four fields of the cinfo struct must be filled in:
				 */
				cinfo.image_width = w; /* image width and height, in pixels */
				cinfo.image_height = h;
				cinfo.input_components = chann; /* # of color components per pixel */
				if (chann == 3)
					cinfo.in_color_space = JCS_RGB; /* colorspace of input image */
				else if (chann == 1)
					cinfo.in_color_space = JCS_GRAYSCALE;
				else
				{
					fprintf(stderr, "JPEG invalid number of channels: %i\n", chann);
					exit(1);
				}
				/* Now use the library's routine to set default compression parameters.
				 * (You must set at least cinfo.in_color_space before calling this,
				 * since the defaults depend on the source color space.)
				 */
				jpeg_set_defaults(&cinfo);
				/* Now you can set any non-default parameters you wish to.
				 * Here we just illustrate the use of quality (quantization table) scaling:
				 */
				jpeg_set_quality(&cinfo, quality, TRUE /* limit to baseline-JPEG values */);

				/* Step 4: Start compressor */

				/* TRUE ensures that we will write a complete interchange-JPEG file.
				 * Pass TRUE unless you are very sure of what you're doing.
				 */
				jpeg_start_compress(&cinfo, TRUE);

				/* Step 5: while (scan lines remain to be written) */
				/*           jpeg_write_scanlines(...); */

				/* Here we use the library's state variable cinfo.next_scanline as the
				 * loop counter, so that we don't have to keep track ourselves.
				 * To keep things simple, we pass one scanline per call; you can pass
				 * more if you wish, though.
				 */
				row_stride = w * chann; /* JSAMPLEs per row in image_buffer */

				while (cinfo.next_scanline < cinfo.image_height)
				{
					/* jpeg_write_scanlines expects an array of pointers to scanlines.
					 * Here the array is only one element long, but you could pass
					 * more than one scanline at a time if that's more convenient.
					 */
					if (invertY)
						row_pointer[0] = (JSAMPROW)&buffer[(h - 1 - cinfo.next_scanline) * row_stride];
					else
						row_pointer[0] = (JSAMPROW)&buffer[cinfo.next_scanline * row_stride];
					(void)jpeg_write_scanlines(&cinfo, row_pointer, 1);
				}

				/* Step 6: Finish compression */

				jpeg_finish_compress(&cinfo);
				/* After finish_compress, we can close the output file. */
				// fclose(outfile);

				char *result_final_buffer = (char *)ITKCommon::Memory::malloc(result_size);
				memcpy(result_final_buffer, result_ptr, result_size);
				*output_size = (int)result_size;

				/* Step 7: release JPEG compression object */

				/* This is an important step since it will release a good deal of memory. */
				jpeg_destroy_compress(&cinfo);

				free(result_ptr);

				/* And we're done! */
				return result_final_buffer;
			}

			void closeJPG(char *&buff)
			{
				if (!buff)
					return;
				ITKCommon::Memory::free(buff);
				// delete[]buff;
				buff = nullptr;
			}
			//----------------------------------------------------------------------------------

			bool isJPGFilename(const char *filename)
			{
				std::string file_lower = ITKCommon::StringUtil::toLower(filename);
				return ITKCommon::StringUtil::endsWith(file_lower, ".jpg") ||
					   ITKCommon::StringUtil::endsWith(file_lower, ".jpeg") ||
					   ITKCommon::StringUtil::endsWith(file_lower, ".jfif");
			}

		}
	}
}
