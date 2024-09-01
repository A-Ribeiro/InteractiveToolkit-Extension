#pragma once

#include "common.h"

#include "Texture.h"

namespace ITKExtension
{
    namespace Model
    {

        class Material
        {
        public:
            std::string name;

            std::unordered_map<std::string, float> floatValue;
            std::unordered_map<std::string, MathCore::vec2f> vec2Value;
            std::unordered_map<std::string, MathCore::vec3f> vec3Value;
            std::unordered_map<std::string, MathCore::vec4f> vec4Value;
            std::unordered_map<std::string, int> intValue;

            std::vector<Texture> textures;

            // std::string textureDiffuse;
            // std::string textureNormal;

            void write(ITKExtension::IO::AdvancedWriter *writer) const
            {
                writer->writeString(name);
                writer->write<std::unordered_map<std::string, float>>(floatValue);
                writer->write<std::unordered_map<std::string, MathCore::vec2f>>(vec2Value);
                writer->write<std::unordered_map<std::string, MathCore::vec3f>>(vec3Value);
                writer->write<std::unordered_map<std::string, MathCore::vec4f>>(vec4Value);
                writer->write<std::unordered_map<std::string, int>>(intValue);

                WriteCustomVector<Texture>(writer, textures);
            }

            void read(ITKExtension::IO::AdvancedReader *reader)
            {
                name = reader->readString();
                floatValue = reader->read<std::unordered_map<std::string, float>>();
                vec2Value = reader->read<std::unordered_map<std::string, MathCore::vec2f>>();
                vec3Value = reader->read<std::unordered_map<std::string, MathCore::vec3f>>();
                vec4Value = reader->read<std::unordered_map<std::string, MathCore::vec4f>>();
                intValue = reader->read<std::unordered_map<std::string, int>>();

                ReadCustomVector<Texture>(reader, &textures);
            }

            Material()
            {
            }

            Material(const Material &v)
            {
                (*this) = v;
            }
            void operator=(const Material &v)
            {
                name = v.name;
                floatValue = v.floatValue;

                vec2Value = v.vec2Value;
                vec3Value = v.vec3Value;
                vec4Value = v.vec4Value;
                intValue = v.intValue;
                textures = v.textures;
            }
        };

    }

}