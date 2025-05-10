#pragma once

// #include <InteractiveToolkit/InteractiveToolkit.h>

// #include <InteractiveToolkit-Extension/io/AdvancedWriter.h>
// #include <InteractiveToolkit-Extension/io/AdvancedReader.h>

#include <stdint.h>
#include <map>
#include <unordered_map>

#include <InteractiveToolkit-Extension/atlas/Atlas.h>
#include <InteractiveToolkit-Extension/font/FontWriterGlyph.h>

namespace ITKExtension
{
    namespace IO
    {
        class AdvancedWriter;
        class AdvancedReader;
    }

    namespace Font
    {

        /// \brief Save a font character set using the basof2 or asbgt2 format.
        ///
        /// The basof2 (Binary ASilva OpenGL Font, version 2.0) or<br />
        /// asbgt2 (ASilva Binary Glyph Table, version 2.0)<br />
        /// file format exports 2 glyphs information (normal and stroke).
        ///
        /// This class handle the correct writing pattern of glyph table and pixelmap (grayscale).
        ///
        /// Example:
        ///
        /// \code
        /// #include <aRibeiroCore/aRibeiroCore.h>
        /// #include <aRibeiroData/aRibeiroData.h>
        /// using namespace aRibeiro;
        /// using namespace aRibeiro;
        ///
        /// Atlas atlas = ...;
        ///
        /// ...
        ///
        /// FontWriter fontWriter;
        ///
        /// float characterSize;
        /// float spaceWidth;
        /// float newLineHeight;
        ///
        /// fontWriter.initFromAtlas(&atlas, characterSize, spaceWidth, newLineHeight);
        ///
        /// for (int i=0;i<utf32data->count();i++){
        ///     AtlasElement* atlasElementFace = ...
        ///     AtlasElement* atlasElementStroke = ...
        ///
        ///     fontWriter.setCharacter(...);
        /// }
        ///
        /// atlas.organizePositions(false);
        ///
        /// fontWriter.save( "file.basof2" );
        /// fontWriter.saveGlyphTable( "file.asbgt2" );
        ///
        /// \endcode
        ///
        /// \author Alessandro Ribeiro
        ///
        class FontWriter
        {

        public:

            //deleted copy constructor and assign operator, to avoid copy...
            FontWriter(const FontWriter &v) = delete;
            FontWriter& operator=(const FontWriter &v) = delete;

            ITKExtension::Atlas::Atlas *atlas; ///< reference to sprite #Atlas of this font

            std::unordered_map<uint32_t, FontWriterGlyph *> glyphmap; ///< the glyphmap (char code to FontWriterGlyph mapping)

            float space_width;        ///< white space horizontal increment
            float new_line_height;    ///< new line height
            float max_square_size_px; ///< font reference matrix size

            FontWriter();

            ~FontWriter();

            /// \brief Setup this FontWriter to work with an #Atlas instance
            ///
            /// The font writer manages the mapping of a character to a sprite inside this atlas.
            ///
            /// Example:
            ///
            /// \code
            /// #include <aRibeiroCore/aRibeiroCore.h>
            /// #include <aRibeiroData/aRibeiroData.h>
            /// using namespace aRibeiro;
            ///
            /// Atlas atlas = ...;
            ///
            /// ...
            ///
            /// FontWriter fontWriter;
            ///
            /// float characterSize;
            /// float spaceWidth;
            /// float newLineHeight;
            ///
            /// fontWriter.initFromAtlas(&atlas, characterSize, spaceWidth, newLineHeight);
            ///
            /// ...
            /// \endcode
            ///
            /// \author Alessandro Ribeiro
            ///
            void initFromAtlas(ITKExtension::Atlas::Atlas *_atlas, float _max_square_size_px, float _space_width, float _new_line_height);

            /// \brief Setup one character inside this font character set.
            ///
            /// Cannot insert the same character twice.
            ///
            /// Example:
            ///
            /// \code
            /// #include <aRibeiroCore/aRibeiroCore.h>
            /// #include <aRibeiroData/aRibeiroData.h>
            /// using namespace aRibeiro;
            ///
            /// ...
            ///
            /// FontWriter fontWriter;
            ///
            /// ...
            ///
            /// for (int i=0;i<utf32data->count();i++){
            ///     ...
            ///     AtlasElement* atlasElementFace = ...
            ///     AtlasElement* atlasElementStroke = ...
            ///
            ///     fontWriter.setCharacter(
            ///         char_code[i],
            ///         advancex,
            ///         face_top,face_left,atlasElementFace,
            ///         stroke_top,stroke_left,atlasElementStroke
            ///     );
            /// }
            ///
            /// ...
            ///
            /// \endcode
            ///
            /// \author Alessandro Ribeiro
            /// \param charcode UTF32 character code
            /// \param advancex Horizontal character advance
            /// \param ftop Character face origin top
            /// \param fleft Character face origin left
            /// \param atlasElementFace Character normal face reference
            /// \param stop Character stroke origin top
            /// \param sleft Character stroke origin left
            /// \param atlasElementStroke Character stroke face reference
            ///
            void setCharacter(uint32_t charcode,
                              float advancex,
                              int16_t ftop,
                              int16_t fleft,
                              ITKExtension::Atlas::AtlasElement *atlasElementFace,
                              int16_t stop,
                              int16_t sleft,
                              ITKExtension::Atlas::AtlasElement *atlasElementStroke);

            /// \brief Save basof2 file
            ///
            /// The basof2 (Binary ASilva OpenGL Font, version 2.0) or<br />
            /// asbgt2 (ASilva Binary Glyph Table, version 2.0)<br />
            /// file format exports 2 glyphs information (normal and stroke).
            ///
            /// Example:
            ///
            /// \code
            /// #include <aRibeiroCore/aRibeiroCore.h>
            /// #include <aRibeiroData/aRibeiroData.h>
            /// using namespace aRibeiro;
            ///
            /// ...
            ///
            /// FontWriter fontWriter;
            ///
            /// ...
            ///
            /// fontWriter.save( "file.basof2" );
            ///
            /// \endcode
            ///
            /// \author Alessandro Ribeiro
            /// \param filename basof2 filename to save
            ///
            void save(const char *filename);

            /// \brief Save asbgt2 file
            ///
            /// The basof2 (Binary ASilva OpenGL Font, version 2.0) or<br />
            /// asbgt2 (ASilva Binary Glyph Table, version 2.0)<br />
            /// file format exports 2 glyphs information (normal and stroke).
            ///
            /// Example:
            ///
            /// \code
            /// #include <aRibeiroCore/aRibeiroCore.h>
            /// #include <aRibeiroData/aRibeiroData.h>
            /// using namespace aRibeiro;
            ///
            /// ...
            ///
            /// FontWriter fontWriter;
            ///
            /// ...
            ///
            /// fontWriter.saveGlyphTable( "file.asbgt2" );
            ///
            /// \endcode
            ///
            /// \author Alessandro Ribeiro
            /// \param filename basof2 filename to save
            ///
            void saveGlyphTable(const char *filename);

            /// \brief Write the UTF32 character, double glyph map table
            ///
            /// Example:
            ///
            /// \code
            /// #include <aRibeiroCore/aRibeiroCore.h>
            /// #include <aRibeiroData/aRibeiroData.h>
            /// using namespace aRibeiro;
            ///
            /// ...
            ///
            /// FontWriter fontWriter;
            ///
            /// ...
            ///
            /// BinaryWriter writer;
            /// writer.writeToFile("file.output");
            ///
            /// fontWriter.writeGlyphTable(&writer);
            ///
            /// writer.close();
            /// \endcode
            ///
            /// \author Alessandro Ribeiro
            /// \param writer The #aRibeiro::BinaryWriter instance
            ///
            void writeGlyphTable(ITKExtension::IO::AdvancedWriter *writer);

            /// \brief Write the bitmap compressed using PNG format
            ///
            /// Example:
            ///
            /// \code
            /// #include <aRibeiroCore/aRibeiroCore.h>
            /// #include <aRibeiroData/aRibeiroData.h>
            /// using namespace aRibeiro;
            ///
            /// ...
            ///
            /// FontWriter fontWriter;
            ///
            /// ...
            ///
            /// BinaryWriter writer;
            /// writer.writeToFile("file.output");
            ///
            /// fontWriter.writeBitmap(&writer);
            ///
            /// writer.close();
            /// \endcode
            ///
            /// \author Alessandro Ribeiro
            /// \param writer The #aRibeiro::BinaryWriter instance
            ///
            void writeBitmap(ITKExtension::IO::AdvancedWriter *writer);
        };

    }
}
