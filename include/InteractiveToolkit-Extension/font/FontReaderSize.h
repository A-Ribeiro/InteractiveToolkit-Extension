#pragma once

// #include <InteractiveToolkit/InteractiveToolkit.h>

// #include <InteractiveToolkit-Extension/io/AdvancedWriter.h>
// #include <InteractiveToolkit-Extension/io/AdvancedReader.h>

#include <stdint.h>

namespace ITKExtension
{
    namespace IO
    {
        class AdvancedWriter;
        class AdvancedReader;
    }

    namespace Font
    {

        /// \brief Store a size information (width, height)
        ///
        /// \author Alessandro Ribeiro
        ///
        struct FontReaderSize
        {
            uint32_t w; ///< width
            uint32_t h; ///< height

            /// \brief Read the size information from a #ITKExtension::IO::AdvancedReader
            ///
            /// Example:
            ///
            /// \code
            ///
            /// FontReaderSize size = ...;
            ///
            /// ...
            ///
            /// ITKExtension::IO::AdvancedReader reader;
            /// reader.readFromFile("file.input");
            ///
            /// size.read(&reader);
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
