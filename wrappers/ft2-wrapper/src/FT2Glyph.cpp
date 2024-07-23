#include <ITKWrappers/FT2Glyph.h>

// #include <InteractiveToolkit/ITKCommon/ITKCommon.h> // ITKAbort
#include <InteractiveToolkit/ITKCommon/ITKAbort.h>

#include <InteractiveToolkit-Extension/image/PNG.h> // PNGHelper

namespace ITKWrappers
{
    namespace FT2
    {

        // private copy constructores, to avoid copy...
        FT2Glyph::FT2Glyph(const FT2Glyph &v) {}
        void FT2Glyph::operator=(const FT2Glyph &v) {}

        void FT2Glyph::copyNormalBuffer(uint8_t *src, bool invert)
        {

            ITK_ABORT(
                (normalRect.w == 0 || normalRect.h == 0),
                "error: normalRect not set\n");
            // if (normalRect.w==0||normalRect.h==0){
            //     fprintf(stderr,"error: normalRect not set\n");
            //     exit(-1);
            // }

            if (invert)
                normalRect.y = normalRect.h - normalRect.y - 1;

            if (bitmapGrayNormal != nullptr)
                delete[] bitmapGrayNormal;

            bitmapGrayNormal = new uint8_t[normalRect.w * normalRect.h];

            if (invert)
            {
                for (int i = 0; i < normalRect.h; i++)
                    memcpy(bitmapGrayNormal + i * normalRect.w,
                           src + (normalRect.h - 1 - i) * normalRect.w,
                           normalRect.w * sizeof(uint8_t));
            }
            else
                memcpy(bitmapGrayNormal, src, normalRect.w * normalRect.h);

            // generate RGBA
            if (bitmapRGBANormal != nullptr)
                delete[] bitmapRGBANormal;
            bitmapRGBANormal = new uint8_t[normalRect.w * normalRect.h * 4];
            for (int y = 0; y < normalRect.h; y++)
            {
                for (int x = 0; x < normalRect.w; x++)
                {
                    bitmapRGBANormal[(x + y * normalRect.w) * 4 + 0] = 255;
                    bitmapRGBANormal[(x + y * normalRect.w) * 4 + 1] = 255;
                    bitmapRGBANormal[(x + y * normalRect.w) * 4 + 2] = 255;
                    bitmapRGBANormal[(x + y * normalRect.w) * 4 + 3] = bitmapGrayNormal[x + y * normalRect.w];
                }
            }
        }

        void FT2Glyph::copyStrokeBuffer(uint8_t *src, bool invert)
        {

            ITK_ABORT(
                (strokeRect.w == 0 || strokeRect.h == 0),
                "error: strokeRect not set\n");
            // if (strokeRect.w==0||strokeRect.h==0){
            //     fprintf(stderr,"error: strokeRect not set\n");
            //     exit(-1);
            // }

            if (invert)
                strokeRect.y = strokeRect.h - strokeRect.y - 1;

            if (bitmapGrayStroke != nullptr)
                delete[] bitmapGrayStroke;

            bitmapGrayStroke = new uint8_t[strokeRect.w * strokeRect.h];

            if (invert)
            {
                for (int i = 0; i < strokeRect.h; i++)
                    memcpy(bitmapGrayStroke + i * strokeRect.w,
                           src + (strokeRect.h - 1 - i) * strokeRect.w,
                           strokeRect.w * sizeof(uint8_t));
            }
            else
                memcpy(bitmapGrayStroke, src, strokeRect.w * strokeRect.h);

            // generate RGBA
            if (bitmapRGBAStroke != nullptr)
                delete[] bitmapRGBAStroke;
            bitmapRGBAStroke = new uint8_t[strokeRect.w * strokeRect.h * 4];
            for (int y = 0; y < strokeRect.h; y++)
            {
                for (int x = 0; x < strokeRect.w; x++)
                {
                    bitmapRGBAStroke[(x + y * strokeRect.w) * 4 + 0] = 255;
                    bitmapRGBAStroke[(x + y * strokeRect.w) * 4 + 1] = 255;
                    bitmapRGBAStroke[(x + y * strokeRect.w) * 4 + 2] = 255;
                    bitmapRGBAStroke[(x + y * strokeRect.w) * 4 + 3] = bitmapGrayStroke[x + y * strokeRect.w];
                }
            }
        }

        void FT2Glyph::saveNormalGrayPNG(const std::string &filename)
        {
            ITKExtension::Image::PNG::writePNG(filename.c_str(), normalRect.w, normalRect.h, 1, (char *)bitmapGrayNormal);
        }

        void FT2Glyph::saveStrokeGrayPNG(const std::string &filename)
        {
            ITKExtension::Image::PNG::writePNG(filename.c_str(), strokeRect.w, strokeRect.h, 1, (char *)bitmapGrayStroke);
        }

        void FT2Glyph::saveNormalRGBAPNG(const std::string &filename)
        {
            ITKExtension::Image::PNG::writePNG(filename.c_str(), normalRect.w, normalRect.h, 4, (char *)bitmapRGBANormal);
        }

        void FT2Glyph::saveStrokeRGBAPNG(const std::string &filename)
        {
            ITKExtension::Image::PNG::writePNG(filename.c_str(), strokeRect.w, strokeRect.h, 4, (char *)bitmapRGBAStroke);
        }

        FT2Glyph::FT2Glyph()
        {
            bitmapGrayNormal = nullptr;
            bitmapGrayStroke = nullptr;
            bitmapRGBANormal = nullptr;
            bitmapRGBAStroke = nullptr;
        }

        FT2Glyph::~FT2Glyph()
        {
            if (bitmapGrayNormal != nullptr)
                delete[] bitmapGrayNormal;
            if (bitmapGrayStroke != nullptr)
                delete[] bitmapGrayStroke;
            if (bitmapRGBANormal != nullptr)
                delete[] bitmapRGBANormal;
            if (bitmapRGBAStroke != nullptr)
                delete[] bitmapRGBAStroke;

            bitmapGrayNormal = nullptr;
            bitmapGrayStroke = nullptr;
            bitmapRGBANormal = nullptr;
            bitmapRGBAStroke = nullptr;
        }
    }
}
