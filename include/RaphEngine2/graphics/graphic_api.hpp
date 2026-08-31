#pragma once

#include <RaphEngine2/export.hpp>

#include "RaphEngine2/renderable.hpp"
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
        virtual void RequestQuit() const = 0;
        virtual void ShowStartupScreen() const = 0;

        static const GraphicApi* get_api();

        virtual void ResizeViewportFramebuffer(int width, int height)
        {
            (void)width;
            (void)height;
        }
        virtual void* GetViewportTexture() const
        {
            return nullptr;
        }

        static unsigned short viewport_res_x;
        static unsigned short viewport_res_y;

        static inline int window_res_x = 0;
        static inline int window_res_y = 0;
        static inline float viewport_pos_x = 0.0f;
        static inline float viewport_pos_y = 0.0f;

        static inline bool viewport_focused = false;

        static std::vector<const Renderable*> render_pool;
        static std::vector<const component::LightComponent*> lights_pool;
        static std::vector<const component::LightComponent*> spot_lights_pool;

    protected:
        static GraphicApi* instance_;
    };
} // namespace raphEngine::graphics
