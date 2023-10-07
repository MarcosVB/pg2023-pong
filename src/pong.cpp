#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>

void framebuffer_size_callback(GLFWwindow *window, int width, int height);
void processInput(GLFWwindow *window);

// settings
const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;

const char *vertexShaderSource = "#version 330 core\n"
                                 "layout (location = 0) in vec3 aPos;\n"
                                 "void main()\n"
                                 "{\n"
                                 "   gl_Position = vec4(aPos.x, aPos.y, aPos.z, 1.0);\n"
                                 "}\0";
const char *fragmentShaderSource = "#version 330 core\n"
                                   "out vec4 FragColor;\n"
                                   "void main()\n"
                                   "{\n"
                                   "   FragColor = vec4(1.0f, 0.5f, 0.2f, 1.0f);\n"
                                   "}\n\0";

const float moveSpeed = 0.01f;      // Speed of movement
const float rectangleHeight = 0.2f; // Height of the rectangle
float leftRectangleYOffset = 0.0f;  // Variable to hold the Y-axis offset for the left rectangle
float rightRectangleYOffset = 0.0f; // Variable to hold the Y-axis offset for the right rectangle

int main()
{
    // glfw: initialize and configure
    // ------------------------------
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    // glfw window creation
    // --------------------
    GLFWwindow *window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Pong", NULL, NULL);
    if (window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    // glad: load all OpenGL function pointers
    // ---------------------------------------
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    // build and compile our shader program
    // ------------------------------------
    // vertex shader
    unsigned int vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
    glCompileShader(vertexShader);
    // check for shader compile errors
    int success;
    char infoLog[512];
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n"
                  << infoLog << std::endl;
    }
    // fragment shader
    unsigned int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
    glCompileShader(fragmentShader);
    // check for shader compile errors
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n"
                  << infoLog << std::endl;
    }
    // link shaders
    unsigned int shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);
    // check for linking errors
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success)
    {
        glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n"
                  << infoLog << std::endl;
    }
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    // set up vertex data (and buffer(s)) and configure vertex attributes
    // ------------------------------------------------------------------
    // Rectangle vertices
    float rectangleVertices[] = {
        // First rectangle (two triangles)
        -0.85f, 0.1f, 0.0f,
        -0.8f, 0.1f, 0.0f,
        -0.8f, -0.1f, 0.0f,

        -0.8f, -0.1f, 0.0f,
        -0.85f, -0.1f, 0.0f,
        -0.85f, 0.1f, 0.0f,

        // Second rectangle (two triangles)
        0.8f, 0.1f, 0.0f,
        0.85f, 0.1f, 0.0f,
        0.85f, -0.1f, 0.0f,

        0.85f, -0.1f, 0.0f,
        0.8f, -0.1f, 0.0f,
        0.8f, 0.1f, 0.0f};

    // Initialize VBO and VAO
    unsigned int rectangleVBO, rectangleVAO;
    glGenVertexArrays(1, &rectangleVAO);
    glGenBuffers(1, &rectangleVBO);

    // Bind and set VBO and VAO
    glBindVertexArray(rectangleVAO);

    glBindBuffer(GL_ARRAY_BUFFER, rectangleVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(rectangleVertices), rectangleVertices, GL_STATIC_DRAW);

    // Position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void *)0);
    glEnableVertexAttribArray(0);

    // Unbind
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    // Ball vertices
    float ballSize = 0.025f; // Adjust as needed
    float ballVertices[] = {
        -ballSize, ballSize, 0.0f, // Top-left
        ballSize, ballSize, 0.0f,  // Top-right
        ballSize, -ballSize, 0.0f, // Bottom-right

        ballSize, -ballSize, 0.0f,  // Bottom-right
        -ballSize, -ballSize, 0.0f, // Bottom-left
        -ballSize, ballSize, 0.0f   // Top-left
    };

    unsigned int ballVBO, ballVAO;
    glGenVertexArrays(1, &ballVAO);
    glGenBuffers(1, &ballVBO);

    glBindVertexArray(ballVAO);

    glBindBuffer(GL_ARRAY_BUFFER, ballVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(ballVertices), ballVertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void *)0);
    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    // render loop
    // -----------
    while (!glfwWindowShouldClose(window))
    {
        // input
        processInput(window);

        // render
        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        // Update vertex data for both rectangles based on their Y-axis offsets
        float updatedRectangleVertices[36];
        for (int i = 0; i < 36; ++i)
        {
            updatedRectangleVertices[i] = rectangleVertices[i];
        }
        // Update Y-coordinates for both triangles of the left rectangle
        for (int i = 1; i < 18; i += 3) // Go up to index 16 to update all vertices of the left rectangle
        {
            updatedRectangleVertices[i] += leftRectangleYOffset;
        }

        // Update Y-coordinates for both triangles of the right rectangle
        for (int i = 19; i < 36; i += 3) // Go up to index 16 to update all vertices of the left rectangle
        {
            updatedRectangleVertices[i] += rightRectangleYOffset;
        }

        // Update the VBO with the new vertex data
        glBindBuffer(GL_ARRAY_BUFFER, rectangleVBO);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(updatedRectangleVertices), updatedRectangleVertices);

        // draw ball
        glBindVertexArray(ballVAO);
        glDrawArrays(GL_TRIANGLES, 0, 6); // Draw the ball

        // Activate the shader program
        glUseProgram(shaderProgram);

        glBindVertexArray(rectangleVAO);
        glDrawArrays(GL_TRIANGLES, 0, 12); // Draw both rectangles

        // glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
        // -------------------------------------------------------------------------------
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // optional: de-allocate all resources once they've outlived their purpose:
    // ------------------------------------------------------------------------
    glDeleteVertexArrays(1, &rectangleVAO);
    glDeleteBuffers(1, &rectangleVBO);
    glDeleteProgram(shaderProgram);

    // glfw: terminate, clearing all previously allocated GLFW resources.
    // ------------------------------------------------------------------
    glfwTerminate();
    return 0;
}

// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
void processInput(GLFWwindow *window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    // Update the Y-axis offset for the left rectangle when "W" is pressed
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS && leftRectangleYOffset + rectangleHeight / 2 < 1.0f)
        leftRectangleYOffset += moveSpeed;

    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS && leftRectangleYOffset - rectangleHeight / 2 > -1.0f)
        leftRectangleYOffset -= moveSpeed;

    // Update the Y-axis offset for the right rectangle when "UP" is pressed
    if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS && rightRectangleYOffset + rectangleHeight / 2 < 1.0f)
        rightRectangleYOffset += moveSpeed;

    if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS && rightRectangleYOffset - rectangleHeight / 2 > -1.0f)
        rightRectangleYOffset -= moveSpeed;
}

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
// ---------------------------------------------------------------------------------------------
void framebuffer_size_callback(GLFWwindow *window, int width, int height)
{
    // make sure the viewport matches the new window dimensions; note that width and
    // height will be significantly larger than specified on retina displays.
    glViewport(0, 0, width, height);
}