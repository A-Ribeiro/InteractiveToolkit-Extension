#pragma once

#include "common.h"

namespace ITKExtension
{
    namespace Model
    {

        class Vec3Key
        {
        public:
            float time;
            MathCore::vec3f value;

            void write(ITKExtension::IO::AdvancedWriter *writer) const
            {
                writer->writeFloat(time);
                writer->write<MathCore::vec3f>(value);
            }

            void read(ITKExtension::IO::AdvancedReader *reader)
            {
                time = reader->readFloat();
                value = reader->read<MathCore::vec3f>();
            }
        };

    }

}