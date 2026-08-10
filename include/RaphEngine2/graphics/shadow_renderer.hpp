#pragma once

#include <memory>

#include "RaphEngine2/graphics/shader.hpp"
#include "RaphEngine2/export.hpp"
#include "RaphEngine2/objects/mesh.hpp"
#include "RaphEngine2/component/light_component.hpp"

namespace raphEngine::objects
{
    class Mesh;
}

namespace raphEngine::graphics
{

    class RAPHENGINE_API ShadowRenderer
    {
    public:
        virtual void
        render_shadows(const raphEngine::objects::Mesh* mesh) const = 0;
        virtual void render_shadows_instanced(
            const std::vector<const raphEngine::objects::Mesh*>& meshes)
            const = 0;

        static ShadowRenderer* getInstance();
        static const component::LightComponent* GetDirectionalLight();

        static std::vector<float> shadowCascadeLevels;

    protected:
        static std::vector<glm::vec4>
        getFrustumCornersWorldSpace(const glm::mat4& projview);

        static std::vector<glm::vec4>
        getFrustumCornersWorldSpace(const glm::mat4& proj,
                                    const glm::mat4& view);

        static glm::mat4 getLightSpaceMatrix(const float nearPlane,
                                             const float farPlane);

        static std::vector<glm::mat4> getLightSpaceMatrices();

        static std::shared_ptr<Shader> shadow_shader;
        static std::shared_ptr<Shader> shadow_shader_instanced;

    private:
        static std::unique_ptr<ShadowRenderer> instance_;
    };
} // namespace raphEngine::graphics
