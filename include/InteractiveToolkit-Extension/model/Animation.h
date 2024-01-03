#pragma once

#include "common.h"

#include "NodeAnimation.h"

namespace ITKExtension
{
    namespace Model
    {

        class Animation
        {
        public:
            std::string name;
            float durationTicks;
            float ticksPerSecond;
            std::vector<NodeAnimation> channels;

            void write(ITKExtension::IO::AdvancedWriter *writer) const
            {
                writer->writeString(name);
                writer->writeFloat(durationTicks);
                writer->writeFloat(ticksPerSecond);

                WriteCustomVector<NodeAnimation>(writer, channels);
            }

            void read(ITKExtension::IO::AdvancedReader *reader)
            {
                name = reader->readString();
                durationTicks = reader->readFloat();
                ticksPerSecond = reader->readFloat();

                ReadCustomVector<NodeAnimation>(reader, &channels);
            }

            Animation()
            {
                durationTicks = 0;
                ticksPerSecond = 0;
            }

            Animation(const Animation &v)
            {
                (*this) = v;
            }

            void operator=(const Animation &v)
            {
                name = v.name;

                durationTicks = v.durationTicks;

                ticksPerSecond = v.ticksPerSecond;
                channels = v.channels;
            }
        };

    }

}
