#include "objects/object_mesh.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include <GL/glew.h>
#include <GL/gl.h>
#include <GLFW/glfw3.h>
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <glm/glm.hpp>
#include <iostream>
#include <memory>

#include "objects/mesh.hpp"
#include "stb_image.h"

namespace raphEngine::objects
{

    std::vector<Texture> ObjectMesh::textures_loaded_ = std::vector<Texture>();

    unsigned int TextureFromFile(const char* path, const std::string& directory,
                                 bool filter)
    {
        std::string filename = std::string(path);
        if (directory != "")
            filename = directory + '/' + filename;
        int index = filename.find("assets");
        if (index != -1)
            filename = filename.substr(index);
        unsigned int textureID;
        glGenTextures(1, &textureID);

        int width, height, nrComponents;
        unsigned char* data =
            stbi_load(filename.c_str(), &width, &height, &nrComponents, 0);

        if (data)
        {
            GLenum format = GL_RGBA;
            if (nrComponents == 1)
                format = GL_RED;
            else if (nrComponents == 3)
                format = GL_RGB;
            else if (nrComponents == 4)
                format = GL_RGBA;

            glBindTexture(GL_TEXTURE_2D, textureID);
            glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format,
                         GL_UNSIGNED_BYTE, data);

            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
            if (filter)
            {
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
                                GL_LINEAR_MIPMAP_LINEAR);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER,
                                GL_LINEAR);
            }
            else
            {
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
                                GL_LINEAR_MIPMAP_NEAREST);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER,
                                GL_NEAREST);
            }

            glGenerateMipmap(GL_TEXTURE_2D);

            std::cout << "Texture loaded at path: " << filename.c_str()
                      << std::endl;
            stbi_image_free(data);
        }
        else
        {
            std::cout << "Texture failed to load at path: " << filename.c_str()
                      << std::endl;
            stbi_image_free(data);
        }

        return textureID;
    }

    std::vector<Texture> loadMaterialTextures(aiMaterial* mat,
                                              aiTextureType type,
                                              Texture::TextureType typeName,
                                              bool filter)
    {
        std::vector<Texture> textures;
        for (unsigned int i = 0; i < mat->GetTextureCount(type); i++)
        {
            aiString str;
            mat->GetTexture(type, i, &str);
            // check if texture was loaded before and if so, continue to next
            // iteration: skip loading a new texture
            bool skip = false;
            for (unsigned int j = 0; j < ObjectMesh::textures_loaded_.size();
                 j++)
            {
                if (std::strcmp(ObjectMesh::textures_loaded_[j].path.data(),
                                str.C_Str())
                    == 0)
                {
                    textures.push_back(ObjectMesh::textures_loaded_[j]);
                    skip = true; // a texture with the same filepath has already
                                 // been loaded, continue to next one.
                                 // (optimization)
                    break;
                }
            }
            if (!skip)
            { // if texture hasn't been loaded already, load it
                Texture texture;
                texture.id = TextureFromFile(str.C_Str(), "", filter);
                texture.type = typeName;
                texture.path = str.C_Str();
                textures.push_back(texture);
                ObjectMesh::textures_loaded_.push_back(
                    texture); // store it as texture loaded for entire model, to
                              // ensure we won't unnecessary load duplicate
                              // textures.
            }
        }
        return textures;
    }

    std::unique_ptr<Mesh> processMesh(aiMesh* mesh, const aiScene* scene,
                                      bool filter, const glm::mat4& ModelMat)
    {
        std::unique_ptr<Mesh> final_mesh = std::make_unique<Mesh>();

        std::vector<Vertex>& vertices = final_mesh->get_vertices();
        std::vector<unsigned int>& indices = final_mesh->get_indices();
        std::vector<Texture>& textures = final_mesh->get_textures();

        vertices.reserve(mesh->mNumVertices);

        for (unsigned int i = 0; i < mesh->mNumVertices; i++)
        {
            Vertex vertex;

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

            vertices.push_back(vertex);
        }

        for (unsigned int i = 0; i < mesh->mNumFaces; i++)
        {
            aiFace face = mesh->mFaces[i];
            for (unsigned int j = 0; j < face.mNumIndices; j++)
                indices.push_back(face.mIndices[j]);
        }
        aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];

        std::vector<Texture> diffuseMaps = loadMaterialTextures(
            material, aiTextureType_DIFFUSE, Texture::DIFFUSE, filter);
        textures.insert(textures.end(), diffuseMaps.begin(), diffuseMaps.end());

        std::vector<Texture> specularMaps = loadMaterialTextures(
            material, aiTextureType_SPECULAR, Texture::SPECULAR, filter);
        textures.insert(textures.end(), specularMaps.begin(),
                        specularMaps.end());

        std::vector<Texture> normalMaps = loadMaterialTextures(
            material, aiTextureType_NORMALS, Texture::NORMAL, filter);
        textures.insert(textures.end(), normalMaps.begin(), normalMaps.end());

        std::vector<Texture> heightMaps = loadMaterialTextures(
            material, aiTextureType_HEIGHT, Texture::HEIGHT, filter);
        textures.insert(textures.end(), heightMaps.begin(), heightMaps.end());

        final_mesh->set_model_matrix(ModelMat);

        return final_mesh;
    }

    void processNode(ObjectMesh* object_mesh, aiNode* node,
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

            glm::mat4 model = glm::mat4(1.0f);
            for (int i = 0; i < 4; i++)
                for (int j = 0; j < 4; j++)
                    model[i][j] = globalTransform[i][j];

            object_mesh->add_mesh(processMesh(mesh, scene, filter, model));
        }

        for (unsigned int i = 0; i < node->mNumChildren; i++)
        {
            processNode(object_mesh, node->mChildren[i], scene, filter);
        }
    }

    void loadModel(ObjectMesh* object_mesh, std::string const& path,
                   bool filter)
    {
        Assimp::Importer importer;
        const aiScene* scene = importer.ReadFile(
            path,
            aiProcess_Triangulate | aiProcess_GenSmoothNormals
                | aiProcess_FlipUVs | aiProcess_CalcTangentSpace);

        if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE
            || !scene->mRootNode)
        {
            std::cout << "ERROR::ASSIMP:: " << importer.GetErrorString()
                      << std::endl;
            return;
        }
        std::string directory = path.substr(0, path.find_last_of('/'));

        processNode(object_mesh, scene->mRootNode, scene, filter);
    }

    ObjectMesh::ObjectMesh(const MeshInfo& info)
    {
        shader_ = info.shader;
        loadModel(this, info.mesh_path, info.bilinear);
    }

    void ObjectMesh::add_mesh(std::unique_ptr<Mesh> mesh)
    {
        meshes_.push_back(std::move(mesh));
    }
} // namespace raphEngine::objects
