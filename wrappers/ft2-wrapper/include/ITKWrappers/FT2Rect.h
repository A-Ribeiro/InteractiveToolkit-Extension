#pragma once

#include <stdint.h>
// #include <InteractiveToolkit/InteractiveToolkit.h>

namespace ITKWrappers {
    namespace FT2 {

    /// \brief FT2 wrapper rectangle
    ///
    /// It holds one character metrics information.
    ///
    /// \author Alessandro Ribeiro
    ///
    class FT2Rect{
    public:
        union{
            struct{int top,left,width,height;};
            struct{int x,y,w,h;};
        };
        FT2Rect();
        FT2Rect(int _left,int _top,int _width,int _height);
    };
    
}
}
