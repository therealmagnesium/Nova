#include <Nova/Camera/FreeLookCamera.h>
#include <Nova/Camera/OrthographicCamera.h>
#include <Nova/Shader.h>
#include <Nova/VertexArray.h>
#include <Nova/Window.h>

#include <GLFW/glfw3.h>
#include <glad/gl.h>
#include <glm.hpp>
#include <gtc/matrix_transform.hpp>
#include <gtc/type_ptr.hpp>
#include <math.h>

int main(int argc, char* argv[])
{
    Nova::WindowArgs windowArgs = Nova::WindowArgs("Sandbox", 1280, 720);
    Nova::Window* window = Nova::CreateWindow(windowArgs);
    window->SetVSync(true);

    Nova::FreeLookCamera camera({0.f, 0.f, 2.f}, windowArgs.width, windowArgs.height);

    float vertices[] = {
        -0.5f, 0.0f, 0.5f, -0.5f, 0.0f, -0.5f, 0.5f, 0.0f, -0.5f, 0.5f, 0.0f, 0.5f, 0.0f, 0.8f, 0.0f,
    };

    uint32_t indices[] = {
        0, 1, 2, 0, 2, 3, 0, 1, 4, 1, 2, 4, 2, 3, 4, 3, 0, 4,
    };

    Nova::VertexArray vao;
    vao.Bind();

    Nova::VertexBuffer vbo(vertices, sizeof(vertices));
    Nova::IndexBuffer ebo(indices, sizeof(indices) / sizeof(uint32_t));

    vao.LinkVBO(vbo, 0);
    vao.Unbind();

    Nova::Shader shader("assets/basic.glsl");
    shader.Bind();

    int frameCount = 0;
    double previousTime = glfwGetTime();

    while (!Nova::WindowShouldClose(window))
    {
        double currentTime = glfwGetTime();

        window->HandleEvents();
        camera.HandleInputs(window);

        window->Clear(0.2f, 0.3f, 0.7f);

        camera.CalculateViewProjectionMatrix(45.f, 0.1f, 100.f, shader, "uViewProj");
        shader.SetUniform4f("uColor", 0.6f, 0.5, 0.7, 1.f);
        shader.Bind();

        vao.Bind();
        glDrawElements(GL_TRIANGLES, ebo.count, GL_UNSIGNED_INT, NULL);

        window->Display();

        frameCount++;
        if (currentTime - previousTime >= 1.0)
        {
            printf("FPS: %d\n", frameCount);
            frameCount = 0;
            previousTime = currentTime;
        }
    }

    return 0;
}
