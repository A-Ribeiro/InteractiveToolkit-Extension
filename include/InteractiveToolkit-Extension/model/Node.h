#pragma once

#include "common.h"

namespace ITKExtension
{
    namespace Model
    {

        class Node
        {
        public:
            std::string name;
            std::vector<uint32_t> geometries;
            std::vector<uint32_t> children;
            MathCore::mat4f transform;

            inline MathCore::vec3f getLocalPosition() const { using namespace MathCore; return CVT<vec4f>::toVec3(transform * vec4f(0, 0, 0, 1)); }
            inline MathCore::vec3f getLocalScale() const { using namespace MathCore; return vec3f(OP<vec4f>::length(transform[0]), OP<vec4f>::length(transform[1]), OP<vec4f>::length(transform[2])); }
            inline MathCore::quatf getLocalRotation() const { using namespace MathCore; return GEN<quatf>::fromMat4(transform); }

            void write(ITKExtension::IO::AdvancedWriter *writer) const
            {
                writer->writeString(name);
                writer->write<MathCore::mat4f>(transform);
                writer->write<std::vector<uint32_t>>(geometries);
                writer->write<std::vector<uint32_t>>(children);
            }

            void read(ITKExtension::IO::AdvancedReader *reader)
            {
                name = reader->readString();
                transform = reader->read<MathCore::mat4f>();
                geometries = reader->read<std::vector<uint32_t>>();
                children = reader->read<std::vector<uint32_t>>();
            }

            Node()
            {
            }

            // copy constructores
            Node(const Node &v)
            {
                (*this) = v;
            }
            Node &operator=(const Node &v)
            {
                name = v.name;
                geometries = v.geometries;
                children = v.children;
                transform = v.transform;

                return *this;
            }
        };

    }

}