#pragma once

// #include <InteractiveToolkit/InteractiveToolkit.h>

// #include <InteractiveToolkit-Extension/io/AdvancedWriter.h>
// #include <InteractiveToolkit-Extension/io/AdvancedReader.h>

#include <stdint.h>

#include <InteractiveToolkit-Extension/atlas/AtlasRect.h>

namespace ITKExtension
{
    namespace IO
    {
        class AdvancedWriter;
        class AdvancedReader;
    }

    namespace Font
    {

        /// \brief Store a glyph render information
        ///
        /// Each glyph needs a starting point reference (top,left) and <br />
        /// the sprite rectangle inside the atlas (bitmapBounds).
        ///
        /// With this information you can render non-monospace character types.
        ///
        /// \author Alessandro Ribeiro
        ///
        struct FontReaderBitmapRef
        {
            int16_t top;                                 ///< glyph origin top (y coord).
            int16_t left;                                ///< glyph origin left (x coord).
            ITKExtension::Atlas::AtlasRect bitmapBounds; ///< sprite rectangle inside the atlas.

            /// \brief Read this glyph metrics from a #ITKExtension::IO::AdvancedReader
            ///
            /// Example:
            ///
            /// \code
            ///
            /// FontReaderBitmapRef bitmapRef = ...;
            ///
            /// ...
            ///
            /// BinaryReader reader;
            /// reader.readFromFile("file.input");
            ///
            /// bitmapRef.read(&reader);
            ///
            /// reader.close();
            /// \endcode
            ///
            /// \author Alessandro Ribeiro
            /// \param reader The #ITKExtension::IO::AdvancedReader instance
            ///
            void read(ITKExtension::IO::AdvancedReader *reader);
        };

    }

}