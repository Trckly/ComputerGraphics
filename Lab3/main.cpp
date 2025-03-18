#include <iostream>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <vector>
#include <cstdlib>
#include <ctime>
#include <sstream>
#include <map>
#include <ft2build.h>
#include FT_FREETYPE_H
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

// Shader sources
const char* vertexShaderSource = R"(
    #version 410 core
    layout (location = 0) in vec2 aPos;
    void main() {
        gl_Position = vec4(aPos, 0.0, 1.0);
    }
)";

const char* fragmentShaderSource = R"(
    #version 410 core
    out vec4 FragColor;
    uniform vec4 squareColor;
    void main() {
        FragColor = squareColor;
    }
)";

// Text rendering shader sources
const char* textVertexShaderSource = R"(
    #version 410 core
    layout (location = 0) in vec4 vertex; // <vec2 pos, vec2 tex>
    out vec2 TexCoords;

    uniform mat4 projection;

    void main() {
        gl_Position = projection * vec4(vertex.xy, 0.0, 1.0);
        TexCoords = vertex.zw;
    }
)";

const char* textFragmentShaderSource = R"(
    #version 410 core
    in vec2 TexCoords;
    out vec4 color;

    uniform sampler2D text;
    uniform vec3 textColor;

    void main() {
        vec4 sampled = vec4(1.0, 1.0, 1.0, texture(text, TexCoords).r);
        color = vec4(textColor, 1.0) * sampled;
    }
)";

// Structure to store character data
struct Character {
    unsigned int TextureID; // ID handle of the glyph texture
    glm::ivec2 Size;        // Size of glyph
    glm::ivec2 Bearing;     // Offset from baseline to left/top of glyph
    unsigned int Advance;   // Offset to advance to next glyph
};

// Global variables
GLuint shaderProgram, VAO, VBO;
GLuint textShaderProgram, textVAO, textVBO;
float scale = 1.0f;
int windowWidth, windowHeight;
float a = 1.0f;
float turnRatio = 0.0f;
float approxStep = 0.1f;
bool playAnimation = false;
std::map<char, Character> Characters; // Map of characters for text rendering
glm::mat4 projection; // Projection matrix for text rendering

// Forward declaration
void renderText(const std::string& text, float x, float y, float scale, glm::vec3 color);

// Initialize GLFW, GLEW, and OpenGL settings
bool initOpenGL(GLFWwindow*& window) {
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW\n";
        return false;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);

    window = glfwCreateWindow(800, 800, "OpenGL Squares", nullptr, nullptr);
    if (!window) {
        std::cerr << "Failed to create GLFW window\n";
        glfwTerminate();
        return false;
    }

    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, [](GLFWwindow* window, int width, int height) {
        glViewport(0, 0, width, height);
    });

    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK) {
        std::cerr << "Failed to initialize GLEW\n";
        return false;
    }

    glClearColor(0.9f, 0.9f, 0.9f, 1.0f); // Dark background
    return true;
}

// Initialize shaders and buffers
void initShadersAndBuffers() {
    // Compile vertex shader
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, nullptr);
    glCompileShader(vertexShader);

    // Compile fragment shader
    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, nullptr);
    glCompileShader(fragmentShader);

    // Link shaders to a program
    shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);
    glUseProgram(shaderProgram);

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    // Set up vertex data for a square (4 vertices)
    GLfloat vertices[] = {
        -0.5f, -0.5f,  // Bottom-left
         0.5f, -0.5f,  // Bottom-right
        -0.5f,  0.5f,  // Top-left
         0.5f,  0.5f   // Top-right
    };

    // Set up buffers
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(GLfloat), (GLvoid*)0);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0); // Unbind VAO

    // Initialize text shaders
    vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &textVertexShaderSource, nullptr);
    glCompileShader(vertexShader);

    fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &textFragmentShaderSource, nullptr);
    glCompileShader(fragmentShader);

    textShaderProgram = glCreateProgram();
    glAttachShader(textShaderProgram, vertexShader);
    glAttachShader(textShaderProgram, fragmentShader);
    glLinkProgram(textShaderProgram);

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    // Initialize text VAO and VBO
    glGenVertexArrays(1, &textVAO);
    glGenBuffers(1, &textVBO);
    glBindVertexArray(textVAO);
    glBindBuffer(GL_ARRAY_BUFFER, textVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 6 * 4, nullptr, GL_DYNAMIC_DRAW);
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 0);
    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    // Set up orthographic projection for text rendering
    int width, height;
    glfwGetFramebufferSize(glfwGetCurrentContext(), &width, &height);
    projection = glm::ortho(0.0f, static_cast<float>(width), 0.0f, static_cast<float>(height));
}

// Initialize FreeType and load a font
bool initFont() {
    FT_Library ft;
    if (FT_Init_FreeType(&ft)) {
        std::cerr << "ERROR::FREETYPE: Could not init FreeType Library" << std::endl;
        return false;
    }

    FT_Face face;
    // Load font (use a path to a TTF font file on your system)
    if (FT_New_Face(ft, "C:/Windows/Fonts/arial.ttf", 0, &face)) {
        std::cerr << "ERROR::FREETYPE: Failed to load font" << std::endl;
        return false;
    }

    FT_Set_Pixel_Sizes(face, 0, 24); // Set size to load glyphs as

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1); // Disable byte-alignment restriction

    // Load first 128 ASCII characters
    for (unsigned char c = 0; c < 128; c++) {
        // Load character glyph
        if (FT_Load_Char(face, c, FT_LOAD_RENDER)) {
            std::cerr << "ERROR::FREETYTPE: Failed to load Glyph" << std::endl;
            continue;
        }

        // Generate texture
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
            face->glyph->bitmap.buffer
        );

        // Set texture options
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        // Now store character for later use
        Character character = {
            texture,
            glm::ivec2(face->glyph->bitmap.width, face->glyph->bitmap.rows),
            glm::ivec2(face->glyph->bitmap_left, face->glyph->bitmap_top),
            static_cast<unsigned int>(face->glyph->advance.x)
        };
        Characters.insert(std::pair<char, Character>(c, character));
    }

    // Destroy FreeType once we're finished
    FT_Done_Face(face);
    FT_Done_FreeType(ft);
    return true;
}

// Render a text string
void renderText(const std::string& text, float x, float y, float scale, glm::vec3 color) {
    // Activate corresponding render state
    glUseProgram(textShaderProgram);
    glUniform3f(glGetUniformLocation(textShaderProgram, "textColor"), color.x, color.y, color.z);
    glUniformMatrix4fv(glGetUniformLocation(textShaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
    glActiveTexture(GL_TEXTURE0);
    glBindVertexArray(textVAO);

    // Iterate through all characters
    float startX = x;
    for (char c : text) {
        Character ch = Characters[c];

        float xpos = startX + ch.Bearing.x * scale;
        float ypos = y - (ch.Size.y - ch.Bearing.y) * scale;

        float w = ch.Size.x * scale;
        float h = ch.Size.y * scale;

        // Update VBO for each character
        float vertices[6][4] = {
            { xpos,     ypos + h,   0.0f, 0.0f },
            { xpos,     ypos,       0.0f, 1.0f },
            { xpos + w, ypos,       1.0f, 1.0f },

            { xpos,     ypos + h,   0.0f, 0.0f },
            { xpos + w, ypos,       1.0f, 1.0f },
            { xpos + w, ypos + h,   1.0f, 0.0f }
        };

        // Render glyph texture over quad
        glBindTexture(GL_TEXTURE_2D, ch.TextureID);

        // Update content of VBO memory
        glBindBuffer(GL_ARRAY_BUFFER, textVBO);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
        glBindBuffer(GL_ARRAY_BUFFER, 0);

        // Render quad
        glDrawArrays(GL_TRIANGLES, 0, 6);

        // Now advance cursors for next glyph (note that advance is number of 1/64 pixels)
        startX += (ch.Advance >> 6) * scale; // Bitshift by 6 to get value in pixels (2^6 = 64)
    }
    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);
}

// Function to invert color
void invertColor(const GLfloat color[3], GLfloat invertedColor[3]) {
    for (int i = 0; i < 3; i++) {
        invertedColor[i] = 1.0f - color[i];  // Invert the color component
    }
}

// Convert normalized OpenGL coordinates to window coordinates
void normalizedToWindowCoords(float& x, float& y) {
    // Convert from [-1, 1] to [0, width/height]
    x = (x + 1.0f) * 0.5f * windowWidth;
    // Convert from [-1, 1] to [0, height] for y (flip y-axis)
    y = (1.0f - (y + 1.0f) * 0.5f) * windowHeight;
}

// Convert window coordinates to normalized OpenGL coordinates
void windowToNormalizedCoords(float& x, float& y) {
    // Convert from [0, width] to [-1, 1] for x
    x = (x / windowWidth) * 2.0f - 1.0f;

    // Convert from [0, height] to [-1, 1] for y (flip y-axis)
    y = 1.0f - (y / windowHeight) * 2.0f;
}

float pixelToNDC(int pixelCount, bool isRelativeToWidth) {
    if (isRelativeToWidth) {
        // Convert pixel count to NDC relative to width
        return static_cast<float>(pixelCount) / windowWidth * 2.0f;
    } else {
        // Convert pixel count to NDC relative to height
        return static_cast<float>(pixelCount) / windowHeight * 2.0f;
    }
}

float NDCToPixel(float ndcValue, bool isRelativeToWidth) {
    if (isRelativeToWidth) {
        // Convert pixel count to NDC relative to width
        return ndcValue * windowWidth / 2.0f;
    } else {
        // Convert pixel count to NDC relative to height
        return ndcValue * windowHeight / 2.0f;
    }
}

void drawSquare(glm::vec2 offset, float squareSize, std::vector<GLfloat> color) {
    if (color.size() != 4) {
        throw std::runtime_error("draw line has to have 4 elements");
    }

    glUseProgram(shaderProgram);
    glBindVertexArray(VAO);

    // Set the square color
    glUniform4f(glGetUniformLocation(shaderProgram, "squareColor"), color[0], color[1], color[2], color[3]);

    float halfSize = squareSize * 0.5f;

    // Define the square's vertices (centered at origin for easier transformations)
    std::vector<GLfloat> model = {
        -halfSize, -halfSize,  // Bottom-left
        -halfSize, +halfSize,  // Top-left
        +halfSize, -halfSize,  // Bottom-right
        +halfSize, +halfSize   // Top-right
    };

    // Create a transformation matrix
    glm::mat4 modelMatrix = glm::mat4(1.0f); // Start with identity matrix

    // Step 1: Translate to the top-left corner (rotation point)
    modelMatrix = glm::translate(modelMatrix, glm::vec3(offset.x - halfSize, offset.y + halfSize, 0.0f));

    // Step 2: Apply rotation (around the Z-axis)
    modelMatrix = glm::rotate(modelMatrix, glm::radians(turnRatio), glm::vec3(0.0f, 0.0f, 1.0f));

    // Step 3: Translate back to the original position
    modelMatrix = glm::translate(modelMatrix, glm::vec3(halfSize, -halfSize, 0.0f));

    // Apply the transformation to the vertices
    for (size_t i = 0; i < model.size(); i += 2) {
        glm::vec4 vertex(model[i], model[i + 1], 0.0f, 1.0f); // Homogeneous coordinates
        vertex = modelMatrix * vertex; // Transform the vertex
        model[i] = vertex.x; // Update the vertex position
        model[i + 1] = vertex.y;
    }

    // Copy the transformed vertices to a std::vector<GLfloat>
    std::vector<GLfloat> modelArray(model.begin(), model.end());

    // Upload the vertex data to the GPU
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, modelArray.size() * sizeof(GLfloat), modelArray.data(), GL_DYNAMIC_DRAW);

    // Draw the square
    glDrawArrays(GL_TRIANGLE_STRIP, 0, model.size() / 2);

    glBindVertexArray(0);
}

void handleKeyboardInput(GLFWwindow* window) {
    static bool zPressed = false;
    static bool xPressed = false;
    static bool qPressed = false;
    static bool wPressed = false;
    static bool spacePressed = false;

    if (glfwGetKey(window, GLFW_KEY_Z) == GLFW_PRESS) {
        if (!zPressed) {
            scale += 0.1f;
            zPressed = true;
        }
    } else {
        zPressed = false;
    }


    if (glfwGetKey(window, GLFW_KEY_X) == GLFW_PRESS) {
        if (!xPressed) {
            scale -= 0.1f;
            xPressed = true;
        }
    } else {
        xPressed = false;
    }

    if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS) {
        if (!qPressed) {
            a += 0.1f;
            turnRatio -= 0.1f;
            qPressed = true;
        }
    } else {
        qPressed = false;
    }

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
        if (!wPressed) {
            a -= 0.1f;
            turnRatio += 0.1f;
            wPressed = true;
        }
    } else {
        wPressed = false;
    }

    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) {
        if (!spacePressed) {
            playAnimation = !playAnimation;
            spacePressed = true;
        }
    } else {
        spacePressed = false;
    }
}

// Main loop for handling events and rendering
void mainLoop(GLFWwindow* window) {
    while (!glfwWindowShouldClose(window)) {
        glClear(GL_COLOR_BUFFER_BIT);

        glfwGetFramebufferSize(glfwGetCurrentContext(), &windowWidth, &windowHeight);

        handleKeyboardInput(window);

        drawSquare({0.0f, 0.0f}, 0.3f, {1.0f, 0.0f, 0.0f, 1.0f});

        if (playAnimation) {
            a -= 0.001f;
            turnRatio += 0.3f;
        }

        glfwSwapBuffers(window);
        glfwPollEvents();
    }
}

int main() {
    srand(time(0)); // Seed for random number generation

    GLFWwindow* window;

    // Initialize OpenGL
    if (!initOpenGL(window)) {
        return -1;
    }

    // Initialize shaders and buffers
    initShadersAndBuffers();

    // Initialize font for text rendering
    if (!initFont()) {
        return -1;
    }

    // Start the main loop
    mainLoop(window);

    // Clean up and terminate
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteVertexArrays(1, &textVAO);
    glDeleteBuffers(1, &textVBO);
    glDeleteProgram(shaderProgram);
    glDeleteProgram(textShaderProgram);

    // Clean up character textures
    for (auto& c : Characters) {
        glDeleteTextures(1, &c.second.TextureID);
    }

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}