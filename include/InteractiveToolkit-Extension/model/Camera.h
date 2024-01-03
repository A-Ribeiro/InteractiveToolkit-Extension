#pragma once

#include "common.h"

namespace ITKExtension
{
    namespace Model
    {

        class Camera
        {
        public:
            std::string name;

            MathCore::vec3f pos;
            MathCore::vec3f up;
            MathCore::vec3f forward;

            float horizontalFOVrad;
            float nearPlane;
            float farPlane;
            float aspect;

            float verticalFOVrad;

            float computeHeight(float width)
            {
                return width / aspect;
            }
            float computeWidth(float height)
            {
                return height * aspect;
            }

            void write(ITKExtension::IO::AdvancedWriter *writer) const
            {

                writer->writeString(name);

                writer->write<MathCore::vec3f>(pos);
                writer->write<MathCore::vec3f>(up);
                writer->write<MathCore::vec3f>(forward);

                writer->writeFloat(horizontalFOVrad);
                writer->writeFloat(nearPlane);
                writer->writeFloat(farPlane);
                writer->writeFloat(aspect);

                writer->writeFloat(verticalFOVrad);
            }

            void read(ITKExtension::IO::AdvancedReader *reader)
            {
                name = reader->readString();

                pos = reader->read<MathCore::vec3f>();
                up = reader->read<MathCore::vec3f>();
                forward = reader->read<MathCore::vec3f>();

                horizontalFOVrad = reader->readFloat();
                nearPlane = reader->readFloat();
                farPlane = reader->readFloat();
                aspect = reader->readFloat();

                verticalFOVrad = reader->readFloat();
            }

            Camera()
            {
                pos = MathCore::vec3f(0, 0, 0);
                up = MathCore::vec3f(0, 1, 0);
                forward = MathCore::vec3f(0, 0, 1);

                horizontalFOVrad = MathCore::OP<float>::deg_2_rad(60.0f);
                nearPlane = 0.1f;
                farPlane = 100.0f;
                aspect = 1.0f;

                verticalFOVrad = MathCore::OP<float>::deg_2_rad(60.0f);
            }

            Camera(const Camera &v)
            {
                (*this) = v;
            }

            void operator=(const Camera &v)
            {
                name = v.name;
                pos = v.pos;
                up = v.up;
                forward = v.forward;
                horizontalFOVrad = v.horizontalFOVrad;
                nearPlane = v.nearPlane;
                farPlane = v.farPlane;
                aspect = v.aspect;
                verticalFOVrad = v.verticalFOVrad;
            }
        };

    }

}