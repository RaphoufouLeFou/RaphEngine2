#include "resources/model_resource.hpp"
#include <RaphEngine2/graphics/texture_loader.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <glm/gtc/matrix_transform.hpp>
#include <cstring>

#include <RaphEngine2/logger/logger.hpp>
#include "stb_image.h"

namespace raphEngine::resources
{
    namespace
    {
        std::vector<objects::Texture>
        loadMaterialTextures(aiMaterial* mat, aiTextureType type,
                             objects::Texture::TextureType typeName,
                             bool filter)
        {
            std::vector<objects::Texture> textures;
            for (unsigned int i = 0; i < mat->GetTextureCount(type); i++)
            {
                aiString str;
                mat->GetTexture(type, i, &str);

                objects::Texture texture;
                texture.id = graphics::TextureLoader::getInstance()->load_texture_cached(std::string(str.C_Str()), filter);
                texture.type = typeName;
                texture.path = str.C_Str();
                texture.bilinear = filter;
                textures.push_back(texture);
            }
            return textures;
        }

        void processMesh(aiMesh* mesh, const aiScene* scene, bool filter,
                         const glm::mat4& ModelMat,
                         std::vector<SubmeshData>& out)
        {
            SubmeshData data;
            data.local_matrix = ModelMat;
            data.vertices.reserve(mesh->mNumVertices);

            for (unsigned int i = 0; i < mesh->mNumVertices; i++)
            {
                objects::Vertex vertex;
                vertex.position.x = mesh->mVertices[i].x;
                vertex.position.y = mesh->mVertices[i].y;
                vertex.position.z = mesh->mVertices[i].z;

                if (mesh->HasNormals())
                {
                    vertex.normal.x = mesh->mNormals[i].x;
                    vertex.normal.y = mesh->mNormals[i].y;
                    vertex.normal.z = mesh->mNormals[i].z;
                }

                if (mesh->mTextureCoords[0])
                {
                    vertex.tex_coords.x = mesh->mTextureCoords[0][i].x;
                    vertex.tex_coords.y = mesh->mTextureCoords[0][i].y;
                    vertex.tangent.x = mesh->mTangents[i].x;
                    vertex.tangent.y = mesh->mTangents[i].y;
                    vertex.tangent.z = mesh->mTangents[i].z;
                    vertex.bitangent.x = mesh->mBitangents[i].x;
                    vertex.bitangent.y = mesh->mBitangents[i].y;
                    vertex.bitangent.z = mesh->mBitangents[i].z;
                }
                else
                    vertex.tex_coords = glm::vec2(0.0f, 0.0f);

                data.vertices.push_back(vertex);
            }

            for (unsigned int i = 0; i < mesh->mNumFaces; i++)
            {
                aiFace face = mesh->mFaces[i];
                for (unsigned int j = 0; j < face.mNumIndices; j++)
                    data.indices.push_back(face.mIndices[j]);
            }
            aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];

            if (!data.vertices.empty())
            {
                data.bounds_min = data.bounds_max = data.vertices[0].position;
                for (const auto& v : data.vertices)
                {
                    data.bounds_min = glm::min(data.bounds_min, v.position);
                    data.bounds_max = glm::max(data.bounds_max, v.position);
                }
                data.local_sphere_center =
                    (data.bounds_min + data.bounds_max) * 0.5f;
                data.local_sphere_radius =
                    glm::length(data.bounds_max - data.local_sphere_center);
            }

            auto diffuseMaps =
                loadMaterialTextures(material, aiTextureType_DIFFUSE,
                                     objects::Texture::DIFFUSE, filter);
            data.textures.insert(data.textures.end(), diffuseMaps.begin(),
                                 diffuseMaps.end());

            auto specularMaps =
                loadMaterialTextures(material, aiTextureType_SPECULAR,
                                     objects::Texture::SPECULAR, filter);
            data.textures.insert(data.textures.end(), specularMaps.begin(),
                                 specularMaps.end());

            auto normalMaps =
                loadMaterialTextures(material, aiTextureType_NORMALS,
                                     objects::Texture::NORMAL, filter);
            data.textures.insert(data.textures.end(), normalMaps.begin(),
                                 normalMaps.end());

            auto heightMaps =
                loadMaterialTextures(material, aiTextureType_HEIGHT,
                                     objects::Texture::HEIGHT, filter);
            data.textures.insert(data.textures.end(), heightMaps.begin(),
                                 heightMaps.end());

            out.push_back(std::move(data));
        }

        glm::mat4 AiMatToGlm(const aiMatrix4x4& from)
        {
            glm::mat4 to;
            to[0][0] = from.a1;
            to[1][0] = from.a2;
            to[2][0] = from.a3;
            to[3][0] = from.a4;
            to[0][1] = from.b1;
            to[1][1] = from.b2;
            to[2][1] = from.b3;
            to[3][1] = from.b4;
            to[0][2] = from.c1;
            to[1][2] = from.c2;
            to[2][2] = from.c3;
            to[3][2] = from.c4;
            to[0][3] = from.d1;
            to[1][3] = from.d2;
            to[2][3] = from.d3;
            to[3][3] = from.d4;
            return to;
        }

        const glm::mat4 kYupToZup =
            glm::rotate(glm::mat4(1.0f), glm::radians(-90.0f),
                        glm::vec3(-1.0f, 0.0f, 0.0f));

        // FBX's default unit is centimeters; this engine works in meters.
        constexpr float kFbxUnitScale = 0.01f;

        const glm::mat4 kImportCorrection =
            kYupToZup * glm::scale(glm::mat4(1.0f), glm::vec3(kFbxUnitScale));

        void processNode(std::vector<SubmeshData>& out, aiNode* node,
                         const aiScene* scene, bool filter)
        {
            for (unsigned int i = 0; i < node->mNumMeshes; i++)
            {
                aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
                aiMatrix4x4 globalTransform = node->mTransformation;
                aiNode* parent = node->mParent;
                while (parent != nullptr)
                {
                    globalTransform = parent->mTransformation * globalTransform;
                    parent = parent->mParent;
                }
                glm::mat4 m = kImportCorrection * AiMatToGlm(globalTransform);
                processMesh(mesh, scene, filter, m, out);
            }
            for (unsigned int i = 0; i < node->mNumChildren; i++)
                processNode(out, node->mChildren[i], scene, filter);
        }
    } // anonymous namespace

    std::unordered_map<std::string, std::weak_ptr<ModelResource>>
        ModelResource::cache_;

    std::shared_ptr<ModelResource>
    ModelResource::get_or_load(const std::string& path, bool filter)
    {
        if (auto it = cache_.find(path); it != cache_.end())
            if (auto locked = it->second.lock())
            {
                Logger::LogDebug("Reused cached model: ", path);
                return locked;
            }

        auto model =
            std::shared_ptr<ModelResource>(new ModelResource(path, filter));
        cache_[path] = model;
        return model;
    }

    ModelResource::ModelResource(const std::string& path, bool filter)
    {
        Assimp::Importer importer;
        const aiScene* scene = importer.ReadFile(
            path,
            aiProcess_Triangulate | aiProcess_GenSmoothNormals
                | aiProcess_FlipUVs | aiProcess_CalcTangentSpace);

        if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE
            || !scene->mRootNode)
        {
            Logger::LogError("assimp: ", importer.GetErrorString());
            return;
        }

        float unitScale = 1.0f;
        if (scene->mMetaData)
            scene->mMetaData->Get("UnitScaleFactor", unitScale);

        processNode(submeshes_, scene->mRootNode, scene, filter);
    }
} // namespace raphEngine::resources