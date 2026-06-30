#include "graphics/shadow_renderer.hpp"

#include <RaphEngine2/export.hpp>
#include <memory>

#include "RaphEngine2/component/camera_component.hpp"
#include "RaphEngine2/graphics/ogl/gl_shadow_renderer.hpp"
#include "RaphEngine2/objects/mesh.hpp"
#include "settings/graphics.hpp"
#include "settings/settings.hpp"
#include "graphics/graphic_api.hpp"
#include "RaphEngine2/component/light_component.hpp"
#include "logger/logger.hpp"
#include "utils.hpp"

namespace raphEngine::graphics
{

    std::unique_ptr<ShadowRenderer> ShadowRenderer::instance_ = nullptr;

    std::vector<float> ShadowRenderer::shadowCascadeLevels = { 125.0f, 33.3f,
                                                               10.0f, 3.1f };

    std::shared_ptr<Shader> ShadowRenderer::shadow_shader = nullptr;

    const component::LightComponent* ShadowRenderer::GetDirectionalLight()
    {
        for (const auto& light : GraphicApi::lights_pool)
        {
            if (light->type == component::LightComponent::DIRECTIONAL)
                return light;
        }

        Logger::LogCritical("No directional light!");
        throw std::runtime_error("No directional light!");
        return nullptr;
    }

    ShadowRenderer* ShadowRenderer::getInstance()
    {
        if (instance_ == nullptr)
        {
            if (Settings::Get<GraphicsSettings>().api == "OpenGL")
            {
                instance_ = std::make_unique<GLShadowRenderer>();
                return instance_.get();
            }
            if (Settings::Get<GraphicsSettings>().api == "Vulkan")
            {
                Logger::LogError("Cannot get shadow renderer from Vulkan",
                                 " (Not implemented). ",
                                 "Defaulting to OpenGl");
                instance_ = std::make_unique<GLShadowRenderer>();
                return instance_.get();
            }
            if (Settings::Get<GraphicsSettings>().api == "D3D11")
            {
                Logger::LogError("Cannot get shadow renderer from DirectX 11",
                                 " (Not implemented). ",
                                 "Defaulting to OpenGl");
                instance_ = std::make_unique<GLShadowRenderer>();
                return instance_.get();
            }

            Logger::LogError(
                "Cannot get shadow renderer from an unknown graphics API.",
                " Defaulting to OpenGl");

            instance_ = std::make_unique<GLShadowRenderer>();
        }

        return instance_.get();
    }

    std::vector<glm::vec4>
    ShadowRenderer::getFrustumCornersWorldSpace(const glm::mat4& projview)
    {
        const auto inv = glm::inverse(projview);

        std::vector<glm::vec4> frustumCorners;
        for (unsigned int x = 0; x < 2; ++x)
            for (unsigned int y = 0; y < 2; ++y)
                for (unsigned int z = 0; z < 2; ++z)
                {
                    const glm::vec4 pt = inv
                        * glm::vec4(2.0f * x - 1.0f, 2.0f * y - 1.0f,
                                    2.0f * z - 1.0f, 1.0f);
                    frustumCorners.push_back(pt / pt.w);
                }

        return frustumCorners;
    }

    std::vector<glm::vec4>
    ShadowRenderer::getFrustumCornersWorldSpace(const glm::mat4& proj,
                                                const glm::mat4& view)
    {
        return getFrustumCornersWorldSpace(proj * view);
    }

    glm::mat4 ShadowRenderer::getLightSpaceMatrix(const float nearPlane,
                                                  const float farPlane)
    {
        component::CameraComponent* cam =
            component::CameraComponent::active_camera;

        const auto Proj = glm::perspective(glm::radians(cam->fov),
                                           (float)(GraphicApi::res_x)
                                               / (float)(GraphicApi::res_y),
                                           nearPlane, farPlane);

        const auto corners =
            getFrustumCornersWorldSpace(Proj, cam->view_matrix_);

        glm::vec3 center = glm::vec3(0.0f);
        for (const auto& v : corners)
        {
            center += glm::vec3(v);
        }
        center /= (float)corners.size();

        float radius = 0.0f;
        for (const auto& v : corners)
        {
            float distance = glm::length(glm::vec3(v) - center);
            radius = std::max(radius, distance);
        }

        radius = std::ceil(radius * 16.0f) / 16.0f;

        const float shadowMapRes =
            (float)Settings::Get<GraphicsSettings>().getShadowResolution();
        const float texelSize = (radius * 2.0f) / shadowMapRes;

        glm::mat4 baseLightView =
            glm::lookAt(glm::vec3(0.0f),
                        -Utils::GetDirectionFromRotation(
                            GetDirectionalLight()
                                ->parent_object->get_transform()
                                .get_rotation()),
                        glm::vec3(0.0f, 1.0f, 0.0f));

        glm::vec3 centerLightSpace =
            glm::vec3(baseLightView * glm::vec4(center, 1.0f));

        centerLightSpace.x =
            std::floor(centerLightSpace.x / texelSize) * texelSize;
        centerLightSpace.y =
            std::floor(centerLightSpace.y / texelSize) * texelSize;

        center = glm::vec3(glm::inverse(baseLightView)
                           * glm::vec4(centerLightSpace, 1.0f));

        const auto lightView =
            glm::lookAt(center,
                        center
                            - Utils::GetDirectionFromRotation(
                                GetDirectionalLight()
                                    ->parent_object->get_transform()
                                    .get_rotation()),
                        glm::vec3(0.0f, 1.0f, 0.0f));

        const float minX = -radius;
        const float maxX = radius;
        const float minY = -radius;
        const float maxY = radius;

        constexpr float CASTER_EXTENSION = 500.0f;
        const float minZ = -radius - 2 * CASTER_EXTENSION;
        const float maxZ = radius + CASTER_EXTENSION;

        const glm::mat4 lightProjection =
            glm::ortho(minX, maxX, minY, maxY, minZ, maxZ);

        return lightProjection * lightView;
    }

    std::vector<glm::mat4> ShadowRenderer::getLightSpaceMatrices()
    {
        component::CameraComponent* cam =
            component::CameraComponent::active_camera;

        std::vector<glm::mat4> ret;
        for (size_t i = 0; i < shadowCascadeLevels.size() + 1; ++i)
        {
            if (i == 0)
            {
                ret.push_back(getLightSpaceMatrix(
                    cam->nearPlane, cam->farPlane / shadowCascadeLevels[i]));
            }
            else if (i < shadowCascadeLevels.size())
            {
                ret.push_back(getLightSpaceMatrix(
                    cam->farPlane / shadowCascadeLevels[i - 1],
                    cam->farPlane / shadowCascadeLevels[i]));
            }
            else
            {
                ret.push_back(getLightSpaceMatrix(
                    cam->farPlane / shadowCascadeLevels[i - 1], cam->farPlane));
            }
        }
        return ret;
    }

} // namespace raphEngine::graphics
