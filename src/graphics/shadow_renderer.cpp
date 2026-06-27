#include "graphics/shadow_renderer.hpp"

#include <RaphEngine2/export.hpp>
#include <memory>

#include "RaphEngine2/component/camera_component.hpp"
#include "RaphEngine2/graphics/ogl/gl_shadow_renderer.hpp"
#include "RaphEngine2/objects/mesh.hpp"
#include "settings/graphics.hpp"
#include "settings/settings.hpp"
#include "graphics/graphic_api.hpp"
#include "logger/logger.hpp"

namespace raphEngine::graphics
{

    std::unique_ptr<ShadowRenderer> ShadowRenderer::instance_ = nullptr;

    std::vector<float> ShadowRenderer::shadowCascadeLevels = { 500.0f, 100.0f,
                                                               30.0f, 4.0f };

    // TODO: Change it to the light component
    glm::vec3 ShadowRenderer::lightDirGlobal =
        glm::normalize(glm::vec3(1, 1, 1));

    std::shared_ptr<Shader> ShadowRenderer::shadow_shader = nullptr;

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
                // TODO: for later
            }
            if (Settings::Get<GraphicsSettings>().api == "D3D11")
            {
                Logger::LogError("Cannot get shadow renderer from DirectX 11",
                                 " (Not implemented). ",
                                 "Defaulting to OpenGl");
                instance_ = std::make_unique<GLShadowRenderer>();
                return instance_.get();
                // TODO: for later
            }

            Logger::LogError(
                "Cannot get shadow renderer from an unknown grpahics API.",
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
        {
            for (unsigned int y = 0; y < 2; ++y)
            {
                for (unsigned int z = 0; z < 2; ++z)
                {
                    const glm::vec4 pt = inv
                        * glm::vec4(2.0f * x - 1.0f, 2.0f * y - 1.0f,
                                    2.0f * z - 1.0f, 1.0f);
                    frustumCorners.push_back(pt / pt.w);
                }
            }
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

        glm::vec3 center = glm::vec3(0, 0, 0);
        for (const auto& v : corners)
        {
            center += glm::vec3(v);
        }
        center /= corners.size();

        const auto lightView = glm::lookAt(center, center - lightDirGlobal,
                                           glm::vec3(0.0f, 0.0f, 1.0f));

        float minX = std::numeric_limits<float>::max();
        float maxX = std::numeric_limits<float>::lowest();
        float minY = std::numeric_limits<float>::max();
        float maxY = std::numeric_limits<float>::lowest();
        float minZ = std::numeric_limits<float>::max();
        float maxZ = std::numeric_limits<float>::lowest();
        for (const auto& v : corners)
        {
            const auto trf = lightView * v;
            minX = std::min(minX, trf.x);
            maxX = std::max(maxX, trf.x);
            minY = std::min(minY, trf.y);
            maxY = std::max(maxY, trf.y);
            minZ = std::min(minZ, trf.z);
            maxZ = std::max(maxZ, trf.z);
        }

        // Tune this parameter according to the scene
        constexpr float zMult = 10.0f;
        if (minZ < 0)
        {
            minZ *= zMult;
        }
        else
        {
            minZ /= zMult;
        }
        if (maxZ < 0)
        {
            maxZ /= zMult;
        }
        else
        {
            maxZ *= zMult;
        }

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
