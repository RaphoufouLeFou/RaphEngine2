#include "objects/object_mesh.hpp"

#include <memory>

#include <RaphEngine2/logger/logger.hpp>
#include "RaphEngine2/graphics/graphic_api.hpp"
#include "objects/mesh.hpp"
#include "resources/model_resource.hpp"

namespace raphEngine::objects
{
    void loadModel(ObjectMesh* object_mesh, std::string const& path,
                   bool filter)
    {
        auto model = resources::ModelResource::get_or_load(path, filter);

        for (size_t i = 0; i < model->get_submeshes().size(); ++i)
        {
            auto& data = model->get_submeshes()[i];

            auto mesh = std::make_unique<Mesh>();
            mesh->data_ = &data;
            mesh->set_source(model, i);

            object_mesh->add_mesh(std::move(mesh));
        }
    }

    std::shared_ptr<ObjectMesh>
    ObjectMesh::get_or_create(objects::GameObject* parent_object,
                              const MeshInfo& info, graphics::Shader* shader,
                              const bool* cast_shadow, bool* outline)
    {
        ObjectMesh* obj =
            new ObjectMesh{ parent_object, info, shader, cast_shadow, outline };
        return std::shared_ptr<ObjectMesh>(obj);
    }

    ObjectMesh::ObjectMesh(objects::GameObject* parent_object,
                           const MeshInfo& info, graphics::Shader* shader,
                           const bool* cast_shadow, bool* outline)
    {
        this->parent_object = parent_object;
        shader_ = shader;
        cast_shadow_ = cast_shadow;
        outline_ = outline;
        Logger::LogDebug("loading new mesh for ", parent_object->get_name());
        loadModel(this, info.mesh_path.string(), info.bilinear);
    }

    void ObjectMesh::add_mesh(std::unique_ptr<Mesh> mesh)
    {
        mesh->set_shader(shader_);
        mesh->cast_shadows = cast_shadow_;
        mesh->parent_object = this->parent_object;
        mesh->set_outline(outline_);
        mesh->generate_mesh_buffers();
        meshes_.push_back(std::move(mesh));
    }

    void ObjectMesh::render() const
    {
        for (size_t i = 0; i < meshes_.size(); i++)
            graphics::GraphicApi::AddToRenderPool(meshes_[i].get());
    }
} // namespace raphEngine::objects
