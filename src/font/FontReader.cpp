// #include <InteractiveToolkit/ITKCommon/ITKCommon.h> // ITKAbort
#include <InteractiveToolkit/ITKCommon/ITKAbort.h>

#include <InteractiveToolkit-Extension/io/AdvancedReader.h>
// #include <InteractiveToolkit-Extension/io/AdvancedWriter.h>

#include <InteractiveToolkit-Extension/font/FontReader.h>
#include <InteractiveToolkit-Extension/image/PNG.h>
#include <InteractiveToolkit-Extension/image/JPG.h>

namespace ITKExtension
{
    namespace Font
    {

        // private copy constructores, to avoid copy...
        FontReader::FontReader(const FontReader &v) {}
        void FontReader::operator=(const FontReader &v) {}

        void FontReader::clear()
        {
            for (size_t i = 0; i < glyphs.size(); i++)
                delete glyphs[i];
            glyphs.clear();
            if (bitmap != NULL)
                ITKExtension::Image::PNG::closePNG(bitmap);
            bitmap = NULL;
        }

        void FontReader::readGlyphTable(ITKExtension::IO::AdvancedReader *reader)
        {
            size = reader->readFloat();
            space_width = reader->readFloat();
            new_line_height = reader->readFloat();

            // can be used to double check the PNG decompression image resolution
            bitmapSize.read(reader);

            uint16_t glyphCount = reader->readUInt16();
            for (uint16_t i = 0; i < glyphCount; i++)
            {
                FontReaderGlyph *glyph = new FontReaderGlyph();
                glyph->read(reader);
                glyphs.push_back(glyph);
            }
        }

        void FontReader::readBitmap(ITKExtension::IO::AdvancedReader *reader)
        {
            uint8_t *pngBuffer;
            uint32_t pngBufferSize;

            Platform::ObjectBuffer buffer;
            reader->readBuffer(&buffer);
            pngBuffer = buffer.data;
            pngBufferSize = (uint32_t)buffer.size;

            int w, h, chann, pixel_depth;
            bitmap = ITKExtension::Image::PNG::readPNGFromMemory((char *)pngBuffer, pngBufferSize, &w, &h, &chann, &pixel_depth);

            ITK_ABORT(bitmap == NULL, "Error to load image from font definition.\n");
            ITK_ABORT(w != bitmapSize.w, "Missmatch font resolution reference.\n");
            ITK_ABORT(h != bitmapSize.h, "Missmatch font resolution reference.\n");
            ITK_ABORT(chann != 1, "FontBitmap not grayscale.\n");
            ITK_ABORT(pixel_depth != 8, "FontBitmap not 8 bits per component.\n");
        }

        FontReader::FontReader()
        {
            bitmap = NULL;
        }

        FontReader::~FontReader()
        {
            clear();
        }

        void FontReader::readFromFile(const std::string &filename)
        {
            clear();

            ITKExtension::IO::AdvancedReader reader;
            reader.readFromFile(filename.c_str());

            readGlyphTable(&reader);
            readBitmap(&reader);

            reader.close();
        }

        void FontReader::readFromFile(const std::string &glyph, const std::string &png_grayscale_8bits)
        {
            clear();

            {
                ITKExtension::IO::AdvancedReader reader;
                reader.readFromFile(glyph.c_str());
                readGlyphTable(&reader);
                reader.close();
            }

            {
                /*
                BinaryReader reader;
                reader.readFromFile(png_grayscale_8bits.c_str(), false);
                readBitmap(&reader);
                reader.close();
                */
                int w, h, chann, pixel_depth;
                bitmap = ITKExtension::Image::PNG::readPNG(png_grayscale_8bits.c_str(), &w, &h, &chann, &pixel_depth);

                ITK_ABORT(bitmap == NULL, "Error to load image from font definition.\n");
                ITK_ABORT(w != bitmapSize.w, "Missmatch font resolution reference.\n");
                ITK_ABORT(h != bitmapSize.h, "Missmatch font resolution reference.\n");
                ITK_ABORT(chann != 1, "FontBitmap not grayscale.\n");
                ITK_ABORT(pixel_depth != 8, "FontBitmap not 8 bits per component.\n");
            }
        }

    }
}
