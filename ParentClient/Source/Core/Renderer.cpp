#include <algorithm>

#include <YKLib.h>

#include <rpc_core.h>

#include "Core/Renderer.h"
#include "Core/Common.h"

namespace rpc
{
  Renderer::Renderer(uint32_t width, uint32_t height, const std::string& name)
  {
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    m_Window = glfwCreateWindow(width, height, name.c_str(), nullptr, nullptr);
    if (!m_Window)
    {
      glfwTerminate();
      YK_ASSERT(false, "[RENDERER] Failed to create GLFW window");
    }
    glfwMakeContextCurrent(m_Window);

    YK_ASSERT(gladLoadGL(glfwGetProcAddress), "[RENDERER] Failed to initialize GLAD");

    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

    glGenTextures(1, &m_CurrentFrameTexture);
    glBindTexture(GL_TEXTURE_2D, m_CurrentFrameTexture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    float vertices[] =
    {
      -1.f, -1.f,    0.f, 0.f,
       1.f, -1.f,    1.f, 0.f,
       1.f,  1.f,    1.f, 1.f,
      -1.f,  1.f,    0.f, 1.f
    };

    unsigned int indices[] =
    {
        0, 1, 2,
        2, 3, 0
    };

    glGenVertexArrays(1, &m_VAO);
    glGenBuffers(1, &m_VBO);
    glGenBuffers(1, &m_EBO);

    glBindVertexArray(m_VAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);

    const char* vertexShaderSource = R"glsl(
#version 330 core
layout (location = 0) in vec2 aPos;
layout (location = 1) in vec2 aTexCoord;
out vec2 TexCoord;
void main() {
    gl_Position = vec4(aPos, 0.0, 1.0);
    TexCoord = aTexCoord;
}
)glsl";

    const char* fragmentShaderSource = R"glsl(
#version 330 core
in vec2 TexCoord;
out vec4 FragColor;
uniform sampler2D tex;
void main() {
    FragColor = texture(tex, TexCoord);
}
)glsl";

    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, nullptr);
    glCompileShader(vertexShader);
    CheckCompileErrors(vertexShader, "VERTEX");

    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, nullptr);
    glCompileShader(fragmentShader);
    CheckCompileErrors(fragmentShader, "FRAGMENT");

    m_ShaderProgram = glCreateProgram();
    glAttachShader(m_ShaderProgram, vertexShader);
    glAttachShader(m_ShaderProgram, fragmentShader);
    glLinkProgram(m_ShaderProgram);
    CheckCompileErrors(m_ShaderProgram, "PROGRAM");

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    glfwSetKeyCallback(m_Window, Renderer::KeyCallback);
  }

  Renderer::~Renderer()
  {
    glDeleteProgram(m_ShaderProgram);
    glDeleteTextures(1, &m_CurrentFrameTexture);

    glDeleteBuffers(1, &m_VBO);
    glDeleteBuffers(1, &m_EBO);
    glDeleteVertexArrays(1, &m_VAO);

    glfwDestroyWindow(m_Window);
    glfwTerminate();
  }

  void Renderer::Clear()
  {
    glClear(GL_COLOR_BUFFER_BIT);
  }

  void Renderer::UpdateTexture()
  {
    glBindTexture(GL_TEXTURE_2D, m_CurrentFrameTexture);

    std::vector<uint8_t> localPixels;
    {
      std::lock_guard<std::mutex> lock(g_framePixelsMutex);
      localPixels = g_framePixelsData;
    }

    std::lock_guard<std::mutex> lock1(g_frameSizeMutex);
    if (g_frameWidth != g_currentFrameWidth || g_frameHeight != g_currentFrameHeight)
    {
      glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, g_frameWidth, g_frameHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, localPixels.data());
      g_currentFrameWidth = g_frameWidth;
      g_currentFrameHeight = g_frameHeight;
    }
    else
    {
      glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, g_currentFrameWidth, g_currentFrameHeight, GL_RGB, GL_UNSIGNED_BYTE, localPixels.data());
    }
  }

  void Renderer::Render()
  {
    glUseProgram(m_ShaderProgram);
    glBindTexture(GL_TEXTURE_2D, m_CurrentFrameTexture);
    glBindVertexArray(m_VAO);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
  }

  void Renderer::Update()
  {
    glfwSwapBuffers(m_Window);
    glfwPollEvents();
  }

  bool Renderer::IsRunning()
  {
    m_Window;
    return !glfwWindowShouldClose(m_Window);
  }

  void Renderer::CheckCompileErrors(unsigned int shader, const std::string& type)
  {
    int success;
    char infoLog[512];

    if (type != "PROGRAM")
    {
      glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
      if (!success)
      {
        glGetShaderInfoLog(shader, 512, nullptr, infoLog);
        YK_ERROR("[RENDERER] Shader compliation failed, shader type '{}' info log: {}", type, infoLog);
      }
    }
    else
    {
      glGetProgramiv(shader, GL_LINK_STATUS, &success);
      if (!success)
      {
        glGetProgramInfoLog(shader, 512, nullptr, infoLog);
        YK_ERROR("[RENDERER] Program linking failed, info log: {}", infoLog);
      }
    }
  }

  void Renderer::KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
  {
    if (key == GLFW_KEY_R && action == GLFW_PRESS)
    {
      g_newFrameQuality = 50;
    }
    if (key == GLFW_KEY_UP && action == GLFW_PRESS)
    {
      g_newFrameQuality -= g_newFrameQuality % 5;
      g_newFrameQuality += 5;
    }
    if (key == GLFW_KEY_DOWN && action == GLFW_PRESS)
    {
      g_newFrameQuality -= g_newFrameQuality % 5;
      g_newFrameQuality -= 5;
    }

    g_newFrameQuality = static_cast<uint32_t>(std::clamp(static_cast<int32_t>(g_newFrameQuality), 1, 100));
  }
}