#pragma once

#include "common.h"

namespace ITKExtension
{
    namespace Model
    {

        class QuatKey
        {
        public:
            float time;
            MathCore::quatf value;

            void write(ITKExtension::IO::AdvancedWriter *writer) const
            {
                writer->writeFloat(time);
                writer->write<MathCore::quatf>(value);
            }

            void read(ITKExtension::IO::AdvancedReader *reader)
            {
                time = reader->readFloat();
                value = reader->read<MathCore::quatf>();
            }
        };

    }

}