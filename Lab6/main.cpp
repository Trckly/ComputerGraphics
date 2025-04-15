#include <GL/glut.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include <stdlib.h>
#include <stdio.h>
#define _USE_MATH_DEFINES
#include <cmath>
#include <chrono>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

// Light IDs (OpenGL has GL_LIGHT0 to GL_LIGHT7)
#define AMBIENT_LIGHT    GL_LIGHT0
#define POINT_LIGHT      GL_LIGHT1
#define DIRECTIONAL_LIGHT GL_LIGHT2
#define SPOTLIGHT        GL_LIGHT3

// Змінні для контролю обертання та масштабування
GLfloat planetRotation = 0.0f;         // Обертання планети навколо своєї осі
GLfloat planetRevolution = 0.0f;       // Обертання планети навколо зорі
GLfloat zoomFactor = -20.0f;           // Масштаб сцени
GLfloat viewAngle = 50.0f;             // Кут огляду

// Змінні для текстур
GLuint starTexture;
GLuint moonTexture;

// Параметри для туману
GLfloat fogColor[4] = {0.0f, 0.0f, 0.0f, 1.0f};
bool enableFog = false;

// Змінна для контролю анімації
bool animationActive = true;

// Змінні для контролю камери
float cameraX = 0.0f;
float cameraY = 0.0f;
float cameraZ = 20.0f;
float cameraYaw = 0.0f;    // Поворот вліво-вправо
float cameraPitch = 0.0f;  // Поворот вгору-вниз
float cameraSpeed = 0.2f;  // Швидкість руху камери

// Змінні для контролю миші
int mouseX = 0, mouseY = 0;
bool mouseLeftDown = false;
bool mouseRightDown = false;
bool mouseMiddleDown = false;
float mouseRotateSpeed = 7.0f;  // Швидкість повороту камери
float mouseMoveSpeed = 0.05f;   // Швидкість руху камери

// Змінні для вільного огляду камерою
bool freeLookMode = true;

// Змінні для вимірювання deltaTime
double currentTime = 0.0;
double previousTime = 0.0;
double deltaTime = 0.0;    // Час між кадрами у секундах
double fps = 0.0;          // Частота кадрів за секунду

// Константи швидкості руху та обертання
const float BASE_ROTATE_SPEED = 60.0f;  // градусів за секунду
const float BASE_MOVE_SPEED = 500.0f;     // одиниць за секунду

// Константи відмальовки кадрів
const double TARGET_FPS = 60.0;
const double FRAME_TIME = 1.0 / TARGET_FPS;

// Константи осі планет
const double AXIS_ANGLE = 23.5f;

bool keyStates[256] = { false };

void updateMovement();
GLuint createFallbackTexture(const char* filename);

#ifdef _WIN64 || _WIN32
#include <windows.h>
void sleepForSeconds(double seconds) {
    Sleep(seconds * 1000);
}
#endif

// Отримання поточного часу в секундах
double getTimeInSeconds() {
    auto now = std::chrono::high_resolution_clock::now();
    auto duration = now.time_since_epoch();
    return std::chrono::duration<double>(duration).count();
}

GLuint loadTexture(const char* filename) {
    GLuint texture;
    int width, height, channels;
    unsigned char* data;

    // Load image using stb_image
    stbi_set_flip_vertically_on_load(1); // Flip images vertically (OpenGL expects bottom-left as origin)
    data = stbi_load(filename, &width, &height, &channels, STBI_rgb_alpha);

    if (!data) {
        printf("Error loading texture '%s': %s\n", filename, stbi_failure_reason());

        // Fall back to a simple procedural texture
        return createFallbackTexture(filename);
    }

    // Generate and bind texture
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);

    // Set texture parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    // Enable mipmapping for older OpenGL versions
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // Build mipmaps manually for older OpenGL versions
    gluBuild2DMipmaps(GL_TEXTURE_2D, GL_RGBA, width, height,
                      GL_RGBA, GL_UNSIGNED_BYTE, data);

    // Free the image data as it's now in GPU memory
    stbi_image_free(data);

    printf("Texture '%s' loaded successfully (%dx%d, %d channels)\n",
           filename, width, height, channels);

    return texture;
}

// Fallback function to create a procedural texture when file loading fails
GLuint createFallbackTexture(const char* filename) {
    GLuint texture;

    printf("Creating fallback texture instead of '%s'\n", filename);

    // Create texture object
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);

    // Set texture parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // Create checkerboard pattern as fallback
    const int size = 64;
    unsigned char checkerboard[size][size][4];

    for(int i = 0; i < size; i++) {
        for(int j = 0; j < size; j++) {
            int c = ((i & 0x8) == 0 ^ (j & 0x8) == 0) * 255;
            checkerboard[i][j][0] = c;
            checkerboard[i][j][1] = 0;
            checkerboard[i][j][2] = c;
            checkerboard[i][j][3] = 255;
        }
    }

    // Create mipmapped texture with gluBuild2DMipmaps
    gluBuild2DMipmaps(GL_TEXTURE_2D, GL_RGBA, size, size,
                     GL_RGBA, GL_UNSIGNED_BYTE, checkerboard);

    return texture;
}

// Функція ініціалізації
void init(void) {
    glEnable(GL_LIGHTING);
    glEnable(GL_NORMALIZE);  // For proper lighting calculations

    // Колір фону - чорний (космос)
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

    glutSetCursor(GLUT_CURSOR_NONE); // Приховати курсор

    // Увімкнення тесту глибини для правильного відображення 3D об'єктів
    glEnable(GL_DEPTH_TEST);

    // Увімкнення згладжування
    glEnable(GL_SMOOTH);

    // Ініціалізація джерела світла (зоря)
    glEnable(GL_LIGHTING);

    // Налаштування матеріалу за замовчуванням
    GLfloat mat_ambient[] = {0.7f, 0.7f, 0.7f, 1.0f};
    GLfloat mat_diffuse[] = {0.8f, 0.8f, 0.8f, 1.0f};
    GLfloat mat_specular[] = {0.1f, 0.1f, 0.1f, 1.0f};
    GLfloat mat_shininess[] = {15.0f};

    glMaterialfv(GL_FRONT, GL_AMBIENT, mat_ambient);
    glMaterialfv(GL_FRONT, GL_DIFFUSE, mat_diffuse);
    glMaterialfv(GL_FRONT, GL_SPECULAR, mat_specular);
    glMaterialfv(GL_FRONT, GL_SHININESS, mat_shininess);

    // Завантаження текстур
    moonTexture = loadTexture("..\\Textures\\moon-texture.jpg");
    starTexture = loadTexture("..\\Textures\\star_texture.jpg");

    // Налаштування туману для космічного простору
    glFogi(GL_FOG_MODE, GL_EXP);
    glFogfv(GL_FOG_COLOR, fogColor);
    glFogf(GL_FOG_DENSITY, 0.05f);
    glHint(GL_FOG_HINT, GL_DONT_CARE);

    // Ініціалізація часу
    previousTime = getTimeInSeconds();
}

// Функція малювання зорі
void drawStar() {

    glPushAttrib(GL_LIGHTING_BIT);

    // Відключаємо освітлення для самої зорі, щоб вона світилася рівномірно
    if (glIsEnabled(POINT_LIGHT)) {
        GLfloat mat_emission[] = {1.0f, 1.0f, 1.0f, 1.0f};  // Bright white emission
        glMaterialfv(GL_FRONT, GL_EMISSION, mat_emission);
    }

    // Накладаємо текстуру зорі
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, starTexture);

    glPushMatrix();

    glRotatef(90.0f, 1.0f, 0.0f, 0.0f);

    // Малюємо сферу для зорі
    GLUquadricObj *quadric = gluNewQuadric();
    gluQuadricTexture(quadric, GL_TRUE);
    gluSphere(quadric, 2.0f, 50, 50);
    gluDeleteQuadric(quadric);

    glPopMatrix();

    glDisable(GL_TEXTURE_2D);

    glPopAttrib();
}

// Функція малювання планети з місячною поверхнею
void drawPlanet() {
    // Накладаємо текстуру місячної поверхні
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, moonTexture);

    // Місячно-сірий колір для планети
    glColor3f(1.0f, 1.0f, 1.0f);

    glPushMatrix();

    glRotatef(90.0f, 1.0f, 0.0f, 0.0f);


    // Малюємо сферу для планети
    GLUquadricObj *quadric = gluNewQuadric();
    gluQuadricTexture(quadric, GL_TRUE);
    gluSphere(quadric, 0.8f, 30, 30);
    gluDeleteQuadric(quadric);

    glPopMatrix();

    glDisable(GL_TEXTURE_2D);
}

// Функція для відображення тексту на екрані
void renderText(float x, float y, const char* text, void* font = GLUT_BITMAP_HELVETICA_12) {
    glDisable(GL_LIGHTING);
    glDisable(GL_TEXTURE_2D);
    glColor3f(1.0f, 1.0f, 1.0f);

    // Встановлюємо позицію тексту
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    gluOrtho2D(0, glutGet(GLUT_WINDOW_WIDTH), 0, glutGet(GLUT_WINDOW_HEIGHT));

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    // Виводимо текст
    glRasterPos2f(x, y);
    for (const char* c = text; *c; c++) {
        glutBitmapCharacter(font, *c);
    }

    // Відновлюємо матриці
    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);

    glEnable(GL_TEXTURE_2D);
    glEnable(GL_LIGHTING);
}

void lightUpdate() {
    // Ambient light
    GLfloat ambientColor[] = {0.9f, 0.9f, 0.9f, 1.0f};
    glLightfv(AMBIENT_LIGHT, GL_AMBIENT, ambientColor);

    // Point light
    // Джерело світла розташоване в центрі зорі
    GLfloat point_light_position[] = {0.0f, 0.0f, 0.0f, 1.0f};
    GLfloat point_light_ambient[] = {0.0f, 0.0f, 0.0f, 1.0f};
    GLfloat point_light_diffuse[] = {1.0f, 1.0f, 1.0f, 1.0f};
    GLfloat point_light_specular[] = {1.0f, 1.0f, 1.0f, 1.0f};

    glLightfv(POINT_LIGHT, GL_POSITION, point_light_position);
    glLightfv(POINT_LIGHT, GL_AMBIENT, point_light_ambient);
    glLightfv(POINT_LIGHT, GL_DIFFUSE, point_light_diffuse);
    glLightfv(POINT_LIGHT, GL_SPECULAR, point_light_specular);

    // Затухання світла з відстанню
    glLightf(POINT_LIGHT, GL_CONSTANT_ATTENUATION, 1.0f);
    glLightf(POINT_LIGHT, GL_LINEAR_ATTENUATION, 0.0f);
    glLightf(POINT_LIGHT, GL_QUADRATIC_ATTENUATION, 0.0001f);


    // Directional light
    GLfloat directional_light_position[] = {1.0f, 1.0f, 0.0f, 0.0f};
    GLfloat directional_light_ambient[] = {0.0f, 0.0f, 0.0f, 1.0f};
    GLfloat directional_light_diffuse[] = {1.0f, 1.0f, 1.0f, 1.0f};
    GLfloat directional_light_specular[] = {1.0f, 1.0f, 1.0f, 1.0f};

    glLightfv(DIRECTIONAL_LIGHT, GL_POSITION, directional_light_position);
    glLightfv(DIRECTIONAL_LIGHT, GL_AMBIENT, directional_light_ambient);
    glLightfv(DIRECTIONAL_LIGHT, GL_DIFFUSE, directional_light_diffuse);
    glLightfv(DIRECTIONAL_LIGHT, GL_SPECULAR, directional_light_specular);
    // Затухання світла з відстанню
    glLightf(DIRECTIONAL_LIGHT, GL_CONSTANT_ATTENUATION, 1.0f);
    glLightf(DIRECTIONAL_LIGHT, GL_LINEAR_ATTENUATION, 0.0f);
    glLightf(DIRECTIONAL_LIGHT, GL_QUADRATIC_ATTENUATION, 0.0f);

    GLfloat spotlight_position[] = {0.0f, 100.0f, 0.0f, 1.0f};
    GLfloat spotlight_direction[] = {0.0f, -1.0f, 0.0f};
    GLfloat spotlight_ambient[] = {0.0f, 0.0f, 0.0f, 1.0f};
    GLfloat spotlight_diffuse[] = {1.0f, 1.0f, 1.0f, 1.0f};
    GLfloat spotlight_specular[] = {1.0f, 1.0f, 1.0f, 1.0f};

    glLightfv(SPOTLIGHT, GL_POSITION, spotlight_position);
    glLightfv(SPOTLIGHT, GL_SPOT_DIRECTION, spotlight_direction);
    glLightfv(SPOTLIGHT, GL_AMBIENT, spotlight_ambient);
    glLightfv(SPOTLIGHT, GL_DIFFUSE, spotlight_diffuse);
    glLightfv(SPOTLIGHT, GL_SPECULAR, spotlight_specular);
    // Затухання світла з відстанню
    glLightf(SPOTLIGHT, GL_SPOT_CUTOFF, 2.0f);
    glLightf(SPOTLIGHT, GL_SPOT_EXPONENT, 70.0f);
    glLightf(SPOTLIGHT, GL_CONSTANT_ATTENUATION, 1.0f);
    glLightf(SPOTLIGHT, GL_LINEAR_ATTENUATION, 0.0f);
    glLightf(SPOTLIGHT, GL_QUADRATIC_ATTENUATION, 0.0001f);
}

// Функція малювання сцени
void display(void) {
    // Вимірювання часу для розрахунку deltaTime
    currentTime = getTimeInSeconds();
    deltaTime = currentTime - previousTime;

    // Calculate target frame delay in seconds
    float targetFrameRate = 1.0 / TARGET_FPS;

    // If we're running too fast, wait until it's time to render the next frame
    if (deltaTime < targetFrameRate) {
        // Calculate how much time we need to wait
        float sleepTime = targetFrameRate - deltaTime;

        // Use a sleep function appropriate for your system
        // e.g., Sleep(sleepTime * 1000) on Windows or usleep(sleepTime * 1000000) on Unix
        sleepForSeconds(sleepTime);

        // Update currentTime after sleeping
        currentTime = getTimeInSeconds();
        deltaTime = currentTime - previousTime;
    }

    previousTime = currentTime;

    // Розрахунок FPS
    if (deltaTime > 0) {
        fps = 1.0 / deltaTime;
    }

    lightUpdate();

    // Continue with rendering regardless of FPS
    updateMovement();

    // Очищення буферів кольору та глибини
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Ініціалізуємо матрицю моделювання/виду
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    // Розрахунок напрямку погляду камери на основі кутів
    float lookX = cameraX + sin(cameraYaw * M_PI / 180.0);
    float lookY = cameraY + sin(cameraPitch * M_PI / 180.0);
    float lookZ = cameraZ - cos(cameraYaw * M_PI / 180.0);

    // Розташування камери з урахуванням кутів повороту
    gluLookAt(cameraX, cameraY, cameraZ,  // Позиція камери
              lookX, lookY, lookZ,        // Напрямок погляду
              0.0, 1.0, 0.0);             // Вектор "вгору"

    // Малюємо зорю в центрі
    glPushMatrix();
    drawStar();
    glPopMatrix();

    // Малюємо планету на орбіті
    glPushMatrix();

    // Обертання навколо зорі
    glRotatef(planetRevolution, 0.0f, 1.0f, 0.0f);

    // Відстань від зорі
    glTranslatef(8.0f, 0.0f, 0.0f);

    // Нахил осі планети
    glRotatef(-AXIS_ANGLE, 0.0f, 0.0f, 1.0f);

    // Обертання планети навколо своєї осі
    glRotatef(planetRotation, 0.0f, 1.0f, 0.0f);

    drawPlanet();
    glPopMatrix();

    // Відображення інформації про стан системи
    char buffer[128];
    sprintf(buffer, "FPS: %.1f  DeltaTime: %.4f ms", fps, deltaTime * 1000);
    renderText(10, glutGet(GLUT_WINDOW_HEIGHT) - 20, buffer);

    sprintf(buffer, "Camera: X=%.1f Y=%.1f Z=%.1f Yaw=%.1f Pitch=%.1f",
            cameraX, cameraY, cameraZ, cameraYaw, cameraPitch);
    renderText(10, glutGet(GLUT_WINDOW_HEIGHT) - 40, buffer);

    sprintf(buffer, "FreeLook Mode: %s  [F] to toggle", freeLookMode ? "ON" : "OFF");
    renderText(10, glutGet(GLUT_WINDOW_HEIGHT) - 60, buffer);

    // Відображення результату на екрані
    glutSwapBuffers();
}

// Функція для зміни розміру вікна
void reshape(int width, int height) {
    // Запобігання ділення на нуль
    if (height == 0)
        height = 1;

    // Встановлення розміру вікна перегляду
    glViewport(0, 0, width, height);

    // Перехід до матриці проекції
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();

    // Встановлення перспективи
    gluPerspective(viewAngle, (GLfloat)width / (GLfloat)height, 0.1f, 100.0f);

    // Повернення до матриці моделювання
    glMatrixMode(GL_MODELVIEW);
}

// Функція для анімації
void animate(int value) {
    if (animationActive) {
        // Оновлення обертальних кутів з урахуванням deltaTime для плавності руху
        planetRotation += BASE_ROTATE_SPEED * deltaTime;  // Обертання планети навколо своєї осі
        if (planetRotation > 360.0f)
            planetRotation -= 360.0f;

        planetRevolution += BASE_ROTATE_SPEED * 0.1f * deltaTime;  // Обертання планети навколо зорі
        if (planetRevolution > 360.0f)
            planetRevolution -= 360.0f;
    }

    // Перемалювання сцени
    glutPostRedisplay();

    // Повторний виклик анімації
    glutTimerFunc(16, animate, 0); // ~60 FPS
}

// Обробник натискання кнопок миші
void mouseFunc(int button, int state, int x, int y) {
    mouseX = x;
    mouseY = y;

    // Обробка кнопок миші
    if (button == GLUT_LEFT_BUTTON) {
        if (state == GLUT_DOWN)
            mouseLeftDown = true;
        else
            mouseLeftDown = false;
    }

    if (button == GLUT_RIGHT_BUTTON) {
        if (state == GLUT_DOWN)
            mouseRightDown = true;
        else
            mouseRightDown = false;
    }

    if (button == GLUT_MIDDLE_BUTTON) {
        if (state == GLUT_DOWN)
            mouseMiddleDown = true;
        else
            mouseMiddleDown = false;
    }
}

// Обробник руху миші з натиснутими кнопками
void mouseMotionFunc(int x, int y) {
    // Розрахунок зміщення курсора миші
    int dx = x - mouseX;
    int dy = y - mouseY;

    // Оновлення останньої позиції миші
    mouseX = x;
    mouseY = y;

    // Швидкість повороту і руху з урахуванням deltaTime
    float adjustedRotateSpeed = mouseRotateSpeed;
    float adjustedMoveSpeed = mouseMoveSpeed;

    // Обробка правої кнопки - переміщення камери
    if (mouseRightDown) {
        // Переміщення камери вперед-назад та вліво-вправо
        float deltaForward = -dy * adjustedMoveSpeed;
        float deltaStrafe = dx * adjustedMoveSpeed;

        // Обчислення напрямків руху з урахуванням кутів камери
        float forwardX = sin(cameraYaw * M_PI / 180.0);
        float forwardZ = -cos(cameraYaw * M_PI / 180.0);

        float strafeX = sin((cameraYaw + 90.0) * M_PI / 180.0);
        float strafeZ = -cos((cameraYaw + 90.0) * M_PI / 180.0);

        // Оновлення позиції камери
        cameraX += forwardX * deltaForward + strafeX * deltaStrafe;
        cameraZ += forwardZ * deltaForward + strafeZ * deltaStrafe;

        int centerX = glutGet(GLUT_WINDOW_WIDTH) / 2;
        int centerY = glutGet(GLUT_WINDOW_HEIGHT) / 2;
        if (x != centerX || y != centerY) {
            mouseX = centerX;
            mouseY = centerY;
            glutWarpPointer(centerX, centerY);
        }
    }
    // Обертання камери якщо не правою кнопкою і не вільний огляд
    else if (mouseLeftDown && !freeLookMode) {
        // Поворот камери вліво-вправо (Yaw)
        cameraYaw += dx * adjustedRotateSpeed * deltaTime;
        if (cameraYaw > 360.0f)
            cameraYaw -= 360.0f;
        if (cameraYaw < 0.0f)
            cameraYaw += 360.0f;

        // Поворот камери вгору-вниз (Pitch) з обмеженням
        cameraPitch += dy * adjustedRotateSpeed * deltaTime;
        if (cameraPitch > 89.0f)
            cameraPitch = 89.0f;
        if (cameraPitch < -89.0f)
            cameraPitch = -89.0f;
    }

    // Обробка середньої кнопки - рух камери вгору-вниз
    if (mouseMiddleDown) {
        cameraY += dy * adjustedMoveSpeed;
    }

    // Перемалювати сцену
    glutPostRedisplay();
}

// Обробник пасивного руху миші (без натиснутих кнопок)
void mousePassiveMotionFunc(int x, int y) {
    // Розрахунок зміщення курсора миші
    int dx = x - mouseX;
    int dy = y - mouseY;

    // Оновлення останньої позиції миші
    mouseX = x;
    mouseY = y;

    // Повороти камери в режимі вільного огляду
    if (freeLookMode) {
        // Поворот камери вліво-вправо (Yaw)
        cameraYaw += dx * mouseRotateSpeed * deltaTime;
        if (cameraYaw > 360.0f)
            cameraYaw -= 360.0f;
        if (cameraYaw < 0.0f)
            cameraYaw += 360.0f;

        // Поворот камери вгору-вниз (Pitch) з обмеженням
        cameraPitch += -1 * dy * mouseRotateSpeed * deltaTime;
        if (cameraPitch > 89.0f)
            cameraPitch = 89.0f;
        if (cameraPitch < -89.0f)
            cameraPitch = -89.0f;

        // Опціонально: Центрування курсора миші для неперервного повороту
        if (freeLookMode) {
            int centerX = glutGet(GLUT_WINDOW_WIDTH) / 2;
            int centerY = glutGet(GLUT_WINDOW_HEIGHT) / 2;
            if (x != centerX || y != centerY) {
                mouseX = centerX;
                mouseY = centerY;
                glutWarpPointer(centerX, centerY);
            }
        }

        // Перемалювати сцену
        glutPostRedisplay();
    }
}

void keyboardDown(unsigned char key, int x, int y) {
    keyStates[key] = true;
}

void updateMovement() {
    // Speed based on deltaTime for smooth movement
    float moveDistance = BASE_MOVE_SPEED * deltaTime;

    // Check WASD keys and adjust camera
    if (keyStates['w']) {
        cameraX += sin(cameraYaw * M_PI / 180.0) * moveDistance * deltaTime;
        cameraZ -= cos(cameraYaw * M_PI / 180.0) * moveDistance * deltaTime;
    }
    if (keyStates['s']) {
        cameraX -= sin(cameraYaw * M_PI / 180.0) * moveDistance * deltaTime;
        cameraZ += cos(cameraYaw * M_PI / 180.0) * moveDistance * deltaTime;
    }
    if (keyStates['a']) {
        cameraX -= cos(cameraYaw * M_PI / 180.0) * moveDistance * deltaTime;
        cameraZ -= sin(cameraYaw * M_PI / 180.0) * moveDistance * deltaTime;
    }
    if (keyStates['d']) {
        cameraX += cos(cameraYaw * M_PI / 180.0) * moveDistance * deltaTime;
        cameraZ += sin(cameraYaw * M_PI / 180.0) * moveDistance * deltaTime;
    }
    if (keyStates['q']) {
        cameraY -= moveDistance * deltaTime;
    }
    if (keyStates['e']) {
        cameraY += moveDistance * deltaTime;
    }
    if (keyStates['+']) {
        cameraZ -= moveDistance * 5.0f;
    }
    if (keyStates['-']) {
        cameraZ += moveDistance * 5.0f;
    }
}

// Обробник клавіатури
void keyboardUp(unsigned char key, int x, int y) {
    keyStates[key] = false;

    switch (key) {
        case 27: // Клавіша Escape - вихід з програми
            exit(0);
            break;
        case ' ': // Пробіл - пауза/продовження анімації
            animationActive = !animationActive;
            break;
        case 'r': // Скидання позиції камери
            cameraX = 0.0f;
            cameraY = 0.0f;
            cameraZ = 20.0f;
            cameraYaw = 0.0f;
            cameraPitch = 0.0f;
            break;
        case 'f': // Перемикання режиму вільного огляду
            freeLookMode = !freeLookMode;
            if (freeLookMode) {
                glutSetCursor(GLUT_CURSOR_NONE); // Приховати курсор
            } else {
                glutSetCursor(GLUT_CURSOR_INHERIT); // Показати курсор
            }
            break;
        case '1': // Фонове освітлення
            glEnable(AMBIENT_LIGHT);
            glDisable(POINT_LIGHT);
            glDisable(DIRECTIONAL_LIGHT);
            glDisable(SPOTLIGHT);
            break;
        case '2':
            glDisable(AMBIENT_LIGHT);
            glEnable(POINT_LIGHT);
            glDisable(DIRECTIONAL_LIGHT);
            glDisable(SPOTLIGHT);
            break;
        case '3':
            glDisable(AMBIENT_LIGHT);
            glDisable(POINT_LIGHT);
            glEnable(DIRECTIONAL_LIGHT);
            glDisable(SPOTLIGHT);
            break;
        case '4':
            glDisable(AMBIENT_LIGHT);
            glDisable(POINT_LIGHT);
            glDisable(DIRECTIONAL_LIGHT);
            glEnable(SPOTLIGHT);
            break;
        case '5':
            enableFog = !enableFog;
            if (enableFog)
                glEnable(GL_FOG);
            else
                glDisable(GL_FOG);
            break;
    }
    glutPostRedisplay();
}

// Функція головного меню
void mainMenu(int id) {
    switch(id) {
        case 1: // Пуск/пауза анімації
            animationActive = !animationActive;
            break;
        case 2: // Скидання позиції камери
            cameraX = 0.0f;
            cameraY = 0.0f;
            cameraZ = 20.0f;
            cameraYaw = 0.0f;
            cameraPitch = 0.0f;
            break;
        case 3: // Перемикання режиму вільного огляду
            freeLookMode = !freeLookMode;
            if (freeLookMode) {
                glutSetCursor(GLUT_CURSOR_NONE); // Приховати курсор
            } else {
                glutSetCursor(GLUT_CURSOR_INHERIT); // Показати курсор
            }
            break;
        case 4: // Вихід з програми
            exit(0);
            break;
    }
    glutPostRedisplay();
}

// Головна функція
int main(int argc, char** argv) {
    // Ініціалізація GLUT
    glutInit(&argc, argv);

    // Режим відображення
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);

    // Розмір вікна
    glutInitWindowSize(2048, 1024);

    // Позиція вікна
    glutInitWindowPosition(100, 100);

    // Створення вікна
    glutCreateWindow("Solar System");

    // Налаштування меню
    glutCreateMenu(mainMenu);
    glutAddMenuEntry("Animation Play/Pause", 1);
    glutAddMenuEntry("Reset Camera Position", 2);
    glutAddMenuEntry("Toggle Free View", 3);
    glutAddMenuEntry("Exit", 4);
    glutAttachMenu(GLUT_MIDDLE_BUTTON);

    // Реєстрація функцій-обробників
    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutKeyboardFunc(keyboardDown);
    glutKeyboardUpFunc(keyboardUp);
    glutMouseFunc(mouseFunc);
    glutMotionFunc(mouseMotionFunc);
    glutPassiveMotionFunc(mousePassiveMotionFunc);

    // Запуск анімації
    glutTimerFunc(16, animate, 0);

    // Ініціалізація OpenGL
    init();

    // Запуск головного циклу GLUT
    glutMainLoop();

    return 0;
}