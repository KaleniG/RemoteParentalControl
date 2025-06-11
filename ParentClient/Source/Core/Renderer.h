#pragma once

#include <cstdint>
#include <string>

#include <GLFW/glfw3.h>
#include <glad/gl.h>

namespace rpc
{
  class Renderer
  {
  public:
    Renderer(uint32_t width, uint32_t height, const std::string& name);
    ~Renderer();

    void Clear();
    void UpdateTexture();
    void Render();
    void Update();

    bool IsRunning();

  private:
    static void CheckCompileErrors(unsigned int shader, const std::string& type);
    static void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);

  private:
    GLFWwindow* m_Window = nullptr;
    GLuint m_CurrentFrameTexture = 0;
    GLuint m_ShaderProgram = 0;
    GLuint m_VAO = 0;
    GLuint m_VBO = 0;
    GLuint m_EBO = 0;
  };
}