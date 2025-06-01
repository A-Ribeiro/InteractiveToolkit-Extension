// #include <InteractiveToolkit/ITKCommon/ITKCommon.h> // ITKAbort
// #include <InteractiveToolkit-Extension/io/AdvancedReader.h>
#include <InteractiveToolkit-Extension/io/AdvancedWriter.h>

#include <InteractiveToolkit-Extension/font/FontWriterGlyph.h>

namespace ITKExtension
{
    namespace Font
    {

        FontWriterGlyph::FontWriterGlyph()
        {
        }
        FontWriterGlyph::FontWriterGlyph(float _advancex,
                                         int16_t _top, int16_t _left,
                                         ITKExtension::Atlas::AtlasElement *_face,
                                         int16_t _stop, int16_t _sleft,
                                         ITKExtension::Atlas::AtlasElement *_stroke,
                                         const std::vector<AlgorithmCore::Polygon::Polygon2D<MathCore::vec2f>> &_contour)
        {
            advancex = _advancex;
            face_top = _top;
            face_left = _left;
            face = _face;
            stroke_top = _stop;
            stroke_left = _sleft;
            stroke = _stroke;
            contour = _contour;
        }

        void FontWriterGlyph::write(ITKExtension::IO::AdvancedWriter *writer)
        {
            writer->writeFloat(advancex);
            writer->writeInt16(face_top);
            writer->writeInt16(face_left);
            face->rect.write(writer);
            writer->writeInt16(stroke_top);
            writer->writeInt16(stroke_left);
            stroke->rect.write(writer);
        }

        void FontWriterGlyph::writeContour(ITKExtension::IO::AdvancedWriter *writer)
        {
            writer->writeUInt32((uint32_t)contour.size());
            for (const auto &poly : contour)
            {
                writer->writeFloat(poly.signedArea);
                writer->writeUInt32((uint32_t)poly.points.size());
                for (const auto &point : poly.points)
                {
                    writer->write(point.vertex);
                    writer->writeUInt8((uint8_t)point.pointType);
                }
            }
        }

    }
}
