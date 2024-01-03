#pragma once

#include "common.h"

namespace ITKExtension
{
    namespace Model
    {

        enum LightType
        {
            LightType_NONE = 0x0,
            LightType_DIRECTIONAL = 0x1,
            LightType_POINT = 0x2,
            LightType_SPOT = 0x3,
            LightType_AMBIENT = 0x4,
            LightType_AREA = 0x5
        };

        static const char *LightTypeToStr(LightType lt)
        {
            switch (lt)
            {
            case LightType_NONE:
                return "none";
            case LightType_DIRECTIONAL:
                return "directional";
            case LightType_POINT:
                return "point";
            case LightType_SPOT:
                return "spot";
            case LightType_AMBIENT:
                return "ambient";
            case LightType_AREA:
                return "area";
            }
            return "error to parse";
        }

        struct  DirectionalLight
        {
            MathCore::vec3f direction;
            MathCore::vec3f up;

            void write(ITKExtension::IO::AdvancedWriter *writer) const
            {
                writer->write<MathCore::vec3f>(direction);
                writer->write<MathCore::vec3f>(up);
            }

            void read(ITKExtension::IO::AdvancedReader *reader)
            {
                direction = reader->read<MathCore::vec3f>();
                up = reader->read<MathCore::vec3f>();
            }

            
        } ;

        struct  PointLight
        {
            MathCore::vec3f position;

            void write(ITKExtension::IO::AdvancedWriter *writer) const
            {
                writer->write<MathCore::vec3f>(position);
            }

            void read(ITKExtension::IO::AdvancedReader *reader)
            {
                position = reader->read<MathCore::vec3f>();
            }

            
        } ;

        struct  SpotLight
        {
            MathCore::vec3f position;
            MathCore::vec3f direction;
            MathCore::vec3f up;
            float angleInnerCone;
            float angleOuterCone;

            SpotLight()
            {
                angleInnerCone = 0;
                angleOuterCone = 0;
            }

            void write(ITKExtension::IO::AdvancedWriter *writer) const
            {
                writer->write<MathCore::vec3f>(position);
                writer->write<MathCore::vec3f>(direction);
                writer->write<MathCore::vec3f>(up);
                writer->writeFloat(angleInnerCone);
                writer->writeFloat(angleOuterCone);
            }

            void read(ITKExtension::IO::AdvancedReader *reader)
            {
                position = reader->read<MathCore::vec3f>();
                direction = reader->read<MathCore::vec3f>();
                up = reader->read<MathCore::vec3f>();
                angleInnerCone = reader->readFloat();
                angleOuterCone = reader->readFloat();
            }

            
        } ;

        struct  AmbientLight
        {
            MathCore::vec3f position;
            MathCore::vec3f direction;
            MathCore::vec3f up;

            void write(ITKExtension::IO::AdvancedWriter *writer) const
            {
                writer->write<MathCore::vec3f>(position);
                writer->write<MathCore::vec3f>(direction);
                writer->write<MathCore::vec3f>(up);
            }

            void read(ITKExtension::IO::AdvancedReader *reader)
            {
                position = reader->read<MathCore::vec3f>();
                direction = reader->read<MathCore::vec3f>();
                up = reader->read<MathCore::vec3f>();
            }

            
        } ;

        struct  AreaLight
        {
            MathCore::vec3f position;
            MathCore::vec3f direction;
            MathCore::vec3f up;
            MathCore::vec2f size;

            void write(ITKExtension::IO::AdvancedWriter *writer) const
            {
                writer->write<MathCore::vec3f>(position);
                writer->write<MathCore::vec3f>(direction);
                writer->write<MathCore::vec3f>(up);
                writer->write<MathCore::vec2f>(size);
            }

            void read(ITKExtension::IO::AdvancedReader *reader)
            {
                position = reader->read<MathCore::vec3f>();
                direction = reader->read<MathCore::vec3f>();
                up = reader->read<MathCore::vec3f>();
                size = reader->read<MathCore::vec2f>();
            }

            
        } ;

        class  Light
        {
        public:
            std::string name;

            LightType type;

            DirectionalLight directional;
            PointLight point;
            SpotLight spot;
            AmbientLight ambient;
            AreaLight area;

            // d = distance
            // Atten = 1/( att0 + att1 * d + att2 * d*d)
            float attenuationConstant;
            float attenuationLinear;
            float attenuationQuadratic;

            MathCore::vec3f colorDiffuse;
            MathCore::vec3f colorSpecular;
            MathCore::vec3f colorAmbient;

            void write(ITKExtension::IO::AdvancedWriter *writer) const
            {
                writer->writeString(name);
                writer->writeUInt8(type);

                directional.write(writer);
                point.write(writer);
                spot.write(writer);
                ambient.write(writer);
                area.write(writer);

                // d = distance
                // Atten = 1/( att0 + att1 * d + att2 * d*d)
                writer->writeFloat(attenuationConstant);
                writer->writeFloat(attenuationLinear);
                writer->writeFloat(attenuationQuadratic);

                writer->write<MathCore::vec3f>(colorDiffuse);
                writer->write<MathCore::vec3f>(colorSpecular);
                writer->write<MathCore::vec3f>(colorAmbient);
            }

            void read(ITKExtension::IO::AdvancedReader *reader)
            {
                name = reader->readString();
                type = (LightType)reader->readUInt8();

                directional.read(reader);
                point.read(reader);
                spot.read(reader);
                ambient.read(reader);
                area.read(reader);

                // d = distance
                // Atten = 1/( att0 + att1 * d + att2 * d*d)
                attenuationConstant = reader->readFloat();
                attenuationLinear = reader->readFloat();
                attenuationQuadratic = reader->readFloat();

                colorDiffuse = reader->read<MathCore::vec3f>();
                colorSpecular = reader->read<MathCore::vec3f>();
                colorAmbient = reader->read<MathCore::vec3f>();
            }

            Light()
            {
                attenuationConstant = 0;
                attenuationLinear = 0;
                attenuationQuadratic = 0;
            }

            Light(const Light &v)
            {
                (*this) = v;
            }

            void operator=(const Light &v)
            {
                name = v.name;

                type = v.type;

                directional = v.directional;
                point = v.point;
                spot = v.spot;
                ambient = v.ambient;
                area = v.area;

                // d = distance
                // Atten = 1/( att0 + att1 * d + att2 * d*d)
                attenuationConstant = v.attenuationConstant;
                attenuationLinear = v.attenuationLinear;
                attenuationQuadratic = v.attenuationQuadratic;

                colorDiffuse = v.colorDiffuse;
                colorSpecular = v.colorSpecular;
                colorAmbient = v.colorAmbient;
            }

            
        } ;

    }

}
