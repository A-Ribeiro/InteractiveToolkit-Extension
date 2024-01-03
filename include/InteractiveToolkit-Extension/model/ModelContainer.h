#pragma once

#include "common.h"

#include "Animation.h"
#include "Light.h"
#include "Camera.h"
#include "Material.h"
#include "Geometry.h"
#include "Node.h"

namespace ITKExtension
{
    namespace Model
    {

        class  ModelContainer
        {
        public:
            std::vector<Animation> animations;
            std::vector<Light> lights;
            std::vector<Camera> cameras;
            std::vector<Material> materials;
            std::vector<Geometry> geometries;
            std::vector<Node> nodes; // the node[0] is the root

            void write(const char *filename) const
            {
                ITKExtension::IO::AdvancedWriter writer;
                
                WriteCustomVector<Animation>(&writer, animations);
                WriteCustomVector<Light>(&writer, lights);
                WriteCustomVector<Camera>(&writer, cameras);
                WriteCustomVector<Material>(&writer, materials);
                WriteCustomVector<Geometry>(&writer, geometries);
                WriteCustomVector<Node>(&writer, nodes);

                writer.writeToFile(filename, true);
            }

            void read(const char *filename)
            {
                ITKExtension::IO::AdvancedReader reader;
                reader.readFromFile(filename, true);

                ReadCustomVector<Animation>(&reader, &animations);
                ReadCustomVector<Light>(&reader, &lights);
                ReadCustomVector<Camera>(&reader, &cameras);
                ReadCustomVector<Material>(&reader, &materials);
                ReadCustomVector<Geometry>(&reader, &geometries);
                ReadCustomVector<Node>(&reader, &nodes);

                reader.close();
            }

        } ;

    }

}