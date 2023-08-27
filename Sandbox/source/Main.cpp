#include <Nova/Shader.h>
#include <Nova/VertexBuffer.h>
#include <Nova/Window.h>
#include <math.h>

int main(int argc, char* argv[])
{
    Nova::Window window(1280, 720, "Nova Engine");
    window.clearColor[0] = 0.79f;
    window.clearColor[1] = 0.67f;
    window.clearColor[2] = 0.47f;

    float vertices[] = {-0.5f, -0.5f, 0.0f, 0.5f, -0.5f, 0.0f, 0.0f, 0.5f, 0.0f};

    uint32_t vao;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    Nova::VertexBuffer vbo(vertices, sizeof(vertices));
    Nova::SetVertexAttributesPointers(0, 3, 0);

    Nova::Shader shader("assets/basic.glsl");
    shader.Bind();

    int frameCount = 0;
    double previousTime = glfwGetTime();
    float greenVal = 0.f;

    while (!Nova::WindowShouldClose(&window))
    {
        window.HandleEvents();
        window.Clear();

        double currentTime = glfwGetTime();

        frameCount++;
        if (currentTime - previousTime >= 1.0)
        {
            printf("%d\n", frameCount);
            frameCount = 0;
            previousTime = currentTime;
        }

        greenVal = (sin(currentTime) / 2.f) + 0.5f;

        shader.Bind();
        shader.SetUniform4f("uColor", 0.3f, greenVal, 0.6, 1.f);

        glBindVertexArray(vao);
        glDrawArrays(GL_TRIANGLES, 0, 3);

        window.Display();
    }

    return 0;
}
