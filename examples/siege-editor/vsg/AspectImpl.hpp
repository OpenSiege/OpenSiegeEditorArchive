
#pragma once

#include "vsg/Aspect.hpp"

#include <vsg/maths/quat.h>
#include <vsg/maths/vec2.h>
#include <vsg/maths/vec3.h>

#include <vsg/state/PipelineLayout.h>

// the asp is a bit complex and trying to make it work has been a fluid experience so I went with a Pimpl approach

namespace ehb
{
    struct Aspect::Impl
    {
        Impl() = default;
        ~Impl() = default;

        // "Raw" version numbers of the ASP sections.
        // There are also alternate versions that can be fetched by 'versionOf()'.
        struct Version
        {
            enum Enum
            {
                null = 0,
                v1_2 = 513,
                v1_3 = 769,
                v2_0 = 2,
                v2_1 = 258,
                v2_2 = 514,
                v2_3 = 770,
                v2_4 = 1026,
                v2_5 = 1282,
                v4_0 = 4,
                v4_1 = 260,
                v5_0 = 5 // DS Legends of Aranna expansion.
            };
        };

        static uint32_t versionOf(const uint32_t v)
        {
            // Adapted from "ASPImport.ms".
            // The question to ask here is, why didn't
            // they save these numbers in the file, instead
            // of the wacky version constants?
            switch (v)
            {
            case Version::v1_2: return 12;
            case Version::v1_3: return 13;
            case Version::v2_0: return 20;
            case Version::v2_1: return 21;
            case Version::v2_2: return 22;
            case Version::v2_3: return 23;
            case Version::v2_4: return 24;
            case Version::v2_5: return 25;
            case Version::v4_0: return 40;
            case Version::v4_1: return 41;
            case Version::v5_0: return 50;
            default: return Version::null;
            } // switch (v)
        }

        // Indexes into the shared corner (vertex) array for a face triangle.
        struct TriIndex
        {
            uint32_t index[3];
        };

        // A model vertex, which can be thought of as a "corner"...
        // "Corner" is the term used in the 3DMax export scripts.
        struct WCornerInfo
        {
            vsg::vec3 position;
            vsg::vec3 normal;
            vsg::quat weight;
            vsg::vec2 texCoord;
            unsigned char color[4];
            unsigned char bone[4];
        };

        // A simpler model vertex (corner), without animation data.
        // This was probably used for static geometry.
        struct CornerInfo
        {
            uint32_t vtxIndex;
            vsg::vec3 position;
            vsg::vec3 normal;
            vsg::vec2 texCoord;
            unsigned char color[4];
        };

        struct MatInfo
        {
            uint32_t textureIndex;
            uint32_t faceSpan;
        };

        struct FaceInfo
        {
            std::vector<uint32_t> cornerStart;
            std::vector<uint32_t> cornerSpan;
            std::vector<TriIndex> cornerIndex;
        };

        struct BoneInfo
        {
            uint32_t parentIndex;
            uint32_t flags;
            std::string name;
        };

        struct RPosInfo
        {
            vsg::quat rotation;
            vsg::vec3 position;
        };

        struct SubMesh
        {
            uint32_t textureCount = 0;
            uint32_t vertexCount = 0;
            uint32_t cornerCount = 0;
            uint32_t faceCount = 0;
            uint32_t stitchCount = 0;

            std::vector<MatInfo> matInfo;      // BSMM
            std::vector<vsg::vec3> positions;  // BVTX
            std::vector<CornerInfo> corners;   // BCRN
            std::vector<WCornerInfo> wCorners; // WCRN
            FaceInfo faceInfo;                 // BTRI
        };

        uint32_t sizeTextField;
        uint32_t boneCount;
        uint32_t textureCount;
        uint32_t vertexCount;
        uint32_t subMeshCount;
        uint32_t renderFlags;

        const vsg::PipelineLayout* pipelineLayout;
        vsg::ref_ptr<const vsg::Options> options; // ugh

        std::vector<SubMesh> subMeshes;
        std::vector<BoneInfo> boneInfos;
        std::vector<std::string> textureNames;
        std::vector<RPosInfo> rposInfoAbI; // absolute inverse?
        std::vector<RPosInfo> rposInfoRel; // relative?
    };
} // namespace ehb