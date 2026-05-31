// #include <InteractiveToolkit/ITKCommon/ITKCommon.h> // ITKAbort
#include <InteractiveToolkit/ITKCommon/ITKAbort.h>

#include <InteractiveToolkit-Extension/io/AdvancedReader.h>
#include <InteractiveToolkit-Extension/io/AdvancedWriter.h>

#include <InteractiveToolkit-Extension/atlas/Atlas.h>

#include <InteractiveToolkit-Extension/image/PNG.h>
#include <InteractiveToolkit-Extension/image/JPG.h>

namespace ITKExtension
{
    namespace Atlas
    {

        void Atlas::resetPositions()
        {
            for (size_t i = 0; i < elements.size(); i++)
            {
                elements[i]->rect.x = xspacing;
                elements[i]->rect.y = yspacing;
            }
        }

        bool Atlas::colideWithAnyObject(AtlasElement *base_element, int maxIterate)
        {
            for (size_t i = 0; i < maxIterate; i++)
            {
                AtlasElement *element = elements[i].get();
                if (base_element->rect.overlaps(element->rect, xspacing, yspacing))
                    return true;
            }
            return false;
        }

        std::vector<AtlasRect> Atlas::computeInsertPositionArray(const AtlasRect &screen, int currY, int maxIterate)
        {
            std::vector<AtlasRect> result;

            result.push_back(AtlasRect(xspacing / 2, currY, 0, 0));

            int xmax = screen.w;

            AtlasRect overlapTest(0, currY, screen.w, 1);

            for (size_t i = 0; i < maxIterate; i++)
            {
                AtlasElement *element = elements[i].get();
                if (!overlapTest.overlaps(element->rect, xspacing, yspacing))
                    continue;

                result.push_back(AtlasRect(element->rect.x + element->rect.w + xspacing, currY, 0, 0));
            }

            return result;
        }

        AtlasElement *Atlas::findMinY(const AtlasRect &screen, int currY, int maxIterate)
        {

            if (currY + yspacing >= screen.h)
                return nullptr;

            AtlasElement *result = nullptr;

            int ymax = screen.h;

            AtlasRect overlapTest(0, currY, screen.w, 1);

            for (size_t i = 0; i < maxIterate; i++)
            {
                AtlasElement *element = elements[i].get();
                if (!overlapTest.overlaps(element->rect, xspacing, yspacing))
                    continue;

                int ymax_aux = element->rect.y + element->rect.h + yspacing;
                if (ymax_aux < ymax)
                {
                    ymax = ymax_aux;
                    result = element;
                }
            }

            return result;
        }

        bool Atlas::repositionAllElements(const AtlasRect &screen, bool fastMode)
        {

            if (elements.size() == 0)
                return true;

            std::vector<AtlasRect> possibleInsertPositions;

            int currY = yspacing / 2;

            for (int i = 0; i < (int)elements.size(); i++)
            {

                if (!fastMode)
                    currY = yspacing / 2;

                AtlasElement *element = elements[i].get();
                if (element->rect.w == 0 || element->rect.h == 0)
                    continue;

                possibleInsertPositions = computeInsertPositionArray(screen, currY, i);

                bool foundPlace = false;
                while (!foundPlace)
                {
                    // each position test overlap with other objects
                    for (int j = 0; j < (int)possibleInsertPositions.size(); j++)
                    {
                        AtlasRect &insertPos = possibleInsertPositions[j];
                        element->rect.setXY(insertPos.x, insertPos.y);
                        if (!colideWithAnyObject(element, i) && element->rect.inside(screen, xspacing / 2, yspacing / 2))
                        {
                            foundPlace = true;
                            break;
                        }
                    }

                    if (!foundPlace)
                    {
                        AtlasElement *minY = findMinY(screen, currY, i);
                        if (minY == nullptr)
                            return false;
                        currY = minY->rect.maxYInclusive() + 1 + yspacing;

                        possibleInsertPositions = computeInsertPositionArray(screen, currY, i);
                    }
                }
            }
            return true;
        }

        void Atlas::clearElements()
        {
            // for (size_t i = 0; i < elements.size(); i++)
            // {
            //     delete elements[i];
            // }
            elements.clear();
        }

        Atlas::Atlas(int _xspacing, int _yspacing)
        {
            xspacing = _xspacing;
            yspacing = _yspacing;

            if (xspacing % 2 == 1)
                xspacing++;
            if (yspacing % 2 == 1)
                yspacing++;
        }

        Atlas::~Atlas()
        {
            clearElements();
        }

        std::shared_ptr<AtlasElement> Atlas::addElement(const std::string &name, int w, int h)
        {
            std::shared_ptr<AtlasElement> result = AtlasElement::CreateShared(w, h);
            result->name = name;
            elements.push_back(result);
            return result;
        }

        void Atlas::removeLastInsertedElement()
        {
            if (!elements.empty())
                elements.pop_back();
        }

        void Atlas::organizePositions(bool fastMode)
        {

            AtlasRect res(128, 128);

            int sideToIncrease = 0;
            while (!repositionAllElements(res, fastMode))
            {
                if (sideToIncrease == 0)
                    res.w = res.w << 1;
                else
                    res.h = res.h << 1;
                sideToIncrease = (sideToIncrease + 1) % 2;
            }

            textureResolution = res;

            // change the texture increase pattern
            res = AtlasRect(128, 128);
            sideToIncrease = 1;

            while (!repositionAllElements(res, fastMode))
            {
                if (sideToIncrease == 0)
                    res.w = res.w << 1;
                else
                    res.h = res.h << 1;
                sideToIncrease = (sideToIncrease + 1) % 2;
            }

            if (res.w * res.h < textureResolution.w * textureResolution.h)
            {
                textureResolution = res;
            }

            repositionAllElements(textureResolution, fastMode);
        }

        std::shared_ptr<uint8_t[]> Atlas::createRGBA() const
        {

            ITK_ABORT((textureResolution.w == 0 || textureResolution.h == 0), "Error to create texture from atlas.\n");
            // if (textureResolution.w == 0 || textureResolution.h == 0){
            //     fprintf(stderr,"Error to create texture from atlas.\n");
            //     exit(-1);
            // }

            std::shared_ptr<uint8_t[]> result = std::shared_ptr<uint8_t[]>(new uint8_t[textureResolution.w * textureResolution.h * 4], std::default_delete<uint8_t[]>());

            // set all color to 0
            memset(result.get(), 0, sizeof(uint8_t) * textureResolution.w * textureResolution.h * 4);

            /*
            for (int y = 0; y < textureResolution.h; y++) {
                for (int x = 0; x < textureResolution.w; x++) {
                    result[(x + y * textureResolution.w) * 4 + 0] = 0;
                    result[(x + y * textureResolution.w) * 4 + 1] = 0;
                    result[(x + y * textureResolution.w) * 4 + 2] = 0;
                    result[(x + y * textureResolution.w) * 4 + 3] = 0;
                }
            }
            */

            for (size_t i = 0; i < elements.size(); i++)
            {
                AtlasElement *element = elements[i].get();
                if (element->rect.w == 0 || element->rect.h == 0)
                    continue;
                element->copyToRGBABuffer(result.get(), textureResolution.w * 4, xspacing / 2, yspacing / 2);
            }

            return result;
        }

        // void Atlas::releaseRGBA(std::shared_ptr<uint8_t[]> *data) const
        // {
        //     data->reset();
        // }

        std::shared_ptr<uint8_t[]> Atlas::createA() const
        {

            ITK_ABORT((textureResolution.w == 0 || textureResolution.h == 0), "Error to create texture from atlas.\n");
            // if (textureResolution.w == 0 || textureResolution.h == 0) {
            //     fprintf(stderr, "Error to create texture from atlas.\n");
            //     exit(-1);
            // }

            std::shared_ptr<uint8_t[]> result = std::shared_ptr<uint8_t[]>(new uint8_t[textureResolution.w * textureResolution.h], std::default_delete<uint8_t[]>());

            // set all color to 0
            memset(result.get(), 0, sizeof(uint8_t) * textureResolution.w * textureResolution.h);

            for (size_t i = 0; i < elements.size(); i++)
            {
                AtlasElement *element = elements[i].get();
                if (element->rect.w == 0 || element->rect.h == 0)
                    continue;
                element->copyToABuffer(result.get(), textureResolution.w, xspacing / 2, yspacing / 2);
            }

            return result;
        }
        // void Atlas::releaseA(std::shared_ptr<uint8_t[]> *data) const
        // {
        //     data->reset();
        // }

        void Atlas::write(ITKExtension::IO::AdvancedWriter *writer) const
        {
            writer->writeUInt32((uint32_t)elements.size());
            for (size_t i = 0; i < elements.size(); i++)
            {
                writer->writeString(elements[i]->name);
                elements[i]->rect.write(writer);
            }
        }

        void Atlas::read(ITKExtension::IO::AdvancedReader *reader)
        {
            clearElements();
            elements.resize(reader->readUInt32());
            for (size_t i = 0; i < elements.size(); i++)
            {
                std::shared_ptr<AtlasElement> element = AtlasElement::CreateShared();
                element->read(reader);
                elements[i] = element;
            }
        }

        void Atlas::savePNG(const std::string &filename) const
        {
            auto image = createRGBA();
            ITKExtension::Image::PNG::writePNG(filename.c_str(), textureResolution.w, textureResolution.h, 4, (char *)image.get());
            // releaseRGBA(&image);
        }

        void Atlas::savePNG_Alpha(const std::string &filename) const
        {
            auto image = createA();
            ITKExtension::Image::PNG::writePNG(filename.c_str(), textureResolution.w, textureResolution.h, 1, (char *)image.get());
            // releaseA(&image);
        }

        void Atlas::writeTable(const std::string &filename) const
        {
            ITKExtension::IO::AdvancedWriter writer;
            write(&writer);
            writer.writeToFile(filename.c_str());
            // writer.close();
        }
    }
}
