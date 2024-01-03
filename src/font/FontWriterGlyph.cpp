#include <InteractiveToolkit-Extension/font/FontWriterGlyph.h>

namespace ITKExtension
{
    namespace Font
    {

        FontWriterGlyph::FontWriterGlyph()
        {
        }
        FontWriterGlyph::FontWriterGlyph(float _advancex, 
                                         int16_t _top, int16_t _left, 
                                         ITKExtension::Atlas::AtlasElement *_face,
                                         int16_t _stop, int16_t _sleft,
                                         ITKExtension::Atlas::AtlasElement *_stroke)
        {
            advancex = _advancex;
            face_top = _top;
            face_left = _left;
            face = _face;
            stroke_top = _stop;
            stroke_left = _sleft;
            stroke = _stroke;
        }

        void FontWriterGlyph::write(ITKExtension::IO::AdvancedWriter *writer)
        {
            writer->writeFloat(advancex);
            writer->writeInt16(face_top);
            writer->writeInt16(face_left);
            face->rect.write(writer);
            writer->writeInt16(stroke_top);
            writer->writeInt16(stroke_left);
            stroke->rect.write(writer);
        }
    }
}
