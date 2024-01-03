#include <ITKWrappers/FT2Rect.h>

namespace ITKWrappers
{
    namespace FT2
    {

        FT2Rect::FT2Rect()
        {
            left = top = width = height = 0;
        }

        FT2Rect::FT2Rect(int _left, int _top, int _width, int _height)
        {
            left = _left;
            top = _top;
            width = _width;
            height = _height;
        }

    }
}
