#include <RaphEngine2/graphics/ogl/gl_skybox.hpp>


#define GLM_ENABLE_EXPERIMENTAL
#include <GL/glew.h>
#include <GL/gl.h>
#include <GLFW/glfw3.h>
#include <RaphEngine2/export.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/quaternion.hpp>
#include <memory>

#include <RaphEngine2/export.hpp>
#include <vector>
#include <string>

namespace raphEngine::graphics::ogl
{

    void GL_Skybox::set_faces(const std::vector<std::string>& faces)
    {
        (void) faces;
        // TODO
    }

    void GL_Skybox::render()
    {
        // TODO
    }
}