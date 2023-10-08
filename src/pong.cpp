#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <ft2build.h>
#include <glm/glm.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <map>
#include FT_FREETYPE_H

void framebuffer_size_callback(GLFWwindow *window, int width, int height);
void processInput(GLFWwindow *window);
void RenderText(unsigned int shaderProgram, std::string text, float x, float y, float scale, glm::vec3 color, int VAO, int VBO);
float CalculateTextWidth(const std::string &text, float scale);

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

const char *freeTypeVertexShaderSource = "#version 330 core\n"
                                         "layout (location = 0) in vec4 vertex; // <vec2 pos, vec2 tex>\n"
                                         "out vec2 TexCoords;\n"
                                         "\n"
                                         "uniform mat4 projection;\n"
                                         "\n"
                                         "void main()\n"
                                         "{\n"
                                         "    gl_Position = projection * vec4(vertex.xy, 0.0, 1.0);\n"
                                         "    TexCoords = vertex.zw;\n"
                                         "}";

const char *freeTypeFragmentShaderCode =
    "#version 330 core\n"
    "in vec2 TexCoords;\n"
    "out vec4 color;\n"
    "\n"
    "uniform sampler2D text;\n"
    "uniform vec3 textColor;\n"
    "\n"
    "void main()\n"
    "{\n"
    "    vec4 sampled = vec4(1.0, 1.0, 1.0, texture(text, TexCoords).r);\n"
    "    color = vec4(textColor, 1.0) * sampled;\n"
    "}";

const float moveSpeed = 0.01f;      // Speed of movement
const float rectangleHeight = 0.2f; // Height of the rectangle
float leftRectangleYOffset = 0.0f;  // Variable to hold the Y-axis offset for the left rectangle
float rightRectangleYOffset = 0.0f; // Variable to hold the Y-axis offset for the right rectangle

// Ball variables
float ballPositionX = 0.0f;    // Initial X position of the ball
float ballPositionY = 0.0f;    // Initial Y position of the ball
float ballVelocityX = 0.005f;  // Initial X-axis speed of the ball
float ballVelocityY = 0.005f;  // Initial Y-axis speed of the ball
const float ballSize = 0.025f; // Size of the ball

struct Character
{
    unsigned int TextureID; // ID handle of the glyph texture
    glm::ivec2 Size;        // Size of glyph
    glm::ivec2 Bearing;     // Offset from baseline to left/top of glyph
    unsigned int Advance;   // Offset to advance to next glyph
};

std::map<char, Character> Characters;
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

    // free type setup
    std::cout << "Checkpoint 1" << std::endl;
    FT_Library ft;
    if (FT_Init_FreeType(&ft))
    {
        std::cout << "ERROR::FREETYPE: Could not init FreeType Library" << std::endl;
        return -1;
    }

    std::cout << "Checkpoint 2" << std::endl;
    FT_Face face;
    if (FT_New_Face(ft, "assets/PressStart2P-Regular.ttf", 0, &face))
    {
        std::cout << "ERROR::FREETYPE: Failed to load font" << std::endl;
        return -1;
    }

    std::cout << "Checkpoint 3" << std::endl;
    FT_Set_Pixel_Sizes(face, 0, 48);

    std::cout << "Checkpoint 4" << std::endl;
    if (FT_Load_Char(face, 'X', FT_LOAD_RENDER))
    {
        std::cout << "ERROR::FREETYTPE: Failed to load Glyph" << std::endl;
        return -1;
    }

    std::cout << "Checkpoint 5" << std::endl;
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1); // disable byte-alignment restriction

    std::cout << "Checkpoint 6" << std::endl;
    for (unsigned char c = 0; c < 128; c++)
    {
        // load character glyph
        if (FT_Load_Char(face, c, FT_LOAD_RENDER))
        {
            std::cout << "ERROR::FREETYTPE: Failed to load Glyph" << std::endl;
            continue;
        }
        // generate texture
        unsigned int texture;
        glGenTextures(1, &texture);
        glBindTexture(GL_TEXTURE_2D, texture);
        glTexImage2D(
            GL_TEXTURE_2D,
            0,
            GL_RED,
            face->glyph->bitmap.width,
            face->glyph->bitmap.rows,
            0,
            GL_RED,
            GL_UNSIGNED_BYTE,
            face->glyph->bitmap.buffer);
        // set texture options
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        // now store character for later use
        Character character = {
            texture,
            glm::ivec2(face->glyph->bitmap.width, face->glyph->bitmap.rows),
            glm::ivec2(face->glyph->bitmap_left, face->glyph->bitmap_top),
            static_cast<unsigned int>(face->glyph->advance.x)};
        Characters.insert(std::pair<char, Character>(c, character));
    }

    // clear free type resources
    FT_Done_Face(face);
    FT_Done_FreeType(ft);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    unsigned int freeTypeVAO, freeTypeVBO;
    glGenVertexArrays(1, &freeTypeVAO);
    glGenBuffers(1, &freeTypeVBO);
    glBindVertexArray(freeTypeVAO);
    glBindBuffer(GL_ARRAY_BUFFER, freeTypeVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 6 * 4, NULL, GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    unsigned int freeTypeVertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(freeTypeVertexShader, 1, &freeTypeVertexShaderSource, NULL);
    glCompileShader(freeTypeVertexShader);
    // check for shader compile errors
    // ... (similar to how you checked for vertexShader and fragmentShader)

    unsigned int freeTypeFragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(freeTypeFragmentShader, 1, &freeTypeFragmentShaderCode, NULL);
    glCompileShader(freeTypeFragmentShader);
    // check for shader compile errors
    // ... (similar to how you checked for vertexShader and fragmentShader)

    unsigned int freeTypeShaderProgram = glCreateProgram();
    glAttachShader(freeTypeShaderProgram, freeTypeVertexShader);
    glAttachShader(freeTypeShaderProgram, freeTypeFragmentShader);
    glLinkProgram(freeTypeShaderProgram);

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

    // free type

    // Define color and projection before using them
    glm::vec3 color = glm::vec3(1.0f); // replace with your color values
    glm::mat4 projection = glm::ortho(0.0f, static_cast<float>(SCR_WIDTH), 0.0f, static_cast<float>(SCR_HEIGHT));

    glUseProgram(shaderProgram);
    unsigned int projectionLoc = glGetUniformLocation(shaderProgram, "projection");
    glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(projection));

    unsigned int textColorLoc = glGetUniformLocation(shaderProgram, "textColor");
    glUniform3f(textColorLoc, color.x, color.y, color.z);
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
        for (int i = 1; i < 18; i += 3)
        {
            updatedRectangleVertices[i] += leftRectangleYOffset;
        }

        // Update Y-coordinates for both triangles of the right rectangle
        for (int i = 19; i < 36; i += 3)
        {
            updatedRectangleVertices[i] += rightRectangleYOffset;
        }

        // Update the VBO with the new vertex data for the rectangles
        glBindBuffer(GL_ARRAY_BUFFER, rectangleVBO);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(updatedRectangleVertices), updatedRectangleVertices);

        // Ball movement
        ballPositionX += ballVelocityX;
        ballPositionY += ballVelocityY;

        // Wall collision
        if (ballPositionY + ballSize >= 1.0f || ballPositionY - ballSize <= -1.0f)
        {
            ballVelocityY = -ballVelocityY;
        }

        // Paddle collision (simplified for demonstration)
        if ((ballPositionX - ballSize <= -0.8f && ballPositionY <= leftRectangleYOffset + 0.1f && ballPositionY >= leftRectangleYOffset - 0.1f) ||
            (ballPositionX + ballSize >= 0.8f && ballPositionY <= rightRectangleYOffset + 0.1f && ballPositionY >= rightRectangleYOffset - 0.1f))
        {
            ballVelocityX = -ballVelocityX;
        }

        // Reset ball if it goes past the left or right edges
        if (ballPositionX - ballSize <= -1.0f || ballPositionX + ballSize >= 1.0f)
        {
            ballPositionX = 0.0f;
            ballPositionY = 0.0f;
        }

        // Update ball vertex data based on its position
        float updatedBallVertices[18];
        for (int i = 0; i < 18; i += 3)
        {
            updatedBallVertices[i] = ballVertices[i] + ballPositionX;
            updatedBallVertices[i + 1] = ballVertices[i + 1] + ballPositionY;
            updatedBallVertices[i + 2] = ballVertices[i + 2];
        }

        // Update the VBO with the new vertex data for the ball
        glBindBuffer(GL_ARRAY_BUFFER, ballVBO);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(updatedBallVertices), updatedBallVertices);

        // Activate the shader program
        glUseProgram(shaderProgram);

        // Render the rectangles
        glBindVertexArray(rectangleVAO);
        glDrawArrays(GL_TRIANGLES, 0, 12);

        // Render the ball
        glBindVertexArray(ballVAO);
        glDrawArrays(GL_TRIANGLES, 0, 6);

        // render the text
        glUseProgram(freeTypeShaderProgram);
        unsigned int projectionLoc = glGetUniformLocation(freeTypeShaderProgram, "projection");
        glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(projection));

        std::string scoreText = "1 - 0";                       // Or whatever your score text is
        float textWidth = CalculateTextWidth(scoreText, 1.0f); // Assuming a scale of 1.0
        float scoreXPosition = (SCR_WIDTH - textWidth) / 2.0f;
        float scoreYPosition = SCR_HEIGHT - 70.0f; // 50 pixels from the top, adjust as necessar
        RenderText(freeTypeShaderProgram, scoreText, scoreXPosition, scoreYPosition, 1.0f, glm::vec3(0.5, 0.8f, 0.2f), freeTypeVAO, freeTypeVBO);

        // glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // optional: de-allocate all resources once they've outlived their purpose:
    // ------------------------------------------------------------------------
    glDeleteVertexArrays(1, &rectangleVAO);
    glDeleteBuffers(1, &rectangleVBO);
    glDeleteProgram(shaderProgram);

    // glfw: terminate, clearing all previously allocated GLFW resources.
    glfwTerminate();
    return 0;
}

// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
void processInput(GLFWwindow *window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS && leftRectangleYOffset + rectangleHeight / 2 < 1.0f)
        leftRectangleYOffset += moveSpeed;

    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS && leftRectangleYOffset - rectangleHeight / 2 > -1.0f)
        leftRectangleYOffset -= moveSpeed;

    if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS && rightRectangleYOffset + rectangleHeight / 2 < 1.0f)
        rightRectangleYOffset += moveSpeed;

    if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS && rightRectangleYOffset - rectangleHeight / 2 > -1.0f)
        rightRectangleYOffset -= moveSpeed;
}

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
void framebuffer_size_callback(GLFWwindow *window, int width, int height)
{
    glViewport(0, 0, width, height);
}

void RenderText(unsigned int shaderProgram, std::string text, float x, float y, float scale, glm::vec3 color, int VAO, int VBO)
{
    // Activate corresponding render state
    glUseProgram(shaderProgram);
    glUniform3f(glGetUniformLocation(shaderProgram, "textColor"), color.x, color.y, color.z);
    glActiveTexture(GL_TEXTURE0);
    glBindVertexArray(VAO);

    // iterate through all characters
    std::string::const_iterator c;
    for (c = text.begin(); c != text.end(); c++)
    {
        Character ch = Characters[*c];

        float xpos = x + ch.Bearing.x * scale;
        float ypos = y - (ch.Size.y - ch.Bearing.y) * scale;

        float w = ch.Size.x * scale;
        float h = ch.Size.y * scale;
        // update VBO for each character
        float vertices[6][4] = {
            {xpos, ypos + h, 0.0f, 0.0f},
            {xpos, ypos, 0.0f, 1.0f},
            {xpos + w, ypos, 1.0f, 1.0f},

            {xpos, ypos + h, 0.0f, 0.0f},
            {xpos + w, ypos, 1.0f, 1.0f},
            {xpos + w, ypos + h, 1.0f, 0.0f}};
        // render glyph texture over quad
        glBindTexture(GL_TEXTURE_2D, ch.TextureID);
        // update content of VBO memory
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        // render quad
        glDrawArrays(GL_TRIANGLES, 0, 6);
        // now advance cursors for next glyph (note that advance is number of 1/64 pixels)
        x += (ch.Advance >> 6) * scale; // bitshift by 6 to get value in pixels (2^6 = 64)
    }
    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);
}

float CalculateTextWidth(const std::string &text, float scale)
{
    float width = 0.0f;
    for (char c : text)
    {
        Character ch = Characters[c];
        width += (ch.Advance >> 6) * scale; // Bitshift by 6 to get value in pixels (2^6 = 64)
    }
    return width;
}
