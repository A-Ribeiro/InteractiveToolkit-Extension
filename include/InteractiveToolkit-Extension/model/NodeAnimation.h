#pragma once

#include "common.h"

#include "Vec3Key.h"
#include "QuatKey.h"

namespace ITKExtension
{
    namespace Model
    {

        // Defines how an animation channel behaves outside the defined time range.
        enum AnimBehaviour
        {
            // use the default node transformation
            AnimBehaviour_DEFAULT = 0x0,
            // The nearest key value is used without interpolation
            AnimBehaviour_CONSTANT = 0x1,
            // The value of the nearest two keys is linearly extrapolated for the current time value.
            AnimBehaviour_LINEAR = 0x2,
            // The animation is repeated.
            //  If the animation key go from n to m and the current
            //  time is t, use the value at (t-n) % (|m-n|).
            AnimBehaviour_REPEAT = 0x3,
        };

        class  NodeAnimation
        {
        public:
            std::string nodeName;
            std::vector<Vec3Key> positionKeys;
            std::vector<QuatKey> rotationKeys;
            std::vector<Vec3Key> scalingKeys;

            AnimBehaviour preState;
            AnimBehaviour postState;

            void write(ITKExtension::IO::AdvancedWriter *writer) const
            {
                writer->writeString(nodeName);
                writer->writeUInt8(preState);
                writer->writeUInt8(postState);

                WriteCustomVector<Vec3Key>(writer, positionKeys);
                WriteCustomVector<QuatKey>(writer, rotationKeys);
                WriteCustomVector<Vec3Key>(writer, scalingKeys);
            }

            void read(ITKExtension::IO::AdvancedReader *reader)
            {
                nodeName = reader->readString();
                preState = (AnimBehaviour)(reader->readUInt8());
                postState = (AnimBehaviour)(reader->readUInt8());

                ReadCustomVector<Vec3Key>(reader, &positionKeys);
                ReadCustomVector<QuatKey>(reader, &rotationKeys);
                ReadCustomVector<Vec3Key>(reader, &scalingKeys);
            }

            NodeAnimation()
            {
                preState = AnimBehaviour_DEFAULT;
                postState = AnimBehaviour_DEFAULT;
            }

            NodeAnimation(const NodeAnimation &v)
            {
                (*this) = v;
            }

            void operator=(const NodeAnimation &v)
            {
                nodeName = v.nodeName;

                positionKeys = v.positionKeys;
                rotationKeys = v.rotationKeys;
                scalingKeys = v.scalingKeys;

                preState = v.preState;
                postState = v.postState;
            }

            
        } ;

    }

}