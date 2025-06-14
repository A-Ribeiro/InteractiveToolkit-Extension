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

        void FontReader::clear()
        {
            glyphs.clear();
            if (bitmap_rgba != nullptr)
                ITKExtension::Image::PNG::closePNG(bitmap_rgba);
            bitmap_rgba = nullptr;
        }

        // void FontReader::readContour(ITKExtension::IO::AdvancedReader *reader)
        // {
        //     std::unordered_map<uint32_t, FontWriterGlyph *>::iterator it;
        //     for (it = glyphmap.begin(); it != glyphmap.end(); it++)
        //     {
        //         writer->writeUInt32(it->first); // charcode
        //         it->second->writeContour(writer);      // all glyph information
        //     }
        // }

        void FontReader::readGlyphTable(ITKExtension::IO::AdvancedReader *reader)
        {
            size = reader->readFloat();
            space_width = reader->readFloat();
            new_line_height = reader->readFloat();

            // can be used to double check the PNG decompression image resolution
            bitmapSize.read(reader);

            glyphs.resize((size_t)reader->readUInt16());
            for(auto& glyph: glyphs)
               glyph.read(reader);
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
            bitmap_rgba = ITKExtension::Image::PNG::readPNGFromMemory((char *)pngBuffer, pngBufferSize, &w, &h, &chann, &pixel_depth);

            ITK_ABORT(bitmap_rgba == nullptr, "Error to load image from font definition.\n");
            ITK_ABORT(w != bitmapSize.w, "Missmatch font resolution reference.\n");
            ITK_ABORT(h != bitmapSize.h, "Missmatch font resolution reference.\n");
            ITK_ABORT(chann != 4, "FontBitmap not rgba.\n");
            ITK_ABORT(pixel_depth != 8, "FontBitmap not 8 bits per component.\n");
        }

        FontReader::FontReader()
        {
            bitmap_rgba = nullptr;
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
            // readContour(&reader);

            reader.close();
        }

        void FontReader::readFromFile(const std::string &glyph, const std::string &png_rgba_8bits)
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
                bitmap_rgba = ITKExtension::Image::PNG::readPNG(png_rgba_8bits.c_str(), &w, &h, &chann, &pixel_depth);

                ITK_ABORT(bitmap_rgba == nullptr, "Error to load image from font definition.\n");
                ITK_ABORT(w != bitmapSize.w, "Missmatch font resolution reference.\n");
                ITK_ABORT(h != bitmapSize.h, "Missmatch font resolution reference.\n");
                ITK_ABORT(chann != 4, "FontBitmap not rgba.\n");
                ITK_ABORT(pixel_depth != 8, "FontBitmap not 8 bits per component.\n");
            }
        }

    }
}
