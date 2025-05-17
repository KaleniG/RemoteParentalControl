#include <iostream>

#include <asio.hpp>

#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image.h>
#include <stb_image_write.h>

#include <GLFW/glfw3.h>
#include <glad/gl.h>

#include "Core/Debug.h"
#include "Core/Utils.h"

#define BROADCAST_INTERVAL_SECONDS 2

#define COMMUNICATION_PORT 4000
#define DATA_TRANSFER_PORT 5000

#define MAX_MESSAGE_SIZE 128
#define PARENT_ACCESS_REQUEST_MESSAGE "RPC_PARENT_REMOTE_ACTIVATION_REQUEST"
#define PARENT_ACCESS_REQUEST_ACKNOWLEDGEMENT_MESSAGE "RPC_CHILD_REMOTE_ACTIVATION_REQUEST_ACKNOWLEDGEMENT"

#define UDP_PACKET_MAX_SIZE 65536

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

void checkCompileErrors(unsigned int shader, const std::string& type) {
  int success;
  char infoLog[512];
  if (type != "PROGRAM") {
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
      glGetShaderInfoLog(shader, 512, nullptr, infoLog);
      std::cerr << type << " SHADER COMPILATION FAILED\n" << infoLog << "\n";
    }
  }
  else {
    glGetProgramiv(shader, GL_LINK_STATUS, &success);
    if (!success) {
      glGetProgramInfoLog(shader, 512, nullptr, infoLog);
      std::cerr << "PROGRAM LINKING FAILED\n" << infoLog << "\n";
    }
  }
}

int main()
{
  try
  {
    bool appIsRunning = true;

    asio::io_context ioContext;

    std::optional<asio::ip::address_v4> childIP;
    std::mutex childIPMutex;

    std::vector<uint8_t> frameData;
    uint32_t frameHeight = 0;
    uint32_t frameWidth = 0;
    std::mutex frameMutex;

    std::jthread broadcastThread([&]
      {
        asio::ip::udp::socket broadcastSocket(ioContext, asio::ip::udp::endpoint(asio::ip::udp::v4(), COMMUNICATION_PORT));
        asio::ip::udp::endpoint broadcastEndpoint(rpc::GetNetworkBroadcastAddress(ioContext), COMMUNICATION_PORT);
        broadcastSocket.set_option(asio::socket_base::broadcast(true));

        while (appIsRunning)
        {
          broadcastSocket.send_to(asio::buffer(std::string(PARENT_ACCESS_REQUEST_MESSAGE)), broadcastEndpoint);
          RPC_INFO("[PARENT REQUEST BROADCAST] Sent a broadcast to: {}:{}", broadcastEndpoint.address().to_string(), COMMUNICATION_PORT);
          std::this_thread::sleep_for(std::chrono::seconds(BROADCAST_INTERVAL_SECONDS));
        }
      });

    std::jthread childCommunicationThread([&]
      {
        asio::ip::tcp::acceptor childMessageAcceptor(ioContext, asio::ip::tcp::endpoint(asio::ip::tcp::v4(), COMMUNICATION_PORT));
        while (appIsRunning)
        {
          try 
          {
            {
              std::lock_guard<std::mutex> lock(childIPMutex);
              if (childIP.has_value())
                continue;
            }

            asio::ip::tcp::socket childMessageSocket(ioContext);
            childMessageAcceptor.accept(childMessageSocket);

            std::array<char, MAX_MESSAGE_SIZE> messageBuffer;

            asio::error_code errorCode;
            size_t length = childMessageSocket.read_some(asio::buffer(messageBuffer), errorCode);

            if (!errorCode) 
            {
              std::string message(messageBuffer.data(), length);

              asio::ip::tcp::endpoint sender = childMessageSocket.remote_endpoint();
              if (message.compare(0, std::strlen(PARENT_ACCESS_REQUEST_ACKNOWLEDGEMENT_MESSAGE), PARENT_ACCESS_REQUEST_ACKNOWLEDGEMENT_MESSAGE) == 0)
              {
                RPC_INFO("[PARENT MESSAGE HANDLER] Recieved an access request acknowledgement from: {}:{}", sender.address().to_string(), COMMUNICATION_PORT);

                std::lock_guard<std::mutex> lock(childIPMutex);
                childIP.emplace(sender.address().to_v4());
                RPC_INFO("[PARENT MESSAGE HANDLER] Estabilished a connection with: {}:{}", sender.address().to_string(), COMMUNICATION_PORT);
              }
              else
              {
                RPC_ERROR("[PARENT MESSAGE HANDLER] Recieved an unknown message '{}' from: {}:{}", message, sender.address().to_string(), COMMUNICATION_PORT);
              }
            }
            else
            {
              RPC_ERROR("[PARENT MESSAGE HANDLER] TCP packet reception error: {}", errorCode.message());
            }

            childMessageSocket.shutdown(asio::ip::tcp::socket::shutdown_both, errorCode);
            childMessageSocket.close(errorCode);
          }
          catch (std::exception& e) 
          {
            RPC_ERROR("[PARENT MESSAGE HANDLER] TCP packet reception error: {}", e.what());
          }
        }
      });

    std::jthread childFrameCaptureThread([&] 
      {
        asio::ip::udp::socket recieveSocket(ioContext, asio::ip::udp::endpoint(asio::ip::udp::v4(), DATA_TRANSFER_PORT));
        recieveSocket.non_blocking(true);
        std::vector<uint8_t> recvBuffer(UDP_PACKET_MAX_SIZE);

        while (appIsRunning)
        {
          {
            std::lock_guard<std::mutex> lock(childIPMutex);
            if (!childIP.has_value())
              continue;
          }
          
          asio::error_code errorCode;
          asio::ip::udp::endpoint senderEndpoint;
          size_t length = recieveSocket.receive_from(asio::buffer(recvBuffer), senderEndpoint, 0, errorCode);

          if (errorCode == asio::error::would_block || errorCode == asio::error::try_again)
            continue;
          if (errorCode) 
          {
            RPC_ERROR("Receive error: {}", errorCode.message());
            continue;
          }

          if (length < 12) 
          {
            RPC_ERROR("[PARENT IMAGE RECEIVE] Received packet too small: {} bytes", length);
            continue;
          }

          uint32_t widthData = (recvBuffer[0] << 24) | (recvBuffer[1] << 16) | (recvBuffer[2] << 8) | recvBuffer[3];
          uint32_t heightData = (recvBuffer[4] << 24) | (recvBuffer[5] << 16) | (recvBuffer[6] << 8) | recvBuffer[7];
          uint32_t imageSizeData = (recvBuffer[8] << 24) | (recvBuffer[9] << 16) | (recvBuffer[10] << 8) | recvBuffer[11];

          if (length < 12 + imageSizeData) {
            RPC_WARN("[PARENT IMAGE RECEIVE] Truncated image payload");
            continue;
          }
          RPC_WARN("[PARENT IMAGE RECIEVE] Recieved an image of {} bytes", imageSizeData + 12);
          std::vector<uint8_t> imageData(recvBuffer.begin() + 12, recvBuffer.begin() + 12 + imageSizeData);

          int32_t width, height, channels;
          stbi_uc* data = stbi_load_from_memory(imageData.data(), imageData.size(), &width, &height, &channels, 3);
          RPC_ASSERT(data, "[PARENT IMAGE RECIEVE] Failed to load image: {}", stbi_failure_reason());

          size_t expectedSize = widthData * heightData * 3;
          if (width * height * 3 != expectedSize)
          {
            RPC_WARN("[PARENT IMAGE RECIEVE] Incomplete image received, expected {} bytes, got {} bytes", expectedSize, width * height * 3);
            stbi_image_free(data);
            continue;
          }

          {
            std::lock_guard<std::mutex> lock(frameMutex);
            frameData = std::vector<uint8_t>(data, data + width * height * 3);
            frameHeight = height;
            frameWidth = width;
            RPC_INFO("[PARENT IMAGE RECIEVE] Decompressed image: {}x{} {} bytes", width, height, frameData.size());
          }

          stbi_image_free(data);
        }
      });
    
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
    checkCompileErrors(vertexShader, "VERTEX");

    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, nullptr);
    glCompileShader(fragmentShader);
    checkCompileErrors(fragmentShader, "FRAGMENT");

    GLuint shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);
    checkCompileErrors(shaderProgram, "PROGRAM");

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    while (!glfwWindowShouldClose(window))
    {
      glClear(GL_COLOR_BUFFER_BIT);
      /*
      {
        std::lock_guard<std::mutex> lock(childIPMutex);
        if (!childIP.has_value())
        {
          glfwSwapBuffers(window);
          glfwPollEvents();
          continue;
        }
      }
      */
      {
        std::lock_guard<std::mutex> lock(frameMutex);
        if (frameWidth <= 0 || frameHeight <= 0 || frameData.empty()) {
          glfwSwapBuffers(window);
          glfwPollEvents();
          continue;
        }

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
  catch (std::exception& e)
  {
    RPC_ASSERT(false, "[GENERAL] Caught exception: {}", e.what());
  }
}
