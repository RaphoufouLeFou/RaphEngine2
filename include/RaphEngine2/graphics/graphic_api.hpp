#pragma once

#include <RaphEngine2/export.hpp>

#include "RaphEngine2/renderable.hpp"
#include "RaphEngine2/settings/graphics.hpp"
#include "RaphEngine2/objects/mesh.hpp"

namespace raphEngine::graphics
{
    class RAPHENGINE_API GraphicApi
    {
    public:
        virtual void Init(const settings::Graphics& graphics_settings,
                          const std::string& window_name) = 0;
        virtual void Render() = 0;
        static void AddToRenderPool(const Renderable* renderable);
        virtual bool Refresh() = 0;
        
        virtual bool IsKeyPressed(int key) const = 0;
        
        virtual glm::vec2 GetCursorPos() const = 0;
        virtual void SetCursorPos(double x, double y) const = 0;
        virtual bool GetMouseButtonPressed(int button) const = 0;
        virtual void SetMouseVisibility(bool visible) const = 0;
        virtual bool IsWindowFocused() const = 0;

        static const GraphicApi* get_api();

        static unsigned short res_x;
        static unsigned short res_y;

        static std::vector<const Renderable*> render_pool;
    
    protected:
        
        static GraphicApi* instance_;
    };
} // namespace raphEngine::graphics
