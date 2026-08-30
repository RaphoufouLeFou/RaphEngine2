#include "resources/model_resource.hpp"
#include <RaphEngine2/graphics/texture_loader.hpp>

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/material.h>
#include <assimp/GltfMaterial.h>
#include <assimp/scene.h>
#include <glm/gtc/matrix_transform.hpp>
#include <cstdlib>
#include <cstring>

#include <RaphEngine2/logger/logger.hpp>

namespace raphEngine::resources
{
    namespace
    {
        graphics::TextureLoader::RawTexture
        loadEmbeddedRaw(const aiTexture* embedded)
        {
            graphics::TextureLoader::RawTexture raw{};

            if (embedded->mHeight == 0)
            {
                const auto* bytes =
                    reinterpret_cast<const unsigned char*>(embedded->pcData);
                raw =
                    graphics::TextureLoader::getInstance()
                        ->load_texture_raw_from_memory(bytes, embedded->mWidth);
            }
            else
            {
                int w = static_cast<int>(embedded->mWidth);
                int h = static_cast<int>(embedded->mHeight);
                auto* rgba = static_cast<unsigned char*>(
                    std::malloc(static_cast<size_t>(w) * h * 4));
                if (rgba)
                {
                    const aiTexel* texels = embedded->pcData;
                    for (int i = 0; i < w * h; i++)
                    {
                        rgba[i * 4 + 0] = texels[i].r;
                        rgba[i * 4 + 1] = texels[i].g;
                        rgba[i * 4 + 2] = texels[i].b;
                        rgba[i * 4 + 3] = texels[i].a;
                    }
                }
                raw.data = rgba;
                raw.width = w;
                raw.height = h;
                raw.nrChannels = 4;
            }

            return raw;
        }

        void readPbrMaterialProperties(aiMaterial* mat, SubmeshData& data)
        {
            // Every read is gated on AI_SUCCESS so an older/non-PBR material
            // (typical FBX) just keeps SubmeshData's sane defaults.
            float metallic = data.metallic_factor;
            if (mat->Get(AI_MATKEY_METALLIC_FACTOR, metallic) == AI_SUCCESS)
                data.metallic_factor = metallic;

            float roughness = data.roughness_factor;
            if (mat->Get(AI_MATKEY_ROUGHNESS_FACTOR, roughness) == AI_SUCCESS)
                data.roughness_factor = roughness;

            aiColor3D emissive(0.0f, 0.0f, 0.0f);
            if (mat->Get(AI_MATKEY_COLOR_EMISSIVE, emissive) == AI_SUCCESS)
                data.emissive_factor =
                    glm::vec3(emissive.r, emissive.g, emissive.b);

            aiString alphaMode;
            if (mat->Get(AI_MATKEY_GLTF_ALPHAMODE, alphaMode) == AI_SUCCESS)
                data.alpha_mask = (std::strcmp(alphaMode.C_Str(), "MASK") == 0);

            float cutoff = data.alpha_cutoff;
            if (mat->Get(AI_MATKEY_GLTF_ALPHACUTOFF, cutoff) == AI_SUCCESS)
                data.alpha_cutoff = cutoff;
        }

        std::vector<objects::Texture>
        loadMaterialTextures(const fs::path& model_path, const aiScene* scene,
                             aiMaterial* mat, aiTextureType type,
                             objects::Texture::TextureType typeName,
                             bool filter)
        {
            std::vector<objects::Texture> textures;
            for (unsigned int i = 0; i < mat->GetTextureCount(type); i++)
            {
                aiString str;
                if (mat->GetTexture(type, i, &str) != AI_SUCCESS)
                {
                    Logger::LogWarning("Failed to retrieve texture path ",
                                       "(type ", (int)type, ", index ", i, ")");
                    continue;
                }

                const char* raw_path = str.C_Str();
                if (raw_path == nullptr || raw_path[0] == '\0')
                {
                    Logger::LogWarning("Texture entry has an empty path ",
                                       "(type ", (int)type, ", index ", i,
                                       ") — skipping");
                    continue;
                }

                objects::Texture texture;
                texture.type = typeName;
                texture.bilinear = filter;

                if (const aiTexture* embedded =
                        scene->GetEmbeddedTexture(raw_path))
                {
                    std::string cache_key =
                        model_path.string() + "#" + raw_path;

                    auto raw = loadEmbeddedRaw(embedded);
                    texture.id =
                        graphics::TextureLoader::getInstance()
                            ->upload_texture_cached(cache_key, raw, filter);

                    if (raw.data)
                    {
                        graphics::TextureLoader::getInstance()->free_raw(raw);
                        Logger::LogDebug("Embedded texture loaded: ",
                                         cache_key);
                    }
                    else
                    {
                        Logger::LogWarning(
                            "Embedded texture failed to decode: ", cache_key);
                    }
                    texture.path = cache_key;
                }
                else
                {
                    texture.id = graphics::TextureLoader::getInstance()
                                     ->load_texture_cached(
                                         std::string(raw_path), filter);
                    texture.path = raw_path;
                }

                textures.push_back(texture);
            }
            return textures;
        }

        std::vector<objects::Texture> loadMaterialTexturesWithFallback(
            const fs::path& model_path, const aiScene* scene, aiMaterial* mat,
            aiTextureType primary, aiTextureType fallback,
            objects::Texture::TextureType typeName, bool filter)
        {
            auto textures = loadMaterialTextures(model_path, scene, mat,
                                                 primary, typeName, filter);
            if (textures.empty())
                textures = loadMaterialTextures(model_path, scene, mat,
                                                fallback, typeName, filter);
            return textures;
        }

        void logMaterialTextureTypes(aiMaterial* mat)
        {
            static const std::pair<aiTextureType, const char*> kTypes[] = {
                { aiTextureType_DIFFUSE, "DIFFUSE" },
                { aiTextureType_BASE_COLOR, "BASE_COLOR" },
                { aiTextureType_SPECULAR, "SPECULAR" },
                { aiTextureType_NORMALS, "NORMALS" },
                { aiTextureType_HEIGHT, "HEIGHT" },
                { aiTextureType_METALNESS, "METALNESS" },
                { aiTextureType_DIFFUSE_ROUGHNESS, "DIFFUSE_ROUGHNESS" },
                { aiTextureType_EMISSIVE, "EMISSIVE" },
                { aiTextureType_AMBIENT_OCCLUSION, "AMBIENT_OCCLUSION" },
                { aiTextureType_OPACITY, "OPACITY" },
            };
            for (const auto& [type, name] : kTypes)
            {
                unsigned int count = mat->GetTextureCount(type);
                if (count > 0)
                    Logger::LogDebug("Material provides ", count, " ", name,
                                     " texture(s)");
            }
        }

        void processMesh(const fs::path& model_path, aiMesh* mesh,
                         const aiScene* scene, bool filter,
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
            logMaterialTextureTypes(material);

            readPbrMaterialProperties(material, data);

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

            auto diffuseMaps = loadMaterialTexturesWithFallback(
                model_path, scene, material, aiTextureType_DIFFUSE,
                aiTextureType_BASE_COLOR, objects::Texture::DIFFUSE, filter);
            data.textures.insert(data.textures.end(), diffuseMaps.begin(),
                                 diffuseMaps.end());

            auto specularMaps = loadMaterialTextures(
                model_path, scene, material, aiTextureType_SPECULAR,
                objects::Texture::SPECULAR, filter);
            data.textures.insert(data.textures.end(), specularMaps.begin(),
                                 specularMaps.end());

            auto normalMaps = loadMaterialTextures(
                model_path, scene, material, aiTextureType_NORMALS,
                objects::Texture::NORMAL, filter);
            data.textures.insert(data.textures.end(), normalMaps.begin(),
                                 normalMaps.end());

            auto heightMaps = loadMaterialTextures(
                model_path, scene, material, aiTextureType_HEIGHT,
                objects::Texture::HEIGHT, filter);
            data.textures.insert(data.textures.end(), heightMaps.begin(),
                                 heightMaps.end());

            auto metallicMaps = loadMaterialTextures(
                model_path, scene, material, aiTextureType_METALNESS,
                objects::Texture::METALLIC, filter);
            data.textures.insert(data.textures.end(), metallicMaps.begin(),
                                 metallicMaps.end());

            auto roughnessMaps = loadMaterialTextures(
                model_path, scene, material, aiTextureType_DIFFUSE_ROUGHNESS,
                objects::Texture::ROUGHNESS, filter);
            data.textures.insert(data.textures.end(), roughnessMaps.begin(),
                                 roughnessMaps.end());

            if (!metallicMaps.empty() && !roughnessMaps.empty()
                && metallicMaps[0].path == roughnessMaps[0].path)
            {
                data.metallic_roughness_packed = true;
            }

            auto aoMaps = loadMaterialTextures(model_path, scene, material,
                                               aiTextureType_AMBIENT_OCCLUSION,
                                               objects::Texture::AO, filter);
            data.textures.insert(data.textures.end(), aoMaps.begin(),
                                 aoMaps.end());

            auto emissiveMaps = loadMaterialTextures(
                model_path, scene, material, aiTextureType_EMISSIVE,
                objects::Texture::EMISSIVE, filter);
            data.textures.insert(data.textures.end(), emissiveMaps.begin(),
                                 emissiveMaps.end());

            if (!emissiveMaps.empty()
                && data.emissive_factor == glm::vec3(0.0f))
            {
                data.emissive_factor = glm::vec3(1.0f);
            }

            auto opacityMaps = loadMaterialTextures(
                model_path, scene, material, aiTextureType_OPACITY,
                objects::Texture::OPACITY, filter);
            data.textures.insert(data.textures.end(), opacityMaps.begin(),
                                 opacityMaps.end());

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

        glm::mat4 computeImportCorrection(const aiScene* scene)
        {
            float relative_to_cm = 100.0f; // default: file is already meters
            if (scene->mMetaData)
                scene->mMetaData->Get("UnitScaleFactor", relative_to_cm);

            if (relative_to_cm <= 0.0f)
            {
                Logger::LogWarning(
                    "Invalid UnitScaleFactor (", relative_to_cm,
                    ") in model metadata — ignoring, assuming meters.");
                relative_to_cm = 100.0f;
            }

            const float to_meters = relative_to_cm * 0.01f;
            Logger::LogDebug("Model unit-to-meter scale: ", to_meters);

            return kYupToZup
                * glm::scale(glm::mat4(1.0f), glm::vec3(to_meters));
        }

        void processNode(const fs::path& model_path,
                         std::vector<SubmeshData>& out, aiNode* node,
                         const aiScene* scene, bool filter,
                         const glm::mat4& import_correction)
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
                glm::mat4 m = import_correction * AiMatToGlm(globalTransform);
                processMesh(model_path, mesh, scene, filter, m, out);
            }
            for (unsigned int i = 0; i < node->mNumChildren; i++)
                processNode(model_path, out, node->mChildren[i], scene, filter,
                            import_correction);
        }

    } // anonymous namespace

    std::unordered_map<fs::path, std::weak_ptr<ModelResource>>
        ModelResource::cache_;

    std::shared_ptr<ModelResource>
    ModelResource::get_or_load(const fs::path& path, bool filter)
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

    ModelResource::ModelResource(const fs::path& path, bool filter)
    {
        Assimp::Importer importer;
        const aiScene* scene = importer.ReadFile(
            path.string(),
            aiProcess_Triangulate | aiProcess_GenSmoothNormals
                | aiProcess_FlipUVs | aiProcess_CalcTangentSpace);

        if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE
            || !scene->mRootNode)
        {
            Logger::LogError("assimp: ", importer.GetErrorString(),
                             " for file named ", path);
            return;
        }

        const glm::mat4 import_correction = computeImportCorrection(scene);

        processNode(path, submeshes_, scene->mRootNode, scene, filter,
                    import_correction);
    }
} // namespace raphEngine::resources
