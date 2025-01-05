#pragma once

#include "common.h"

#include "Bone.h"

namespace ITKExtension
{
    namespace Model
    {
        typedef uint32_t BitMask;

        // enum VertexFormat {
        const uint32_t CONTAINS_POS = (1 << 0);      // float3 position
        const uint32_t CONTAINS_NORMAL = (1 << 1);   // float3 normal
        const uint32_t CONTAINS_BINORMAL = (1 << 2); // float3 binormal
        const uint32_t CONTAINS_TANGENT = (1 << 3);  // float3 tangent
        // CONTAINS_COL = (1 << 4); // unsigned byte 4 color

        const uint32_t CONTAINS_UV0 = (1 << 5);  // float2
        const uint32_t CONTAINS_UV1 = (1 << 6);  // float2
        const uint32_t CONTAINS_UV2 = (1 << 7);  // float2
        const uint32_t CONTAINS_UV3 = (1 << 8);  // float2
        const uint32_t CONTAINS_UV4 = (1 << 9);  // float2
        const uint32_t CONTAINS_UV5 = (1 << 10); // float2
        const uint32_t CONTAINS_UV6 = (1 << 11); // float2
        const uint32_t CONTAINS_UV7 = (1 << 12); // float2

        const uint32_t CONTAINS_COLOR0 = (1 << 13); // byte4
        const uint32_t CONTAINS_COLOR1 = (1 << 14); // byte4
        const uint32_t CONTAINS_COLOR2 = (1 << 15); // byte4
        const uint32_t CONTAINS_COLOR3 = (1 << 16); // byte4
        const uint32_t CONTAINS_COLOR4 = (1 << 17); // byte4
        const uint32_t CONTAINS_COLOR5 = (1 << 18); // byte4
        const uint32_t CONTAINS_COLOR6 = (1 << 19); // byte4
        const uint32_t CONTAINS_COLOR7 = (1 << 20); // byte4

        // PAIR MATRIX[*] + PAIR float4(vertexindex) float4(weight)
        const uint32_t CONTAINS_VERTEX_WEIGHT16 = (1 << 27);
        const uint32_t CONTAINS_VERTEX_WEIGHT32 = (1 << 28);
        const uint32_t CONTAINS_VERTEX_WEIGHT64 = (1 << 29);
        const uint32_t CONTAINS_VERTEX_WEIGHT96 = (1 << 30);
        const uint32_t CONTAINS_VERTEX_WEIGHT128 = (1 << 31);

        const uint32_t CONTAINS_VERTEX_WEIGHT_ANY = CONTAINS_VERTEX_WEIGHT16 | CONTAINS_VERTEX_WEIGHT32 | CONTAINS_VERTEX_WEIGHT64 | CONTAINS_VERTEX_WEIGHT96 | CONTAINS_VERTEX_WEIGHT128;

        //};

        class  Geometry
        {

        public:
            std::string name;

            // VertexFormat: CONTAINS_POS | CONTAINS_NORMAL | ...
            BitMask format;
            uint32_t vertexCount;
            uint32_t indiceCountPerFace; // 1 - points, 2 - lines, 3 - triangles, 4 - quads

            std::vector<MathCore::vec3f> pos;
            std::vector<MathCore::vec3f> normals;
            std::vector<MathCore::vec3f> tangent;
            std::vector<MathCore::vec3f> binormal;
            std::vector<MathCore::vec3f> uv[8];
            std::vector<MathCore::vec4f> color[8]; // RGBA
            // std::vector<uint32_t> color[8];//RGBA

            std::vector<uint32_t> indice;

            uint32_t materialIndex;

            std::vector<Bone> bones;

            void write(ITKExtension::IO::AdvancedWriter *writer) const
            {
                writer->writeString(name);

                // VertexFormat: CONTAINS_POS | CONTAINS_NORMAL | ...
                writer->writeUInt32(format);
                writer->writeUInt32(vertexCount);
                writer->writeUInt32(indiceCountPerFace); // 1 - points, 2 - lines, 3 - triangles, 4 - quads
                writer->writeUInt32(materialIndex);

                writer->write<std::vector<MathCore::vec3f>>(pos);
                writer->write<std::vector<MathCore::vec3f>>(normals);
                writer->write<std::vector<MathCore::vec3f>>(tangent);

                // for (size_t i = 0; i < normals.size(); i++)
                // printf("%f\n", distance(binormal[i], cross(normals[i], tangent[i])));
                writer->write<std::vector<MathCore::vec3f>>(binormal);

                for (int i = 0; i < 8; i++)
                    writer->write<std::vector<MathCore::vec3f>>(uv[i]);
                for (int i = 0; i < 8; i++)
                    writer->write<std::vector<MathCore::vec4f>>(color[i]);
                // writer->writeVectorUInt32(color[i]);//RGBA
                writer->write<std::vector<uint32_t>>(indice);

                WriteCustomVector<Bone>(writer, bones);
            }

            void read(ITKExtension::IO::AdvancedReader *reader)
            {
                name = reader->readString();

                // VertexFormat: CONTAINS_POS | CONTAINS_NORMAL | ...
                format = reader->readUInt32();
                vertexCount = reader->readUInt32();
                indiceCountPerFace = reader->readUInt32(); // 1 - points, 2 - lines, 3 - triangles, 4 - quads
                materialIndex = reader->readUInt32();

                pos = reader->read<std::vector<MathCore::vec3f>>();
                normals = reader->read<std::vector<MathCore::vec3f>>();
                tangent = reader->read<std::vector<MathCore::vec3f>>();

                // binormal.resize(normals.size());
                // for (size_t i = 0; i < normals.size(); i++)
                // binormal[i] = cross(normals[i], tangent[i]);
                binormal = reader->read<std::vector<MathCore::vec3f>>();

                for (int i = 0; i < 8; i++)
                    uv[i] = reader->read<std::vector<MathCore::vec3f>>();
                for (int i = 0; i < 8; i++)
                    color[i] = reader->read<std::vector<MathCore::vec4f>>();
                // reader->readVectorUInt32(&color[i]);//RGBA
                indice = reader->read<std::vector<uint32_t>>();

                ReadCustomVector<Bone>(reader, &bones);
            }

            Geometry()
            {
                format = 0; // CONTAINS_POS | CONTAINS_NORMAL | ...
                vertexCount = 0;
                indiceCountPerFace = 0; // 1 - points, 2 - lines, 3 - triangles, 4 - quads
                materialIndex = 0;
            }

            // copy constructores
            Geometry(const Geometry &v)
            {
                (*this) = v;
            }
            void operator=(const Geometry &v)
            {

                name = v.name;

                format = v.format;
                vertexCount = v.vertexCount;
                indiceCountPerFace = v.indiceCountPerFace;

                pos = v.pos;
                normals = v.normals;
                tangent = v.tangent;
                binormal = v.binormal;
                for (int i = 0; i < 8; i++)
                {
                    uv[i] = v.uv[i];
                    color[i] = v.color[i];
                }

                indice = v.indice;

                materialIndex = v.materialIndex;

                bones = v.bones;
            }

            
        } ;

    }

}
