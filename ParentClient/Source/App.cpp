#include <iostream>

#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image.h>
#include <stb_image_write.h>
#include <GLFW/glfw3.h>
#include <glad/gl.h>
#include <rpc_core.h>

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

void CheckCompileErrors(unsigned int shader, const std::string& type) 
{
  int success;
  char infoLog[512];

  if (type != "PROGRAM") 
  {
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) 
    {
      glGetShaderInfoLog(shader, 512, nullptr, infoLog);
      RPC_ERROR("[RENDERER] Shader compliation failed, shader type '{}' info log: {}", type, infoLog);
    }
  }
  else 
  {
    glGetProgramiv(shader, GL_LINK_STATUS, &success);
    if (!success) 
    {
      glGetProgramInfoLog(shader, 512, nullptr, infoLog);
      RPC_ERROR("[RENDERER] Program linking failed, info log: {}", infoLog);
    }
  }
}

int main()
{
  bool appIsRunning = true;

  std::vector<uint8_t> frameData;
  uint32_t frameHeight = 0;
  uint32_t frameWidth = 0;
  int32_t width, height, channels;

  /*
  stbi_uc* data = stbi_load_from_memory(imageData.data(), static_cast<int32_t>(imageData.size()), &width, &height, &channels, 3);
  RPC_ASSERT(data, "[PARENT IMAGE RECIEVE] Failed to load image: {}", stbi_failure_reason());

  size_t expectedSize = widthData * heightData * 3;
  if (width * height * 3 != expectedSize)
  {
    RPC_WARN("[PARENT IMAGE RECIEVE] Incomplete image received, expected {} bytes, got {} bytes", expectedSize, width * height * 3);
    stbi_image_free(data);
    return; /////////////////
  }

  frameData = std::vector<uint8_t>(data, data + width * height * 3);
  frameHeight = height;
  frameWidth = width;
  RPC_INFO("[PARENT IMAGE RECIEVE] Decompressed image: {}x{} {} bytes", width, height, frameData.size());

  stbi_image_free(data);
  */

  glfwInit();
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

  GLFWwindow* window = glfwCreateWindow(1024, 576, "Render JPEG", nullptr, nullptr);
  if (!window) {
    std::cerr << "Failed to create GLFW window\n";
    glfwTerminate();
    return -1;
  }
  glfwMakeContextCurrent(window);

  if (!gladLoadGL(glfwGetProcAddress))
  {
    std::cerr << "Failed to initialize GLAD\n";
    return -1;
  }

  glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

  GLuint texture;
  glGenTextures(1, &texture);
  glBindTexture(GL_TEXTURE_2D, texture);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  uint32_t currentImageWidth = 0;
  uint32_t currentImageHeight = 0;

  float vertices[] = {
    -1.f, -1.f,    0.f, 0.f,
     1.f, -1.f,    1.f, 0.f,
     1.f,  1.f,    1.f, 1.f,
    -1.f,  1.f,    0.f, 1.f
  };
  unsigned int indices[] = {
      0, 1, 2,
      2, 3, 0
  };

  GLuint VAO, VBO, EBO;
  glGenVertexArrays(1, &VAO);
  glGenBuffers(1, &VBO);
  glGenBuffers(1, &EBO);

  glBindVertexArray(VAO);
  glBindBuffer(GL_ARRAY_BUFFER, VBO);
  glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

  // Position attribute
  glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
  glEnableVertexAttribArray(0);
  // TexCoord attribute
  glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
  glEnableVertexAttribArray(1);

  // Build shader
  GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
  glShaderSource(vertexShader, 1, &vertexShaderSource, nullptr);
  glCompileShader(vertexShader);
  CheckCompileErrors(vertexShader, "VERTEX");

  GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
  glShaderSource(fragmentShader, 1, &fragmentShaderSource, nullptr);
  glCompileShader(fragmentShader);
  CheckCompileErrors(fragmentShader, "FRAGMENT");

  GLuint shaderProgram = glCreateProgram();
  glAttachShader(shaderProgram, vertexShader);
  glAttachShader(shaderProgram, fragmentShader);
  glLinkProgram(shaderProgram);
  CheckCompileErrors(shaderProgram, "PROGRAM");

  glDeleteShader(vertexShader);
  glDeleteShader(fragmentShader);

  while (!glfwWindowShouldClose(window))
  {
    glClear(GL_COLOR_BUFFER_BIT);

    {
      glBindTexture(GL_TEXTURE_2D, texture);
      if (frameWidth != currentImageWidth || frameHeight != currentImageHeight)
      {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, frameWidth, frameHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, frameData.data());
        currentImageWidth = frameWidth;
        currentImageHeight = frameHeight;
      }
      else {
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, currentImageWidth, currentImageHeight, GL_RGB, GL_UNSIGNED_BYTE, frameData.data());
      }
    }

    glUseProgram(shaderProgram);
    glBindTexture(GL_TEXTURE_2D, texture);
    glBindVertexArray(VAO);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
    glfwSwapBuffers(window);
    glfwPollEvents();
  }

  glDeleteProgram(shaderProgram);
  glDeleteTextures(1, &texture);

  glDeleteBuffers(1, &VBO);
  glDeleteBuffers(1, &EBO);
  glDeleteVertexArrays(1, &VAO);

  glfwDestroyWindow(window);
  glfwTerminate();

  appIsRunning = false;

  RPC_INFO("[GENERAL] Application Killed");
}
