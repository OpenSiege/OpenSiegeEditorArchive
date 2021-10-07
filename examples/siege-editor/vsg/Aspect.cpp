
#include "Aspect.hpp"
#include "AspectImpl.hpp"

#include <algorithm>
#include <functional>

#include <spdlog/spdlog.h>

#include <vsg/io/read.h> // temp
#include <vsg/nodes/VertexIndexDraw.h>
#include <vsg/state/DescriptorSet.h>

#include <vsg/commands/BindIndexBuffer.h>
#include <vsg/commands/BindVertexBuffers.h>
#include <vsg/commands/Commands.h>
#include <vsg/commands/DrawIndexed.h>

namespace ehb
{
    Aspect::Aspect(std::shared_ptr<Impl> impl) :
        d(std::move(impl))
    {
        auto log = spdlog::get("log");

        log->set_level(spdlog::level::debug);

        log->debug("asp has {} sub meshes", d->subMeshes.size());

        for (const auto& mesh : d->subMeshes)
            for (auto kk = 0; kk < 1; ++kk)
            {
                log->debug("asp subMesh has {} textures", mesh.textureCount);

                uint32_t f = 0; // track which face the loader is loading across the sub mesh
                for (uint32_t i = 0; i < mesh.textureCount; ++i)
                {
                    // create the geometry which will be drawn
                    // vsg::ref_ptr<vsg::Geometry> geometry = new vsg::Geometry;

                    // textures are stored directly against the mesh
                    // TODO: should we store the vsg::Image against AspectImpl?
                    const std::string& imageFilename = d->textureNames[i];

                    auto vertices = vsg::vec3Array::create(mesh.cornerCount);
                    auto normals = vsg::vec3Array::create(mesh.cornerCount);
                    auto texCoords = vsg::vec2Array::create(mesh.cornerCount);
                    auto colors = vsg::vec3Array::create(mesh.cornerCount); // this is actually 4 colors but im lazy rn

                    // this has to match the incoming pipe which was defined by the siege nodes
                    auto attributeArrays = vsg::DataList{vertices, colors, texCoords};

#if 0
                // this is replaced by the attributeArrays call in vsg
                geometry->setVertexArray(vertices);
                geometry->setNormalArray(normals, vsg::Array::BIND_PER_VERTEX);
                geometry->setTexCoordArray(0, texCoords);
                geometry->setColorArray(colors, vsg::Array::BIND_PER_VERTEX);
#endif

                    for (uint32_t cornerCounter = 0; cornerCounter < mesh.cornerCount; ++cornerCounter)
                    {
                        const auto& c = mesh.corners[cornerCounter];
                        const auto& w = mesh.wCorners[cornerCounter];

                        (*vertices)[cornerCounter] = w.position;
                        (*texCoords)[cornerCounter] = c.texCoord;
                        (*normals)[cornerCounter] = c.normal;
                        (*colors)[cornerCounter] = vsg::vec3(c.color[0], c.color[1], c.color[2]);
                    }

                    auto elements = vsg::uintArray::create(mesh.matInfo[i].faceSpan * 3);
                    //vsg::ref_ptr<vsg::DrawElementsUInt> elements = new vsg::DrawElementsUInt(GL_TRIANGLES);
                    //geometry->addPrimitiveSet(elements);

                    ///for (uint32_t fpt = 0; fpt < mesh.matInfo[i].faceSpan; ++fpt)
                    for (uint32_t fpt = 0; fpt < mesh.matInfo[i].faceSpan * 3; fpt += 3)
                    {
                        (*elements)[fpt] = mesh.faceInfo.cornerIndex.at(f / 3).index[0] + mesh.faceInfo.cornerStart[i];
                        (*elements)[fpt + 1] = mesh.faceInfo.cornerIndex.at(f / 3).index[1] + mesh.faceInfo.cornerStart[i];
                        (*elements)[fpt + 2] = mesh.faceInfo.cornerIndex.at(f / 3).index[2] + mesh.faceInfo.cornerStart[i];

                        f += 3;
                    }

                    // make sure to calculate our bounding boxes
                    //geometry->setInitialBound(geometry->computeBoundingBox());

                    // by default we draw static geometry
                    //pseudoRoot->addChild(geometry);

                    // make sure the skeleton is ready to go if we need it
                    //skeleton->addChild(rigGeometry);

                    if (auto textureData = vsg::read_cast<vsg::Data>(imageFilename, d->options); textureData != nullptr)
                    {
                        auto texture = vsg::DescriptorImage::create(vsg::Sampler::create(), textureData, 0, 0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
                        assert(texture != nullptr);

                        //! NOTE: should we be accessing the first element of the vector?
                        auto descriptorSet = vsg::DescriptorSet::create(d->pipelineLayout->setLayouts[0], vsg::Descriptors{texture});
                        assert(descriptorSet != nullptr);

                        auto bindDescriptorSets = vsg::BindDescriptorSets::create(VK_PIPELINE_BIND_POINT_GRAPHICS, const_cast<vsg::PipelineLayout*>(d->pipelineLayout), 0, vsg::DescriptorSets{descriptorSet});
                        assert(bindDescriptorSets != nullptr);

                        addChild(bindDescriptorSets);
                    }

#if 1
                    auto vid = vsg::VertexIndexDraw::create();
                    vid->arrays = attributeArrays;
                    vid->indices = elements;
                    vid->indexCount = static_cast<uint32_t>(elements->size());
                    vid->instanceCount = 1;

                    addChild(vid);
#else
                    auto commands = vsg::Commands::create();
                    commands->addChild(vsg::BindVertexBuffers::create(0, attributeArrays));
                    commands->addChild(vsg::BindIndexBuffer::create(elements));
                    commands->addChild(vsg::DrawIndexed::create(elements->valueCount(), 1, 0, 0, 0));

                    addChild(commands);
#endif
                }
            }

#if 0
        debugDrawingGroups.resize(3);

        skeleton = new osgAnimation::Skeleton;

        // be default meshes aren't rendered using their skeleton so lets mimic a root bone
        pseudoRoot = new vsg::MatrixTransform;
        addChild(pseudoRoot);

        // put the root bone as the first child of skeleton for easy acces
        skeleton->addChild(new osgAnimation::Bone(d->boneInfos[0].name));

        // it looks like the skeleton callback just validates the skeleton and then stays in the callbacks
        // TODO: osgAnimation should expose the ValidateSkeletonVisitor to the API and let you call it once when you need it
        skeleton->setUpdateCallback(nullptr);

        log->debug("asp has {} sub meshes", d->subMeshes.size());

        for (const auto& mesh : d->subMeshes)
        {
            log->debug("asp subMesh has {} textures", mesh.textureCount);

            uint32_t f = 0; // track which face the loader is loading across the sub mesh
            for (uint32_t i = 0; i < mesh.textureCount; ++i)
            {
                // create the geometry which will be drawn
                vsg::ref_ptr<vsg::Geometry> geometry = new vsg::Geometry;

                // create the rigged geometry that is used to do our weighting
                vsg::ref_ptr<osgAnimation::RigGeometry> rigGeometry = new osgAnimation::RigGeometry;
                rigGeometry->setInfluenceMap(new osgAnimation::VertexInfluenceMap);
                rigGeometry->setSourceGeometry(geometry);

                // store our rigGeometry for later use
                rigGeometryVec.push_back(rigGeometry);

                // add the geometry to our aspect for quick lookups
                geometryVec.push_back(geometry);

                // textures are stored directly against the mesh
                // TODO: should we store the vsg::Image against AspectImpl?
                const std::string imageFilename = d->textureNames[i] + ".raw";
                vsg::ref_ptr<vsg::Image> image = osgDB::readImageFile(imageFilename);

                geometry->setName(d->textureNames[i]);

                if (image != nullptr)
                {
                    vsg::Texture2D* texture = new vsg::Texture2D(image);
                    geometry->getOrCreateStateSet()->setTextureAttributeAndModes(0, texture);
                    geometry->getOrCreateStateSet()->setMode(GL_BLEND, vsg::StateAttribute::ON);

                    texture->setWrap(vsg::Texture2D::WRAP_S, vsg::Texture2D::REPEAT);
                    texture->setWrap(vsg::Texture2D::WRAP_T, vsg::Texture2D::REPEAT);

                    log->debug("loaded image {} for asp sub mesh: {}", i, imageFilename);
                }

                vsg::ref_ptr<vsg::Vec3Array> vertices = new vsg::Vec3Array;
                vsg::ref_ptr<vsg::Vec3Array> normals = new vsg::Vec3Array;
                vsg::ref_ptr<vsg::Vec2Array> texCoords = new vsg::Vec2Array;
                vsg::ref_ptr<vsg::Vec4Array> colors = new vsg::Vec4Array;

                geometry->setVertexArray(vertices);
                geometry->setNormalArray(normals, vsg::Array::BIND_PER_VERTEX);
                geometry->setTexCoordArray(0, texCoords);
                geometry->setColorArray(colors, vsg::Array::BIND_PER_VERTEX);

                for (uint32_t cornerCounter = 0; cornerCounter < mesh.cornerCount; ++cornerCounter)
                {
                    const auto& c = mesh.corners[cornerCounter];
                    const auto& w = mesh.wCorners[cornerCounter];

                    vertices->push_back(c.position);
                    texCoords->push_back(c.texCoord);
                    normals->push_back(c.normal);
                    colors->push_back(vsg::Vec4(c.color[0], c.color[1], c.color[2], c.color[3]));

                    osgAnimation::VertexInfluenceMap& influenceMap = *rigGeometry->getInfluenceMap();

                    // There are 4 weights per bone
                    for (uint32_t weight = 0; weight < 4; ++weight)
                    {
                        int32_t boneId = static_cast<int32_t> (w.bone[weight]);
                        float value = static_cast<float> (w.weight[weight]);

                        if (boneId != 0) {

                            osgAnimation::VertexInfluence& vi = getVertexInfluence(influenceMap, d->boneInfos[boneId - 1].name);
                            vi.push_back(osgAnimation::VertexIndexWeight(cornerCounter, value));
                        }
                    }

                }

                vsg::ref_ptr<vsg::DrawElementsUInt> elements = new vsg::DrawElementsUInt(GL_TRIANGLES);
                geometry->addPrimitiveSet(elements);

                for (uint32_t fpt = 0; fpt < mesh.matInfo[i].faceSpan; ++fpt)
                {
                    elements->push_back(mesh.faceInfo.cornerIndex.at(f).index[0] + mesh.faceInfo.cornerStart[i]);
                    elements->push_back(mesh.faceInfo.cornerIndex.at(f).index[1] + mesh.faceInfo.cornerStart[i]);
                    elements->push_back(mesh.faceInfo.cornerIndex.at(f).index[2] + mesh.faceInfo.cornerStart[i]);

                    ++f;
                }

                // make sure to calculate our bounding boxes
                geometry->setInitialBound(geometry->computeBoundingBox());

                // by default we draw static geometry
                pseudoRoot->addChild(geometry);

                // make sure the skeleton is ready to go if we need it
                skeleton->addChild(rigGeometry);
            }
        }

        vsg::ComputeBoundsVisitor cb;
        cb.apply(*pseudoRoot);

        // both pseudoRoot and skeleton should contain the same geometry information but pseudo root should take less time to traverse
        setInitialBound(cb.getBoundingBox());

        // setup our root bone
        vsg::ref_ptr<osgAnimation::Bone> root = static_cast<osgAnimation::Bone*>(skeleton->getChild(0));
        bones.push_back(root);

        // root->addChild(createRefGeometry(pos, 0.25).get());

        //  hack work around for some meshes having duplicate names
        std::set<std::string> boneNames;

        boneNames.emplace(root->getName());

        // generate our bone heirarchy - start from 1 since we already handled the root bone
        for (uint32_t i = 1; i < d->boneInfos.size(); ++i)
        {
            vsg::ref_ptr<osgAnimation::Bone> bone = new osgAnimation::Bone(d->boneInfos[i].name);
            vsg::ref_ptr<osgAnimation::Bone> parent = bones.at(d->boneInfos[i].parentIndex);

            if (boneNames.count(bone->getName()) != 0)
            {
                const std::string& newName = bone->getName() + "_DUP";
                // log->error("DUPLICATE BONE NAME: {} WE ARE REMAPPING TO {}", bone->getName(), bone->getName() + "_DUP");
                boneNames.emplace(bone->getName() + "_DUP");

                bone->setName(newName);
            }
            else
            {
                boneNames.emplace(bone->getName());
            }

            parent->addChild(bone);
            bones.push_back(bone);
        }

        vsg::Matrix bind = vsg::Matrix::rotate(d->rposInfoRel[0].rotation) * vsg::Matrix::translate(d->rposInfoRel[0].position);
        vsg::Matrix inverseBind = vsg::Matrix::rotate(d->rposInfoAbI[0].rotation) * vsg::Matrix::translate(d->rposInfoAbI[0].position);

        // this is totally wrong, i think
        if (d->boneInfos.size() > 1)
        {
            root->setInvBindMatrixInSkeletonSpace(inverseBind);
        }
        else
        {
            root->setInvBindMatrixInSkeletonSpace(inverseBind * bind);
        }

        // updaters aren't applied unless the GameObject has a [body] component
        vsg::ref_ptr<osgAnimation::UpdateBone> updater = new osgAnimation::UpdateBone(d->boneInfos[0].name);
        updater->getStackedTransforms().push_back(new osgAnimation::StackedTranslateElement("position", d->rposInfoRel[0].position));
        updater->getStackedTransforms().push_back(new osgAnimation::StackedQuaternionElement("rotation", d->rposInfoRel[0].rotation));

        root->setUpdateCallback(updater);

        // lets force the intial position of the bone to be our stacked transform
        // with this we don't require the UpdateBone updaters to run at all
        pseudoRoot->setMatrix(bind);
        root->setMatrix(bind);
        root->setMatrixInSkeletonSpace(bind);

        // iterate again and setup our initial bone positions
        for (uint32_t i = 1; i < d->boneInfos.size(); ++i)
        {
            vsg::ref_ptr<osgAnimation::Bone> bone = bones.at(i);
            vsg::ref_ptr<osgAnimation::Bone> parent = bones.at(d->boneInfos[i].parentIndex);

            bind = vsg::Matrix::rotate(d->rposInfoRel[i].rotation) * vsg::Matrix::translate(d->rposInfoRel[i].position);
            inverseBind = vsg::Matrix::rotate(d->rposInfoAbI[i].rotation) * vsg::Matrix::translate(d->rposInfoAbI[i].position);

            bone->setInvBindMatrixInSkeletonSpace(inverseBind);

            // updaters aren't applied unless the GameObject has a [body] component
            vsg::ref_ptr<osgAnimation::UpdateBone> updater = new osgAnimation::UpdateBone(d->boneInfos[i].name);
            updater->getStackedTransforms().push_back(new osgAnimation::StackedTranslateElement("position", d->rposInfoRel[i].position));
            updater->getStackedTransforms().push_back(new osgAnimation::StackedQuaternionElement("rotation", d->rposInfoRel[i].rotation));
            bone->setUpdateCallback(updater);

            // lets force the intial position of the bone to be our stacked transform
            // with this we don't require the UpdateBone updaters to run at all
            bone->setMatrix(vsg::Matrix(d->rposInfoRel[i].rotation) * vsg::Matrix::translate(d->rposInfoRel[i].position));
            bone->setMatrixInSkeletonSpace(vsg::Matrix(d->rposInfoRel[i].rotation) * vsg::Matrix::translate(d->rposInfoRel[i].position) * parent->getMatrixInSkeletonSpace());
        }
#endif
    }
} // namespace ehb

#if 0
#    include <osg/ComputeBoundsVisitor>
#    include <osg/MatrixTransform>
#    include <osg/PolygonMode>
#    include <osg/Texture2D>
#    include <osgAnimation/StackedQuaternionElement>
#    include <osgAnimation/StackedTranslateElement>
#    include <osgAnimation/UpdateBone>
#    include <osgDB/ReadFile>

namespace ehb
{
    static osgAnimation::VertexInfluence& getVertexInfluence(osgAnimation::VertexInfluenceMap& vim, const std::string& name) {

        osgAnimation::VertexInfluenceMap::iterator it = vim.lower_bound(name);
        if (it == vim.end() || name != it->first) {

            it = vim.insert(it, osgAnimation::VertexInfluenceMap::value_type(name, osgAnimation::VertexInfluence()));
            it->second.setName(name);
        }
        return it->second;
    }

    static vsg::ref_ptr<vsg::Group> createRefGeometry(vsg::Vec3 p, double len)
    {
        vsg::ref_ptr<vsg::Group> group = new vsg::Group;
        {
            vsg::ref_ptr<vsg::Geometry> geometry = new vsg::Geometry;
            vsg::ref_ptr<vsg::Vec3Array> vertices = new vsg::Vec3Array;

            // Joint
            vertices->push_back(vsg::Vec3(-len, 0.0, 0.0));
            vertices->push_back(vsg::Vec3(len, 0.0, 0.0));
            vertices->push_back(vsg::Vec3(0.0, -len, 0.0));
            vertices->push_back(vsg::Vec3(0.0, len, 0.0));
            vertices->push_back(vsg::Vec3(0.0, 0.0, -len));
            vertices->push_back(vsg::Vec3(0.0, 0.0, len));

            // Bone
            vertices->push_back(vsg::Vec3(0.0, 0.0, 0.0));
            vertices->push_back(p);

            geometry->addPrimitiveSet(new vsg::DrawArrays(vsg::PrimitiveSet::LINES, 0, 8));
            geometry->setVertexArray(vertices.get());

            group->addChild(geometry.get());
        }

        return group;
    }

    // https://github.com/xarray/osgRecipes/blob/master/cookbook/chapter8/ch08_07/OctreeBuilder.cpp
    // flipped the parameters for ease of use with ds bboxes
    static vsg::Group* createBoxForDebug(const vsg::Vec3& min, const vsg::Vec3& max)
    {
        vsg::Vec3 dir = max - min;
        vsg::ref_ptr<vsg::Vec3Array> va = new vsg::Vec3Array(10);
        (*va)[0] = min + vsg::Vec3(0.0f, 0.0f, 0.0f);
        (*va)[1] = min + vsg::Vec3(0.0f, 0.0f, dir[2]);
        (*va)[2] = min + vsg::Vec3(dir[0], 0.0f, 0.0f);
        (*va)[3] = min + vsg::Vec3(dir[0], 0.0f, dir[2]);
        (*va)[4] = min + vsg::Vec3(dir[0], dir[1], 0.0f);
        (*va)[5] = min + vsg::Vec3(dir[0], dir[1], dir[2]);
        (*va)[6] = min + vsg::Vec3(0.0f, dir[1], 0.0f);
        (*va)[7] = min + vsg::Vec3(0.0f, dir[1], dir[2]);
        (*va)[8] = min + vsg::Vec3(0.0f, 0.0f, 0.0f);
        (*va)[9] = min + vsg::Vec3(0.0f, 0.0f, dir[2]);

        vsg::ref_ptr<vsg::Geometry> geom = new vsg::Geometry;
        geom->setVertexArray(va.get());
        geom->addPrimitiveSet(new vsg::DrawArrays(GL_QUAD_STRIP, 0, 10));

        vsg::ref_ptr<vsg::Group> group = new vsg::Group;
        group->addChild(geom.get());
        group->getOrCreateStateSet()->setAttribute(new vsg::PolygonMode(vsg::PolygonMode::FRONT_AND_BACK, vsg::PolygonMode::LINE));
        group->getOrCreateStateSet()->setMode(GL_LIGHTING, vsg::StateAttribute::OFF);
        return group.release();
    }

    Aspect::Aspect(std::shared_ptr<Impl> impl) : d(std::move(impl))
    {
        auto log = spdlog::get("log");

        debugDrawingGroups.resize(3);

        skeleton = new osgAnimation::Skeleton;

        // be default meshes aren't rendered using their skeleton so lets mimic a root bone
        pseudoRoot = new vsg::MatrixTransform;
        addChild(pseudoRoot);

        // put the root bone as the first child of skeleton for easy acces
        skeleton->addChild(new osgAnimation::Bone(d->boneInfos[0].name));

        // it looks like the skeleton callback just validates the skeleton and then stays in the callbacks
        // TODO: osgAnimation should expose the ValidateSkeletonVisitor to the API and let you call it once when you need it
        skeleton->setUpdateCallback(nullptr);

        log->debug("asp has {} sub meshes", d->subMeshes.size());

        for (const auto& mesh : d->subMeshes)
        {
            log->debug("asp subMesh has {} textures", mesh.textureCount);

            uint32_t f = 0; // track which face the loader is loading across the sub mesh
            for (uint32_t i = 0; i < mesh.textureCount; ++i)
            {
                // create the geometry which will be drawn
                vsg::ref_ptr<vsg::Geometry> geometry = new vsg::Geometry;

                // create the rigged geometry that is used to do our weighting
                vsg::ref_ptr<osgAnimation::RigGeometry> rigGeometry = new osgAnimation::RigGeometry;
                rigGeometry->setInfluenceMap(new osgAnimation::VertexInfluenceMap);
                rigGeometry->setSourceGeometry(geometry);

                // store our rigGeometry for later use
                rigGeometryVec.push_back(rigGeometry);

                // add the geometry to our aspect for quick lookups
                geometryVec.push_back(geometry);

                // textures are stored directly against the mesh
                // TODO: should we store the vsg::Image against AspectImpl?
                const std::string imageFilename = d->textureNames[i] + ".raw";
                vsg::ref_ptr<vsg::Image> image = osgDB::readImageFile(imageFilename);

                geometry->setName(d->textureNames[i]);

                if (image != nullptr)
                {
                    vsg::Texture2D* texture = new vsg::Texture2D(image);
                    geometry->getOrCreateStateSet()->setTextureAttributeAndModes(0, texture);
                    geometry->getOrCreateStateSet()->setMode(GL_BLEND, vsg::StateAttribute::ON);

                    texture->setWrap(vsg::Texture2D::WRAP_S, vsg::Texture2D::REPEAT);
                    texture->setWrap(vsg::Texture2D::WRAP_T, vsg::Texture2D::REPEAT);

                    log->debug("loaded image {} for asp sub mesh: {}", i, imageFilename);
                }

                vsg::ref_ptr<vsg::Vec3Array> vertices = new vsg::Vec3Array;
                vsg::ref_ptr<vsg::Vec3Array> normals = new vsg::Vec3Array;
                vsg::ref_ptr<vsg::Vec2Array> texCoords = new vsg::Vec2Array;
                vsg::ref_ptr<vsg::Vec4Array> colors = new vsg::Vec4Array;

                geometry->setVertexArray(vertices);
                geometry->setNormalArray(normals, vsg::Array::BIND_PER_VERTEX);
                geometry->setTexCoordArray(0, texCoords);
                geometry->setColorArray(colors, vsg::Array::BIND_PER_VERTEX);

                for (uint32_t cornerCounter = 0; cornerCounter < mesh.cornerCount; ++cornerCounter)
                {
                    const auto& c = mesh.corners[cornerCounter];
                    const auto& w = mesh.wCorners[cornerCounter];

                    vertices->push_back(c.position);
                    texCoords->push_back(c.texCoord);
                    normals->push_back(c.normal);
                    colors->push_back(vsg::Vec4(c.color[0], c.color[1], c.color[2], c.color[3]));

                    osgAnimation::VertexInfluenceMap& influenceMap = *rigGeometry->getInfluenceMap();

                    // There are 4 weights per bone
                    for (uint32_t weight = 0; weight < 4; ++weight)
                    {
                        int32_t boneId = static_cast<int32_t> (w.bone[weight]);
                        float value = static_cast<float> (w.weight[weight]);

                        if (boneId != 0) {

                            osgAnimation::VertexInfluence& vi = getVertexInfluence(influenceMap, d->boneInfos[boneId - 1].name);
                            vi.push_back(osgAnimation::VertexIndexWeight(cornerCounter, value));
                        }
                    }

                }

                vsg::ref_ptr<vsg::DrawElementsUInt> elements = new vsg::DrawElementsUInt(GL_TRIANGLES);
                geometry->addPrimitiveSet(elements);

                for (uint32_t fpt = 0; fpt < mesh.matInfo[i].faceSpan; ++fpt)
                {
                    elements->push_back(mesh.faceInfo.cornerIndex.at(f).index[0] + mesh.faceInfo.cornerStart[i]);
                    elements->push_back(mesh.faceInfo.cornerIndex.at(f).index[1] + mesh.faceInfo.cornerStart[i]);
                    elements->push_back(mesh.faceInfo.cornerIndex.at(f).index[2] + mesh.faceInfo.cornerStart[i]);

                    ++f;
                }

                // make sure to calculate our bounding boxes
                geometry->setInitialBound(geometry->computeBoundingBox());

                // by default we draw static geometry
                pseudoRoot->addChild(geometry);

                // make sure the skeleton is ready to go if we need it
                skeleton->addChild(rigGeometry);
            }
        }

        vsg::ComputeBoundsVisitor cb;
        cb.apply(*pseudoRoot);

        // both pseudoRoot and skeleton should contain the same geometry information but pseudo root should take less time to traverse
        setInitialBound(cb.getBoundingBox());

        // setup our root bone
        vsg::ref_ptr<osgAnimation::Bone> root = static_cast<osgAnimation::Bone*>(skeleton->getChild(0));
        bones.push_back(root);

        // root->addChild(createRefGeometry(pos, 0.25).get());

        //  hack work around for some meshes having duplicate names
        std::set<std::string> boneNames;

        boneNames.emplace(root->getName());

        // generate our bone heirarchy - start from 1 since we already handled the root bone
        for (uint32_t i = 1; i < d->boneInfos.size(); ++i)
        {
            vsg::ref_ptr<osgAnimation::Bone> bone = new osgAnimation::Bone(d->boneInfos[i].name);
            vsg::ref_ptr<osgAnimation::Bone> parent = bones.at(d->boneInfos[i].parentIndex);

            if (boneNames.count(bone->getName()) != 0)
            {
                const std::string& newName = bone->getName() + "_DUP";
                // log->error("DUPLICATE BONE NAME: {} WE ARE REMAPPING TO {}", bone->getName(), bone->getName() + "_DUP");
                boneNames.emplace(bone->getName() + "_DUP");

                bone->setName(newName);
            }
            else
            {
                boneNames.emplace(bone->getName());
            }

            parent->addChild(bone);
            bones.push_back(bone);
        }

        vsg::Matrix bind = vsg::Matrix::rotate(d->rposInfoRel[0].rotation) * vsg::Matrix::translate(d->rposInfoRel[0].position);
        vsg::Matrix inverseBind = vsg::Matrix::rotate(d->rposInfoAbI[0].rotation) * vsg::Matrix::translate(d->rposInfoAbI[0].position);

        // this is totally wrong, i think
        if (d->boneInfos.size() > 1)
        {
            root->setInvBindMatrixInSkeletonSpace(inverseBind);
        }
        else
        {
            root->setInvBindMatrixInSkeletonSpace(inverseBind * bind);
        }

        // updaters aren't applied unless the GameObject has a [body] component
        vsg::ref_ptr<osgAnimation::UpdateBone> updater = new osgAnimation::UpdateBone(d->boneInfos[0].name);
        updater->getStackedTransforms().push_back(new osgAnimation::StackedTranslateElement("position", d->rposInfoRel[0].position));
        updater->getStackedTransforms().push_back(new osgAnimation::StackedQuaternionElement("rotation", d->rposInfoRel[0].rotation));

        root->setUpdateCallback(updater);

        // lets force the intial position of the bone to be our stacked transform
        // with this we don't require the UpdateBone updaters to run at all
        pseudoRoot->setMatrix(bind);
        root->setMatrix(bind);
        root->setMatrixInSkeletonSpace(bind);

        // iterate again and setup our initial bone positions
        for (uint32_t i = 1; i < d->boneInfos.size(); ++i)
        {
            vsg::ref_ptr<osgAnimation::Bone> bone = bones.at(i);
            vsg::ref_ptr<osgAnimation::Bone> parent = bones.at(d->boneInfos[i].parentIndex);

            bind = vsg::Matrix::rotate(d->rposInfoRel[i].rotation) * vsg::Matrix::translate(d->rposInfoRel[i].position);
            inverseBind = vsg::Matrix::rotate(d->rposInfoAbI[i].rotation) * vsg::Matrix::translate(d->rposInfoAbI[i].position);

            bone->setInvBindMatrixInSkeletonSpace(inverseBind);

            // updaters aren't applied unless the GameObject has a [body] component
            vsg::ref_ptr<osgAnimation::UpdateBone> updater = new osgAnimation::UpdateBone(d->boneInfos[i].name);
            updater->getStackedTransforms().push_back(new osgAnimation::StackedTranslateElement("position", d->rposInfoRel[i].position));
            updater->getStackedTransforms().push_back(new osgAnimation::StackedQuaternionElement("rotation", d->rposInfoRel[i].rotation));
            bone->setUpdateCallback(updater);

            // lets force the intial position of the bone to be our stacked transform
            // with this we don't require the UpdateBone updaters to run at all
            bone->setMatrix(vsg::Matrix(d->rposInfoRel[i].rotation) * vsg::Matrix::translate(d->rposInfoRel[i].position));
            bone->setMatrixInSkeletonSpace(vsg::Matrix(d->rposInfoRel[i].rotation) * vsg::Matrix::translate(d->rposInfoRel[i].position) * parent->getMatrixInSkeletonSpace());
        }
    }

    Aspect::Aspect(const Aspect& aspect, const vsg::CopyOp& copyop) : vsg::Group(aspect, copyop)
    {
        // share our implementation details
        d = aspect.d;

        // if this is a shallow copy we are pretty safe to copy over the pointers
        if (copyop.getCopyFlags() & vsg::CopyOp::SHALLOW_COPY)
        {
            bones = aspect.bones;
            rigGeometryVec = aspect.rigGeometryVec;
            geometryVec = aspect.geometryVec;
            skeleton = aspect.skeleton;
            pseudoRoot = aspect.pseudoRoot;
        }
        else if (copyop.getCopyFlags() & vsg::CopyOp::DEEP_COPY_NODES)
        {
            std::function<void(vsg::ref_ptr<osgAnimation::Bone>)> recurse = [&, this](osgAnimation::Bone* bone)
            {
                bones.push_back(bone);

                for (uint32_t i = 0; i < bone->getNumChildren(); ++i)
                {
                    recurse(static_cast<osgAnimation::Bone*>(bone->getChild(i)));
                }
            };

            d = aspect.d;

            // this is normally called from AspectComponent when things are added to the graph so... make some assumptions...

            // deep copy our skeleton which isn't part of the group yet
            skeleton = static_cast<osgAnimation::Skeleton*>(aspect.skeleton->clone(copyop));

            // make sure to update our bone reference vec
            recurse(static_cast<osgAnimation::Bone*>(skeleton->getChild(0)));

            // first child is our pseudo bone which we can grab from this group since it's been cloned
            pseudoRoot = static_cast<vsg::MatrixTransform*>(getChild(0));

            // rebuild our geometry references
            for (uint32_t i = 0; i < pseudoRoot->getNumChildren(); ++i)
            {
                geometryVec.push_back(static_cast<vsg::Geometry*>(pseudoRoot->getChild(i)));
            }
        }
    }

    vsg::Object* Aspect::clone(const vsg::CopyOp& copyop) const
    {
        return new Aspect(*this, copyop);
    }

    vsg::Geometry* Aspect::geometry(unsigned int index)
    {
        return geometryVec[index];
    }

    // Should this be an update callback?
    void Aspect::applySkeleton()
    {
        // should I shallow clone the mesh to prevent anything going out of scope and being destructed?

        // first step is to remove all children from the group
        removeChildren(0, getNumChildren());

        // apply the skeleton as the first child
        addChild(skeleton);
    }

    // Should this be an update callback?
    void Aspect::removeSkeleton()
    {
        // remove our skeleton
        removeChildren(0, getNumChildren());

        // apply our static geometry with root bone transform
        addChild(pseudoRoot);
    }

    void Aspect::toggleBoundingBox()
    {
        if (!drawingBoundingBox)
        {
            if (debugDrawingGroups[1] == nullptr)
            {
                debugDrawingGroups[1] = new vsg::Group;

                vsg::ComputeBoundsVisitor cbv;
                accept(cbv);

                debugDrawingGroups[1]->addChild(createBoxForDebug(cbv.getBoundingBox()._min, cbv.getBoundingBox()._max));
            }

            addChild(debugDrawingGroups[1]);

            drawingBoundingBox = true;

            return;
        }

        if (drawingBoundingBox)
        {
            if (debugDrawingGroups[1] != nullptr)
                removeChild(debugDrawingGroups[1]);

            drawingBoundingBox = false;

            return;
        }
    }
}

#endif