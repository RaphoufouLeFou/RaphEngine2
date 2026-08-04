#pragma once

#include <RaphEngine2/export.hpp>

#include "RaphEngine2/objects/mesh.hpp"
#include "RaphEngine2/renderable.hpp"
#include "RaphEngine2/settings/graphics.hpp"
#include "RaphEngine2/component/light_component.hpp"

namespace raphEngine::graphics
{
    class RAPHENGINE_API GraphicApi
    {
    public:
        virtual void Init(const std::string& window_name) = 0;
        virtual void StartFrame() = 0;
        virtual void Render() = 0;
        virtual bool Refresh() = 0;

        static void AddToRenderPool(const Renderable* renderable);
        static void
        AddToLightsPool(const component::LightComponent* light_componentt);
        void
        AddToSpotLightsPool(const component::LightComponent* light_componentt);

        virtual bool IsKeyPressed(int key) const = 0;

        virtual glm::vec2 GetCursorPos() const = 0;
        virtual void SetCursorPos(double x, double y) const = 0;
        virtual bool GetMouseButtonPressed(int button) const = 0;
        virtual void SetMouseVisibility(bool visible) const = 0;
        virtual bool IsWindowFocused() const = 0;

        static const GraphicApi* get_api();

#ifdef EDITOR_BUILD
        virtual void ResizeViewportFramebuffer(int width, int height)
        {
            (void)width;
            (void)height;
        }
        virtual void* GetViewportTexture() const
        {
            return nullptr;
        }
#endif

        static unsigned short res_x;
        static unsigned short res_y;

#ifdef EDITOR_BUILD
        static inline int viewport_res_x = 0;
        static inline int viewport_res_y = 0;

        static inline float viewport_pos_x = 0.0f;
        static inline float viewport_pos_y = 0.0f;

        static inline bool viewport_focused = false;
#endif

        static std::vector<const Renderable*> render_pool;
        static std::vector<const component::LightComponent*> lights_pool;
        static std::vector<const component::LightComponent*> spot_lights_pool;

    protected:
        static GraphicApi* instance_;
    };
} // namespace raphEngine::graphics
