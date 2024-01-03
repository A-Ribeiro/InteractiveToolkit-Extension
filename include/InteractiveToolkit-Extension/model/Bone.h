#pragma once

#include "common.h"

namespace ITKExtension
{
    namespace Model
    {

        class VertexWeight
        {
        public:
            uint32_t vertexID;
            float weight;

            void write(ITKExtension::IO::AdvancedWriter *writer) const
            {
                writer->writeUInt32(vertexID);
                writer->writeFloat(weight);
            }

            void read(ITKExtension::IO::AdvancedReader *reader)
            {
                vertexID = reader->readUInt32();
                weight = reader->readFloat();
            }
        };

        class Bone
        {
        public:
            std::string name;
            std::vector<VertexWeight> weights;

            // MathCore::mat4f offset;

            void write(ITKExtension::IO::AdvancedWriter *writer) const
            {
                writer->writeString(name);
                // writer->writeMat4(offset);
                WriteCustomVector<VertexWeight>(writer, weights);
            }

            void read(ITKExtension::IO::AdvancedReader *reader)
            {
                name = reader->readString();
                // offset = reader->readMat4();
                ReadCustomVector<VertexWeight>(reader, &weights);
            }

            Bone()
            {
            }

            Bone(const Bone &v)
            {
                (*this) = v;
            }
            void operator=(const Bone &v)
            {
                name = v.name;
                weights = v.weights;
                // offset = v.offset;
            }
        };

    }

}