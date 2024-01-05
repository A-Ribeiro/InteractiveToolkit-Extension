#pragma once

// #include <InteractiveToolkit/InteractiveToolkit.h>
#include <InteractiveToolkit/MathCore/MathCore.h>
#include <InteractiveToolkit-Extension/io/AdvancedReader.h>
#include <InteractiveToolkit-Extension/io/AdvancedWriter.h>

namespace ITKExtension
{
    namespace Model
    {

        template <typename T>
        void WriteCustomVector(ITKExtension::IO::AdvancedWriter *writer, const std::vector<T> &v)
        {
            writer->writeUInt32((uint32_t)v.size());
            for (size_t i = 0; i < v.size(); i++)
                v[i].write(writer);
        }

        template <typename T>
        void WriteCustomStringMap(ITKExtension::IO::AdvancedWriter *writer, const std::map<std::string, T> &v)
        {
            writer->writeUInt32((uint32_t)v.size());
            typename std::map<std::string, T>::const_iterator it;
            for (it = v.begin(); it != v.end(); it++)
            {
                writeString(it->first);
                (it->second).write(writer);
            }
        }

        template <typename T>
        void ReadCustomVector(ITKExtension::IO::AdvancedReader *reader, std::vector<T> *v)
        {
            v->resize(reader->readUInt32());
            for (size_t i = 0; i < v->size(); i++)
                (*v)[i].read(reader);
        }

        template <typename T>
        void ReadCustomStringMap(ITKExtension::IO::AdvancedReader *reader, std::map<std::string, T> *result)
        {
            uint32_t size = reader->readUInt32();
            // std::map<std::string, T> result;
            (*result).clear();
            for (int i = 0; i < size; i++)
            {
                std::string aux = reader->readString();
                T value;
                value.read(reader);
                (*result)[aux] = value;
            }
        }

    }
}
