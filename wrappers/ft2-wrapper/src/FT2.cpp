#include <ITKWrappers/FT2.h>

// #include <InteractiveToolkit/ITKCommon/ITKCommon.h> // ITKAbort
#include <InteractiveToolkit/ITKCommon/ITKAbort.h>

#define __STDC_CONSTANT_MACROS
#include <iostream>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_STROKER_H

#include <iostream>

using namespace std;
#define FT_CHECK_ERROR(_fnc, message) \
    ITK_ABORT((_fnc) != 0, message);

namespace ITKWrappers
{
    namespace FT2
    {

        FT2::FT2()
        {
            library = nullptr;
            face = nullptr;
            stroker = nullptr;
        }
        FT2::~FT2()
        {
            clear();
        }

        void FT2::clear()
        {
            if (stroker != nullptr)
                FT_Stroker_Done(stroker);
            if (face != nullptr)
                FT_Done_Face(face);
            if (library != nullptr)
                FT_Done_FreeType(library);
            stroker = nullptr;
            face = nullptr;
            library = nullptr;
        }

        void FT2::readFromFile_PX(const std::string &filename,
                                  FT_Int face_index,
                                  float outlineThickness,
                                  FT_UInt max_width, FT_UInt max_height)
        {
            clear();

            FT_CHECK_ERROR(FT_Init_FreeType(&library), "Erro do initialize FT2 lib");
            FT_CHECK_ERROR(FT_New_Face(library, filename.c_str(), face_index, &face), "Erro do load font file FT2 lib");
            FT_CHECK_ERROR(FT_Stroker_New(library, &stroker), "error to create font stroker");

            cout << "FT2 (PX mode)" << endl;
            cout << "File Opened: " << filename << endl;
            cout << "Faces: " << face->num_faces << endl;
            cout << "Selected face: " << face->face_index << endl;
            cout << "Family: " << face->family_name << endl;
            cout << "Style: " << face->style_name << endl;

            FT_CHECK_ERROR(FT_Set_Pixel_Sizes(face, max_width, max_height), "error to set the font size");
            FT_Stroker_Set(stroker,
                           (FT_Fixed)(outlineThickness * 64.0f + 0.5f),
                           FT_STROKER_LINECAP_ROUND, FT_STROKER_LINEJOIN_ROUND, 0);
        }

        ///    windows default DPI = 96
        ///    mac default DPI = 72
        ///    iphone4 - 960-by-640-pixel resolution at 326 ppi
        ///    iphone - 480 x 320 163 pixels per inch
        ///    ipod touch - 480 x 320 160 pixels per inch
        void FT2::readFromFile_DPI(const std::string &filename,
                                   FT_Int face_index,
                                   float outlineThickness,
                                   FT_UInt DPI, float fontPT)
        {
            clear();

            FT_CHECK_ERROR(FT_Init_FreeType(&library), "Erro do initialize FT2 lib");
            FT_CHECK_ERROR(FT_New_Face(library, filename.c_str(), face_index, &face), "Erro do load font file FT2 lib");
            FT_CHECK_ERROR(FT_Stroker_New(library, &stroker), "error to create font stroker");

            cout << "FT2 (DPI mode)" << endl;
            cout << "File Opened: " << filename << endl;
            cout << "Faces: " << face->num_faces << endl;
            cout << "Selected face: " << face->face_index << endl;
            cout << "Family: " << face->family_name << endl;
            cout << "Style: " << face->style_name << endl;

            FT_UInt ptInt = (FT_UInt)(fontPT * 64.0f + 0.5f);
            FT_CHECK_ERROR(FT_Set_Char_Size(face, 0, ptInt, DPI, DPI), "error to set the font size");

            FT_Stroker_Set(stroker,
                           (FT_Fixed)(outlineThickness * 64.0f + 0.5f),
                           FT_STROKER_LINECAP_ROUND, FT_STROKER_LINEJOIN_ROUND, 0);
        }

        FT2Glyph *FT2::generateGlyph(FT_ULong charcode_utf32)
        {
            FT_Int glyph_index = FT_Get_Char_Index(face, charcode_utf32);
            if (glyph_index == 0)
                return nullptr;

            FT2Glyph *result = new FT2Glyph();

            FT_CHECK_ERROR(FT_Load_Glyph(face, glyph_index, FT_LOAD_DEFAULT), "Error to load the character");
            FT_CHECK_ERROR(FT_Render_Glyph(face->glyph, FT_RENDER_MODE_NORMAL), "Error to render the character");

            result->charcode = charcode_utf32;
            result->advancex = ((float)face->glyph->advance.x) / 64.0f;
            result->advancey = ((float)face->glyph->advance.y) / 64.0f;

            result->normalRect = FT2Rect(face->glyph->bitmap_left,
                                         face->glyph->bitmap_top,
                                         face->glyph->bitmap.width,
                                         face->glyph->bitmap.rows);

            if (face->glyph->bitmap.width > 0 && face->glyph->bitmap.rows > 0)
                result->copyNormalBuffer(face->glyph->bitmap.buffer, false);

            FT_CHECK_ERROR(FT_Load_Glyph(face, glyph_index, FT_LOAD_NO_BITMAP | FT_LOAD_TARGET_NORMAL), "Error to load the character");

            FT_Glyph glyphDescStroke = nullptr;
            FT_CHECK_ERROR(FT_Get_Glyph(face->glyph, &glyphDescStroke), "error to get stroke glyph");

            FT_CHECK_ERROR(FT_Glyph_Stroke(&glyphDescStroke, stroker, true), "error to stroke");
            FT_CHECK_ERROR(FT_Glyph_To_Bitmap(&glyphDescStroke, FT_RENDER_MODE_NORMAL, 0, true), "error to get bitmap");

            FT_BitmapGlyph glyph_bitmap = (FT_BitmapGlyph)glyphDescStroke;

            result->strokeRect = FT2Rect(glyph_bitmap->left,
                                         glyph_bitmap->top,
                                         glyph_bitmap->bitmap.width,
                                         glyph_bitmap->bitmap.rows);

            if (glyph_bitmap->bitmap.width > 0 && glyph_bitmap->bitmap.rows > 0)
                result->copyStrokeBuffer(glyph_bitmap->bitmap.buffer, false);

            FT_Done_Glyph(glyphDescStroke);

            // get countour polygon
            FT_CHECK_ERROR(FT_Load_Glyph(face, glyph_index, FT_LOAD_NO_BITMAP | FT_LOAD_NO_HINTING), "Error to load the outline character");

            if (face->glyph->format == FT_GLYPH_FORMAT_OUTLINE)
            {
                FT_Outline *outline = &face->glyph->outline;
                for (int c = 0; c < outline->n_contours; c++)
                {
                    int start = (c == 0) ? 0 : outline->contours[c - 1] + 1;
                    int end = outline->contours[c] + 1;
                    AlgorithmCore::Polygon::Polygon2D<MathCore::vec2f> polygon;
                    char last_tag = -1;
                    int cubic_count = 0;
                    for (int i = start; i < end; i++)
                    {
                        FT_Vector &point = outline->points[i];
                        char tag = FT_CURVE_TAG(outline->tags[i]);
                        MathCore::vec2f vertex((float)point.x / 64.0f, (float)point.y / 64.0f);
                        if (tag == FT_CURVE_TAG_ON) {
                            polygon.addPoint(vertex);
                            cubic_count = 0;
                        } else if (tag == FT_CURVE_TAG_CONIC) {
                            if (last_tag != FT_CURVE_TAG_ON) {
                                // printf("Warning: Conic point without previous ON point in glyph 0x%.8x\n", (uint32_t)charcode_utf32);
                                // two consecutive conic points, add a midpoint
                                MathCore::vec2f mid_point = (polygon.points.back().vertex + vertex) * 0.5f;
                                polygon.addPoint(mid_point);
                                // cubic_count = 0;
                            }
                            polygon.addControlPoint(vertex);
                            cubic_count = 2; // reset cubic count to 2 for conic
                        }
                        else if (tag == FT_CURVE_TAG_CUBIC) {
                            if (cubic_count == 2)
                            {
                                printf("Warning: Cubic point after two consecutive cubic points in glyph 0x%.8x\n", (uint32_t)charcode_utf32);
                                // three consecutive cubic points, or cubic after conic, add a midpoint
                                MathCore::vec2f mid_point = (polygon.points.back().vertex + vertex) * 0.5f;
                                polygon.addPoint(mid_point);
                                cubic_count = 0; // reset cubic count
                            }
                            polygon.addControlPoint(vertex);
                            cubic_count++;
                        }
                        last_tag = tag;
                    }
                    polygon.computeSignedArea();
                    // reverse the polygon to have the correct orientation:
                    // outline -> CCW -> positive area
                    // hole -> CW -> negative area
                    polygon.reverse();
                    result->contour.push_back(std::move(polygon));
                }
            }

            return result;
        }

        void FT2::releaseGlyph(FT2Glyph **glyph)
        {
            delete *glyph;
            *glyph = nullptr;
        }

        float FT2::getNewLineHeight(bool use_max_bouding_box)
        {
            if (use_max_bouding_box)
            {
                int bbox_ymax = FT_MulFix(face->bbox.yMax, face->size->metrics.y_scale);
                int bbox_ymin = FT_MulFix(face->bbox.yMin, face->size->metrics.y_scale);
                int height = bbox_ymax - bbox_ymin;
                return (float)height / 64.0f;
            }
            return (float)(face->size->metrics.height) / 64.0f;
        }
    }
}
