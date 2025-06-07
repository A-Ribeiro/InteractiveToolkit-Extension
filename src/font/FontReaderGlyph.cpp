// #include <InteractiveToolkit/ITKCommon/ITKCommon.h> // ITKAbort
#include <InteractiveToolkit-Extension/io/AdvancedReader.h>
// #include <InteractiveToolkit-Extension/io/AdvancedWriter.h>

#include <InteractiveToolkit-Extension/font/FontReaderGlyph.h>

namespace ITKExtension
{
    namespace Font
    {
        void FontReaderGlyph::read(ITKExtension::IO::AdvancedReader *reader)
        {
            charcode = reader->readUInt32();
            advancex = reader->readFloat();
            face.read(reader);
            stroke.read(reader);
            readContour(reader);
        }
        void FontReaderGlyph::readContour(ITKExtension::IO::AdvancedReader *reader)
        {
            uint32_t contourSize = reader->readUInt32();
            contour.resize(contourSize);
            for (uint32_t i = 0; i < contourSize; i++)
            {
                auto &poly_output = contour[i];
                poly_output.signedArea = reader->readFloat();
                uint32_t pointCount = reader->readUInt32();
                poly_output.points.resize(pointCount);
                for (uint32_t j = 0; j < pointCount; j++)
                {
                    auto &point_output = poly_output.points[j];
                    point_output.vertex = reader->read<MathCore::vec2f>();
                    point_output.pointType = (AlgorithmCore::Polygon::PointType)reader->readUInt8();
                }
            }
        }
    }
}