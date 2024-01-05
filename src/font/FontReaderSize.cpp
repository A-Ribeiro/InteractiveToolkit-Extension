// #include <InteractiveToolkit/ITKCommon/ITKCommon.h> // ITKAbort
#include <InteractiveToolkit-Extension/io/AdvancedReader.h>
// #include <InteractiveToolkit-Extension/io/AdvancedWriter.h>

#include <InteractiveToolkit-Extension/font/FontReaderSize.h>

namespace ITKExtension
{
    namespace Font
    {
        void FontReaderSize::read(ITKExtension::IO::AdvancedReader *reader)
        {
            w = reader->readUInt32();
            h = reader->readUInt32();
        }
    }
}
