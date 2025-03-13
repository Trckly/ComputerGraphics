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

// Structure to store square properties
struct Square {
    GLfloat x, y;        // Position
    GLfloat size;        // Size of the square
    GLfloat color[3];    // Color of the square (RGB)
};

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
std::vector<Square> squares; // Vector to store squares
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

    window = glfwCreateWindow(800, 600, "OpenGL Squares", nullptr, nullptr);
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

    glClearColor(0.1f, 0.1f, 0.1f, 1.0f); // Dark background
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
    if (FT_New_Face(ft, "/System/Library/Fonts/Helvetica.ttc", 0, &face)) {
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
    int width, height;
    glfwGetFramebufferSize(glfwGetCurrentContext(), &width, &height);

    // Convert from [-1, 1] to [0, width/height]
    x = (x + 1.0f) * 0.5f * width;
    y = (y + 1.0f) * 0.5f * height;
}

// Draw the squares with labels
void drawSquares() {
    for (const auto& square : squares) {
        // --- Square Render ---
        glUseProgram(shaderProgram);
        glBindVertexArray(VAO);
        // Set the square color
        glUniform4f(glGetUniformLocation(shaderProgram, "squareColor"), square.color[0], square.color[1], square.color[2], 1.0f);

        GLfloat model[] = {
            square.x - square.size, square.y - square.size,
            square.x + square.size, square.y - square.size,
            square.x - square.size, square.y + square.size,
            square.x + square.size, square.y + square.size
        };

        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(model), model, GL_STATIC_DRAW);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

        glBindVertexArray(0);

        // --- Text Render ---
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        GLfloat invertedColor[3];
        invertColor(square.color, invertedColor);

        // Convert RGB values to text
        std::stringstream ss;
        ss << "RGB("
           << static_cast<int>(square.color[0] * 255) << ", "
           << static_cast<int>(square.color[1] * 255) << ", "
           << static_cast<int>(square.color[2] * 255) << ")";

        // Convert normalized coordinates to window coordinates for text rendering
        float textX = square.x;
        float textY = square.y;
        normalizedToWindowCoords(textX, textY);

        // Center the text on the square
        textX -= ss.str().length() * 6; // Adjust based on text length (approximation)

        // Render the text with inverted color for better visibility
        renderText(ss.str(), textX, textY, 0.5f, glm::vec3(invertedColor[0], invertedColor[1], invertedColor[2]));

        glDisable(GL_BLEND);
    }
}

void handleKeyboardInput(GLFWwindow* window) {
    static bool zPressed = false;
    static bool xPressed = false;

    int width, height;
    glfwGetFramebufferSize(window, &width, &height);


    if (glfwGetKey(window, GLFW_KEY_Z) == GLFW_PRESS) {
        if (!zPressed) {
            Square newSquare{};

            // Generate positions in pixel space
            const auto x = static_cast<float>(arc4random() % width);
            const auto y = static_cast<float>(arc4random() % height);

            // Convert pixel coordinates to OpenGL normalized coordinates
            newSquare.x = x / static_cast<float>(width) * 2.0f - 1.0f;
            newSquare.y = y / static_cast<float>(height) * 2.0f - 1.0f;

            newSquare.size = static_cast<float>(arc4random() % 200 + 50) / static_cast<float>(width);

            newSquare.color[0] = static_cast<float>(arc4random() % 100) / 100.0f;
            newSquare.color[1] = static_cast<float>(arc4random() % 100) / 100.0f;
            newSquare.color[2] = static_cast<float>(arc4random() % 100) / 100.0f;

            squares.push_back(newSquare);
            zPressed = true;
        }
    } else {
        zPressed = false; // Reset the flag when the key is released
    }

    // Check if the 'X' key is pressed
    if (glfwGetKey(window, GLFW_KEY_X) == GLFW_PRESS) {
        if (!xPressed) { // Only trigger once on first press
            // Delete the oldest square (if any)
            if (!squares.empty()) {
                squares.pop_back();
            }
            xPressed = true; // Set the flag to indicate 'X' has been pressed
        }
    } else {
        xPressed = false; // Reset the flag when the key is released
    }
}

// Main loop for handling events and rendering
void mainLoop(GLFWwindow* window) {
    while (!glfwWindowShouldClose(window)) {
        glClear(GL_COLOR_BUFFER_BIT);

        handleKeyboardInput(window); // Check for keyboard input ('Z' and 'X')

        drawSquares(); // Draw all squares with labels

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