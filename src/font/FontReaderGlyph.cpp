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
        }
    }
}