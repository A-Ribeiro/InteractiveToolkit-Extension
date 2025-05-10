// #include <InteractiveToolkit/ITKCommon/ITKCommon.h> // ITKAbort
#include <InteractiveToolkit/ITKCommon/ITKAbort.h>

#include <InteractiveToolkit-Extension/io/AdvancedReader.h>
#include <InteractiveToolkit-Extension/io/AdvancedWriter.h>

#include <InteractiveToolkit-Extension/atlas/AtlasElement.h>

#include <InteractiveToolkit-Extension/image/PNG.h>
#include <InteractiveToolkit-Extension/image/JPG.h>

namespace ITKExtension
{
    namespace Atlas
    {

        AtlasElement::AtlasElement(int w, int h)
        {

            ITK_ABORT((w < 0 || h < 0), "error assign Atlas Element\n");
            // if (w<=0||h<=0){
            //     fprintf(stderr,"error assign Atlas Element\n");
            //     exit(-1);
            // }

            rect = AtlasRect(w, h);
            buffer = new uint8_t[w * h * 4];
        }

        AtlasElement::~AtlasElement()
        {
            if (buffer != nullptr)
                delete[] buffer;
        }

        AtlasElement::AtlasElement()
        {
            buffer = nullptr;
        }

        void AtlasElement::read(ITKExtension::IO::AdvancedReader *reader)
        {
            if (buffer != nullptr)
            {
                delete[] buffer;
                buffer = nullptr;
            }
            name = reader->readString();
            rect.read(reader);
        }

        void AtlasElement::write(ITKExtension::IO::AdvancedWriter *writer) const
        {
            writer->writeString(name);
            rect.write(writer);
        }

        void AtlasElement::copyFromRGBABuffer(uint8_t *src, int strideX)
        {
            for (int y = 0; y < rect.h; y++)
            {
                memcpy(&buffer[y * rect.w * 4], &src[y * strideX], sizeof(uint8_t) * 4 * rect.w);
            }
        }

        void AtlasElement::copyToRGBABuffer(uint8_t *dst, int strideX, int xspacing, int yspacing)
        {
            for (int y = 0; y < rect.h; y++)
            {
                memcpy(&dst[rect.x * 4 + strideX * (y + rect.y)], &buffer[y * rect.w * 4], sizeof(uint8_t) * 4 * rect.w);
            }

            // borders
            for (int y = -yspacing; y < rect.h + yspacing; y++)
            {
                int srcY = y;
                if (y < 0)
                    srcY = 0;
                else if (y >= rect.h)
                    srcY = rect.h - 1;

                for (int x = -xspacing; x < rect.w + xspacing; x++)
                {

                    if (x >= 0 && x < rect.w && y >= 0 && y < rect.h)
                        continue;

                    int srcX = x;
                    if (x < 0)
                        srcX = 0;
                    else if (x >= rect.w)
                        srcX = rect.w - 1;

                    dst[(rect.x + x) * 4 + strideX * (y + rect.y) + 0] = buffer[(srcX + srcY * rect.w) * 4 + 0];
                    dst[(rect.x + x) * 4 + strideX * (y + rect.y) + 1] = buffer[(srcX + srcY * rect.w) * 4 + 1];
                    dst[(rect.x + x) * 4 + strideX * (y + rect.y) + 2] = buffer[(srcX + srcY * rect.w) * 4 + 2];
                    dst[(rect.x + x) * 4 + strideX * (y + rect.y) + 3] = 0; // alpha 0
                }
            }
        }

        void AtlasElement::copyToABuffer(uint8_t *dst, int strideX, int xspacing, int yspacing)
        {
            for (int y = 0; y < rect.h; y++)
            {
                for (int x = 0; x < rect.w; x++)
                {
                    dst[x + rect.x + strideX * (y + rect.y)] = buffer[(x + y * rect.w) * 4 + 3];
                }
            }

            // borders
            /*

            for (int y = -yspacing; y < rect.h + yspacing; y++) {
                int srcY = y;
                if (y < 0)
                    srcY = 0;
                else if (y >= rect.h)
                    srcY = rect.h - 1;

                for (int x = -xspacing; x < rect.w + xspacing; x++) {

                    if (x >= 0 && x < rect.w && y >= 0 && y < rect.h)
                        continue;

                    int srcX = x;
                    if (x < 0)
                        srcX = 0;
                    else if (x >= rect.w)
                        srcX = rect.w - 1;

                    dst[(rect.x + x) + strideX * (y + rect.y) + 0] = buffer[(srcX + srcY * rect.w) * 4 + 3];
                }
            }

            */
        }

        void AtlasElement::savePNG(const std::string &filename)
        {
            ITKExtension::Image::PNG::writePNG(filename.c_str(), rect.w, rect.h, 4, (char *)buffer);
        }

    }
}
