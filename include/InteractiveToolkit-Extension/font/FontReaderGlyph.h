#pragma once

// #include <InteractiveToolkit/InteractiveToolkit.h>

// #include <InteractiveToolkit-Extension/io/AdvancedWriter.h>
// #include <InteractiveToolkit-Extension/io/AdvancedReader.h>

#include <stdint.h>

#include <InteractiveToolkit-Extension/font/FontReaderBitmapRef.h>
#include <InteractiveToolkit/AlgorithmCore/Polygon/Polygon2D.h>

namespace ITKExtension
{
    namespace IO
    {
        class AdvancedWriter;
        class AdvancedReader;
    }

    namespace Font
    {

        /// \brief Store a double glyph render information for reading
        ///
        /// The basof2 (Binary ASilva OpenGL Font, version 2.0) or<br />
        /// asbgt2 (ASilva Binary Glyph Table, version 2.0)<br />
        /// file format exports 2 glyphs information.
        ///
        /// By that way it is possible to render the font face in a normal<br />
        /// way and render the stroke of that same font.
        ///
        /// This class has 2 glyphs information: The Normal Face and the Stroke of the Face.
        ///
        /// \author Alessandro Ribeiro
        ///
        struct FontReaderGlyph
        {
            uint32_t charcode;          ///< UTF32 character code
            float advancex;             ///< column advance of this double glyph
            FontReaderBitmapRef face;   ///< the normal glyph face
            FontReaderBitmapRef stroke; ///< the stroke glyph of this face

            std::vector<AlgorithmCore::Polygon::Polygon2D<MathCore::vec2f>> contour; ///< The glyph contour polygon representation

            /// \brief Read this double glyph metrics from a #ITKExtension::IO::AdvancedReader
            ///
            /// Example:
            ///
            /// \code
            ///
            /// FontReaderGlyph doubleGlyph = ...;
            ///
            /// ...
            ///
            /// ITKExtension::IO::AdvancedReader reader;
            /// reader.readFromFile("file.input");
            ///
            /// doubleGlyph.read(&reader);
            ///
            /// reader.close();
            /// \endcode
            ///
            /// \author Alessandro Ribeiro
            /// \param reader The #ITKExtension::IO::AdvancedReader instance
            ///
            void read(ITKExtension::IO::AdvancedReader *reader);

        private:
            void readContour(ITKExtension::IO::AdvancedReader *reader);
        };

    }
}
