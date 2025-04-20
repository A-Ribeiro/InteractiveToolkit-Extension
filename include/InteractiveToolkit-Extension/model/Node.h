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
            Node& operator=(const Node &v)
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