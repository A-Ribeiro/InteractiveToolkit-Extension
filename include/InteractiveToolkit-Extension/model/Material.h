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

            std::unordered_map<std::string, std::string> stringValue;
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
                writer->write<std::unordered_map<std::string, std::string>>(stringValue);
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
                stringValue = reader->read<std::unordered_map<std::string, std::string>>();
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
            Material &operator=(const Material &v)
            {
                name = v.name;

                stringValue = v.stringValue;
                floatValue = v.floatValue;
                vec2Value = v.vec2Value;
                vec3Value = v.vec3Value;
                vec4Value = v.vec4Value;
                intValue = v.intValue;
                textures = v.textures;

                return *this;
            }
            Material(Material &&v)
            {
                (*this) = std::move(v);
            }
            Material &operator=(Material &&v)
            {
                name = std::move(v.name);

                stringValue = std::move(v.stringValue);
                floatValue = std::move(v.floatValue);
                vec2Value = std::move(v.vec2Value);
                vec3Value = std::move(v.vec3Value);
                vec4Value = std::move(v.vec4Value);
                intValue = std::move(v.intValue);
                textures = std::move(v.textures);

                return *this;
            }

            bool is_opaque() const
            {
                if (stringValue.find("gltf.alphaMode") != stringValue.end())
                    return stringValue.at("gltf.alphaMode") == "OPAQUE";
                else if (floatValue.find("opacity") != floatValue.end())
                    return floatValue.at("opacity") >= 1.0f;
                return true;
            }

            bool is_unlit() const
            {
                if (intValue.find("gltf.unlit") != intValue.end())
                    return intValue.at("gltf.unlit") != 0;
                else if (stringValue.find("shadingm") != stringValue.end())
                    return stringValue.at("shadingm") == "NO_SHADING";
                return true;
            }

            bool is_two_sided() const
            {
                if (intValue.find("twosided") != intValue.end())
                    return intValue.at("twosided") != 0;
                return false;
            }
            
        };

    }

}