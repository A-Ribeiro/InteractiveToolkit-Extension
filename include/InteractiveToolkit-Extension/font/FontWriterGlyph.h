#pragma once

#include <InteractiveToolkit/InteractiveToolkit.h>

#include <InteractiveToolkit-Extension/io/AdvancedWriter.h>
#include <InteractiveToolkit-Extension/io/AdvancedReader.h>

#include <InteractiveToolkit-Extension/atlas/Atlas.h>

namespace ITKExtension
{
    namespace Font
    {

        /// \brief Store a double glyph render information for writing
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
        struct FontWriterGlyph
        {
            float advancex;       ///< normal face horizontal advance
            int16_t face_top;     ///< normal face glyph top reference
            int16_t face_left;    ///< normal face glyph left reference
            ITKExtension::Atlas::AtlasElement *face;   ///< atlas internal rectangle reference (normal face)
            int16_t stroke_top;   ///< stroke face glyph top reference
            int16_t stroke_left;  ///< stroke face glyph left reference
            ITKExtension::Atlas::AtlasElement *stroke; ///< atlas internal rectangle reference (stroke face)

            FontWriterGlyph();

            /// \brief Constructs a double glyph for writing reference.
            ///
            /// This class hold references to the #AtlasElement from an #Atlas.<br />
            /// It will not release the memory from this references.
            ///
            /// It is good to release the #Atlas after releasing this class.
            ///
            /// Example:
            ///
            /// \code
            /// #include <aRibeiroCore/aRibeiroCore.h>
            /// #include <aRibeiroData/aRibeiroData.h>
            /// using namespace aRibeiro;
            ///
            /// float advancex;
            /// uint16_t face_top, face_left;
            /// AtlasElement* atlasElementFace;
            /// uint16_t stroke_top, stroke_left;
            /// AtlasElement* atlasElementStroke
            ///
            /// FontWriterGlyph doubleGlyph = FontWriterGlyph(
            ///     advancex,
            ///     face_top, face_left, atlasElementFace,
            ///     stroke_top, stroke_left, atlasElementStroke
            /// );
            ///
            /// ...
            /// \endcode
            ///
            /// \author Alessandro Ribeiro
            /// \param _advancex Horizontal character advance
            /// \param _top Character face origin top
            /// \param _left Character face origin left
            /// \param _face Character normal face reference
            /// \param _stop Character stroke origin top
            /// \param _sleft Character stroke origin left
            /// \param _stroke Character stroke face reference
            ///
            FontWriterGlyph(float _advancex, 
                            int16_t _top, int16_t _left, 
                            ITKExtension::Atlas::AtlasElement *_face,
                            int16_t _stop, int16_t _sleft,
                            ITKExtension::Atlas::AtlasElement *_stroke);

            /// \brief Write this double glyph
            ///
            /// Example:
            ///
            /// \code
            /// #include <aRibeiroCore/aRibeiroCore.h>
            /// #include <aRibeiroData/aRibeiroData.h>
            /// using namespace aRibeiro;
            ///
            /// FontWriterGlyph doubleGlyph = ...;
            ///
            /// ...
            ///
            /// BinaryWriter writer;
            /// writer.writeToFile("file.output");
            ///
            /// doubleGlyph.write(&writer);
            ///
            /// writer.close();
            /// \endcode
            ///
            /// \author Alessandro Ribeiro
            /// \param writer The #aRibeiro::BinaryWriter instance
            ///
            void write(ITKExtension::IO::AdvancedWriter *writer);
        };

    }
}
