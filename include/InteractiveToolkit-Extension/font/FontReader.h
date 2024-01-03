#pragma once

#include <InteractiveToolkit/InteractiveToolkit.h>

#include <InteractiveToolkit-Extension/io/AdvancedWriter.h>
#include <InteractiveToolkit-Extension/io/AdvancedReader.h>

#include <InteractiveToolkit-Extension/font/FontReaderSize.h>
#include <InteractiveToolkit-Extension/font/FontReaderBitmapRef.h>
#include <InteractiveToolkit-Extension/font/FontReaderGlyph.h>

namespace ITKExtension
{
    namespace Font
    {

        /// \brief Store a pixelmap font definition.
        ///
        /// It is render system agnostic. Can be used in OpenGL, DirectX or Vulkan.
        ///
        /// This class reads all information needed to allow to render<br />
        /// a pixel map font.
        ///
        /// It has the glyphlist and the alpha channel bitmap, etc...
        ///
        /// The basof2 is a custom font format exporter created in this framework.<br />
        /// It means: Binary ASilva OpenGL Font, version 2.0
        ///
        /// It is not strictly applyable to OpenGL. You can use it in any 3D or 2D render application.
        ///
        /// Example:
        ///
        /// \code
        /// #include <aRibeiroCore/aRibeiroCore.h>
        /// #include <aRibeiroData/aRibeiroData.h>
        /// using namespace aRibeiro;
        /// using namespace aRibeiro;
        ///
        /// FontReader fontReader;
        ///
        /// fontReader.readFromFile( "font.basof2" );
        ///
        /// // can use the FontReader attributes
        /// ...
        /// \endcode
        ///
        /// \author Alessandro Ribeiro
        ///
        class FontReader
        {
            void clear();

            void readGlyphTable(ITKExtension::IO::AdvancedReader *reader);
            void readBitmap(ITKExtension::IO::AdvancedReader *reader);

            // private copy constructores, to avoid copy...
            FontReader(const FontReader &v);
            void operator=(const FontReader &v);

        public:
            float size;                            ///< The font matrix size of the loaded face.
            float space_width;                     ///< The x advance of a white space character.
            float new_line_height;                 ///< The height of a new line
            std::vector<FontReaderGlyph *> glyphs; ///< All glyphs loaded from this font.
            FontReaderSize bitmapSize;             ///< the bitmap resolution of the atlas
            char *bitmap;                          ///< The alpha channel bitmap

            FontReader();
            virtual ~FontReader();

            /// \brief Read the data saved with the #FontWriter class.
            ///
            /// The basof2 is a custom font format exporter created in this framework.<br />
            /// It means: Binary ASilva OpenGL Font, version 2.0
            ///
            /// Example:
            ///
            /// \code
            /// #include <aRibeiroCore/aRibeiroCore.h>
            /// #include <aRibeiroData/aRibeiroData.h>
            /// using namespace aRibeiro;
            ///
            /// FontReader fontReader;
            ///
            /// fontReader.readFromFile( "font.basof2" );
            ///
            /// // can use the FontReader attributes
            /// ...
            /// \endcode
            ///
            /// \author Alessandro Ribeiro
            /// \param filename basof2 filename to load
            ///
            void readFromFile(const std::string &filename);

            /// \brief Read the data saved with the #FontWriter class and a PNG grayscale image file.
            ///
            /// The asbgt2 is a custom font format exporter created in this framework.<br />
            /// It means: ASilva Binary Glyph Table, version 2.0
            ///
            /// Example:
            ///
            /// \code
            /// #include <aRibeiroCore/aRibeiroCore.h>
            /// #include <aRibeiroData/aRibeiroData.h>
            /// using namespace aRibeiro;
            ///
            /// FontReader fontReader;
            ///
            /// fontReader.readFromFile( "font.asbgt2", "font_atlas_gray.png" );
            ///
            /// // can use the FontReader attributes
            /// ...
            /// \endcode
            ///
            /// \author Alessandro Ribeiro
            /// \param glyph asbgt2 filename to load
            /// \param png_grayscale_8bits PNG image filename to load
            ///
            void readFromFile(const std::string &glyph, const std::string &png_grayscale_8bits);
        };

    }

}
