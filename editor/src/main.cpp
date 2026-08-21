#include <RaphEngine2/RaphEngine2.hpp>
#include <RaphEngine2/component/camera_component.hpp>
#include <RaphEngine2/component/collider_component.hpp>
#include <RaphEngine2/component/light_component.hpp>
#include <RaphEngine2/component/mesh_component.hpp>
#include <RaphEngine2/graphics/skybox.hpp>
#include <RaphEngine2/logger/logger.hpp>
#include <nlohmann/json.hpp>
#include <glm/glm.hpp>

#include <string>

#include "Camera/camera.hpp"
#include "project_file/project_file.hpp"

using namespace raphEngine;
using namespace raphEngine::objects;

int main(int argc, char* argv[])
{
    Logger::LogInfo("Starting editor...");

    if (argc <= 1)
    {
        Logger::LogError("No project given in argument");
        if (!Project::parse_project_file(std::string(Project::path)))
        {
            Project::store_project_file();
            return 1;
        }
    }
    else if (!Project::parse_project_file(std::string(argv[1])))
    {
        return 1;
    }

    Core::Init(Project::name);

    Camera camera{};

    Core::Run();

    Project::store_project_file();
    return 0;
}
