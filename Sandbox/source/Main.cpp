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

    glm::mat4 model = glm::mat4(1.0f);
    glm::mat4 view = glm::mat4(1.0f);
    glm::mat4 proj = glm::mat4(1.0f);

    view = glm::translate(view, glm::vec3(0.f, -0.5f, -2.f));
    proj = glm::perspective(glm::radians(45.f), (float)windowArgs.width / windowArgs.height, 0.1f, 100.f);

    int frameCount = 0;
    double previousTime = glfwGetTime();

    while (!Nova::WindowShouldClose(window))
    {
        double currentTime = glfwGetTime();

        window->HandleEvents();
        window->Clear(0.2f, 0.3f, 0.7f);

        model = glm::rotate(model, glm::radians(1.f), glm::vec3(0.f, 1.f, 0.f));

        shader.Bind();
        shader.SetUniform4f("uColor", 0.6f, 0.5, 0.7, 1.f);
        shader.SetUniformMatrix4fv("uModel", 1, glm::value_ptr(model));
        shader.SetUniformMatrix4fv("uView", 1, glm::value_ptr(view));
        shader.SetUniformMatrix4fv("uProj", 1, glm::value_ptr(proj));

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
