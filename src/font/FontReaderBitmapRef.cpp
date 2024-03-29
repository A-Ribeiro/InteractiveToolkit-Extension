// #include <InteractiveToolkit/ITKCommon/ITKCommon.h> // ITKAbort
#include <InteractiveToolkit-Extension/io/AdvancedReader.h>
// #include <InteractiveToolkit-Extension/io/AdvancedWriter.h>

#include <InteractiveToolkit-Extension/font/FontReaderBitmapRef.h>

namespace ITKExtension
{
    namespace Font
    {

        void FontReaderBitmapRef::read(ITKExtension::IO::AdvancedReader *reader)
        {
            top = reader->readInt16();
            left = reader->readInt16();
            bitmapBounds.read(reader);
        }

    }
}