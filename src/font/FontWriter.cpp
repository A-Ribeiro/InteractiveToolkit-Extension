// #include <InteractiveToolkit/ITKCommon/ITKCommon.h> // ITKAbort
#include <InteractiveToolkit/ITKCommon/ITKAbort.h>

// #include <InteractiveToolkit-Extension/io/AdvancedReader.h>
#include <InteractiveToolkit-Extension/io/AdvancedWriter.h>

#include <InteractiveToolkit-Extension/font/FontWriter.h>
#include <InteractiveToolkit-Extension/image/PNG.h>
#include <InteractiveToolkit-Extension/image/JPG.h>

namespace ITKExtension
{
    namespace Font
    {

        FontWriter::FontWriter()
        {
        }

        FontWriter::~FontWriter()
        {
            std::unordered_map<uint32_t, FontWriterGlyph *>::iterator it;
            for (it = glyphmap.begin(); it != glyphmap.end(); it++)
                delete it->second;
            glyphmap.clear();
        }

        void FontWriter::initFromAtlas(ITKExtension::Atlas::Atlas *_atlas, float _max_square_size_px, float _space_width, float _new_line_height)
        {
            atlas = _atlas;
            max_square_size_px = _max_square_size_px;
            new_line_height = _new_line_height;
            space_width = _space_width;
        }

        void FontWriter::setCharacter(uint32_t charcode,
                                      float advancex,
                                      int16_t ftop,
                                      int16_t fleft,
                                      ITKExtension::Atlas::AtlasElement *atlasElementFace,
                                      int16_t stop,
                                      int16_t sleft,
                                      ITKExtension::Atlas::AtlasElement *atlasElementStroke,
                                      const std::vector<AlgorithmCore::Polygon::Polygon2D<MathCore::vec2f>> &contour)
        {

            ITK_ABORT((glyphmap.find(charcode) != glyphmap.end()), "Trying to insert the same character twice.\n");
            // if (glyphmap.find(charcode) != glyphmap.end()) {
            //     fprintf(stderr, "FontWriter - Error: Trying to insert the same character twice.\n");
            //     exit(-1);
            // }

            glyphmap[charcode] = new FontWriterGlyph(
                advancex, ftop, fleft, atlasElementFace,
                stop, sleft, atlasElementStroke,
                contour);
        }

        void FontWriter::save(const char *filename)
        {
            ITKExtension::IO::AdvancedWriter writer;
            // writer.writeToFile(filename);
            writeGlyphTable(&writer);
            writeBitmap(&writer);

            writeContour(&writer);
            // writer.close();

            writer.writeToFile(filename);
        }

        void FontWriter::writeContour(ITKExtension::IO::AdvancedWriter *writer)
        {
            std::unordered_map<uint32_t, FontWriterGlyph *>::iterator it;
            for (it = glyphmap.begin(); it != glyphmap.end(); it++)
            {
                writer->writeUInt32(it->first); // charcode
                it->second->writeContour(writer);      // all glyph information
            }
        }

        void FontWriter::writeGlyphTable(ITKExtension::IO::AdvancedWriter *writer)
        {
            writer->writeFloat(max_square_size_px);
            writer->writeFloat(space_width);
            writer->writeFloat(new_line_height);

            writer->writeUInt32(atlas->textureResolution.w);
            writer->writeUInt32(atlas->textureResolution.h);

            writer->writeUInt16((uint16_t)glyphmap.size());
            std::unordered_map<uint32_t, FontWriterGlyph *>::iterator it;
            for (it = glyphmap.begin(); it != glyphmap.end(); it++)
            {
                writer->writeUInt32(it->first); // charcode
                it->second->write(writer);      // all glyph information
            }
        }
        void FontWriter::writeBitmap(ITKExtension::IO::AdvancedWriter *writer)
        {
            uint8_t *rgbaBuffer = atlas->createRGBA();
            int size;
            char *result = ITKExtension::Image::PNG::writePNGToMemory(&size, atlas->textureResolution.w, atlas->textureResolution.h, 4, (char *)rgbaBuffer);

            ITK_ABORT(result == nullptr, "Error to write PNG to memory.\n");

            atlas->releaseRGBA(&rgbaBuffer);

            writer->writeBuffer(
                Platform::ObjectBuffer((uint8_t *)result, (int64_t)size));

            ITKExtension::Image::PNG::closePNG(result);
        }

        void FontWriter::saveGlyphTable(const char *filename)
        {
            ITKExtension::IO::AdvancedWriter writer;
            // writer.writeToFile(filename);
            writeGlyphTable(&writer);
            // writer.close();

            writer.writeToFile(filename);
        }

    }
}