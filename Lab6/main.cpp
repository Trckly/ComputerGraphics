#include <GL/glut.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include <stdio.h>
#define _USE_MATH_DEFINES
#include <cmath>
#include <chrono>
#include <vector>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

// Світлові джерела
#define MAIN_LIGHT GL_LIGHT0
#define SECOND_LIGHT GL_LIGHT1
#define THIRD_LIGHT GL_LIGHT2

// Контекстне меню константи
#define MENU_TOGGLE_MAIN_LIGHT 1
#define MENU_TOGGLE_SECOND_LIGHT 2
#define MENU_TOGGLE_THIRD_LIGHT 3
#define MENU_INCREASE_ROTATION_SPEED 4
#define MENU_DECREASE_ROTATION_SPEED 5
#define MENU_INCREASE_MOVE_SPEED 6
#define MENU_DECREASE_MOVE_SPEED 7
#define MENU_RESET_SETTINGS 8
#define MENU_TOGGLE_ANIMATION 9
#define MENU_EXIT 10

// Глобальні змінні
GLfloat planetRotation = 0.0f;         // Обертання планети навколо своєї осі
GLfloat planetRevolution = 0.0f;       // Обертання планети навколо зірки
GLfloat zoomFactor = -20.0f;           // Рівень масштабування сцени

// Ідентифікатори текстур
GLuint starTexture;
GLuint moonTexture;

// Камера
float cameraX = 0.0f, cameraY = 0.0f, cameraZ = 20.0f;

// Часові параметри анімації
double currentTime = 0.0, previousTime = 0.0, deltaTime = 0.0;
float rotationSpeed = 60.0f;  // градусів за секунду (базова швидкість)
float moveSpeed = 5.0f;      // одиниць за секунду (базова швидкість)
bool animationActive = true;

// Режим вільного огляду
bool freeLookMode = true;

// Стан клавіатури
bool keyStates[256] = { false };

// Стан освітлення
bool mainLightEnabled = true;
bool secondLightEnabled = false;
bool thirdLightEnabled = false;

// Межі сцени
const float sceneBounds[6] = {-15.0f, 15.0f, -10.0f, 10.0f, -15.0f, 15.0f}; // xmin, xmax, ymin, ymax, zmin, zmax

// Структура для об'єкта з ламаною траєкторією
struct MovingObject {
    float x, y, z;        // Позиція
    float dx, dy, dz;     // Напрямок/швидкість
    float size;           // Розмір
    float rotX, rotY, rotZ; // Кути обертання

    // Ініціалізація з випадковою позицією та напрямком
    MovingObject() {
        x = (float)rand() / RAND_MAX * 10.0f - 5.0f; // -5 до 5
        y = (float)rand() / RAND_MAX * 6.0f - 3.0f;  // -3 до 3
        z = (float)rand() / RAND_MAX * 10.0f - 5.0f; // -5 до 5

        dx = ((float)rand() / RAND_MAX * 2.0f - 1.0f);
        dy = ((float)rand() / RAND_MAX * 2.0f - 1.0f);
        dz = ((float)rand() / RAND_MAX * 2.0f - 1.0f);

        // Нормалізація вектора швидкості
        float len = sqrt(dx*dx + dy*dy + dz*dz);
        dx /= len; dy /= len; dz /= len;

        size = 0.3f + (float)rand() / RAND_MAX * 0.5f; // 0.3 до 0.8
        rotX = rotY = rotZ = 0.0f;
    }

    // Оновлення позиції із відбиванням від меж
    void update(float speed, float deltaTime) {
        // Оновлення позиції
        x += dx * speed * deltaTime;
        y += dy * speed * deltaTime;
        z += dz * speed * deltaTime;

        // Відбивання від меж (всі 6 площин сцени)
        // X-межі
        if (x < sceneBounds[0] || x > sceneBounds[1]) {
            dx = -dx;
            // Зміна напрямку (для ламаної траєкторії)
            if (rand() % 4 == 0) {
                dy += ((float)rand() / RAND_MAX * 0.4f - 0.2f);
                dz += ((float)rand() / RAND_MAX * 0.4f - 0.2f);
                // Нормалізація
                float len = sqrt(dx*dx + dy*dy + dz*dz);
                dx /= len; dy /= len; dz /= len;
            }
        }

        // Y-межі
        if (y < sceneBounds[2] || y > sceneBounds[3]) {
            dy = -dy;
            // Зміна напрямку
            if (rand() % 4 == 0) {
                dx += ((float)rand() / RAND_MAX * 0.4f - 0.2f);
                dz += ((float)rand() / RAND_MAX * 0.4f - 0.2f);
                // Нормалізація
                float len = sqrt(dx*dx + dy*dy + dz*dz);
                dx /= len; dy /= len; dz /= len;
            }
        }

        // Z-межі
        if (z < sceneBounds[4] || z > sceneBounds[5]) {
            dz = -dz;
            // Зміна напрямку
            if (rand() % 4 == 0) {
                dx += ((float)rand() / RAND_MAX * 0.4f - 0.2f);
                dy += ((float)rand() / RAND_MAX * 0.4f - 0.2f);
                // Нормалізація
                float len = sqrt(dx*dx + dy*dy + dz*dz);
                dx /= len; dy /= len; dz /= len;
            }
        }

        // Оновлення обертання
        rotX += 15.0f * deltaTime;
        rotY += 20.0f * deltaTime;
        rotZ += 10.0f * deltaTime;
    }
};

// Вектор для об'єктів з ламаною траєкторією
std::vector<MovingObject> movingObjects;

// Прототипи функцій
GLuint loadTexture(const char* filename);
void createMenu();
void menuCallback(int value);

// Отримання поточного часу в секундах
double getTimeInSeconds() {
    auto now = std::chrono::high_resolution_clock::now();
    auto duration = now.time_since_epoch();
    return std::chrono::duration<double>(duration).count();
}

// Завантаження текстури з файлу
GLuint loadTexture(const char* filename) {
    GLuint texture;
    int width, height, channels;
    unsigned char* data;

    stbi_set_flip_vertically_on_load(1);
    data = stbi_load(filename, &width, &height, &channels, STBI_rgb_alpha);

    if (!data) {
        printf("Помилка завантаження текстури '%s': %s\n", filename, stbi_failure_reason());
        return 0;
    }

    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    gluBuild2DMipmaps(GL_TEXTURE_2D, GL_RGBA, width, height,
                      GL_RGBA, GL_UNSIGNED_BYTE, data);

    stbi_image_free(data);
    return texture;
}

// Ініціалізація OpenGL
void init(void) {
    // Встановлення чорного фону (космос)
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

    // Приховування курсору в режимі вільного огляду
    glutSetCursor(GLUT_CURSOR_NONE);

    // Ввімкнення тесту глибини для правильного 3D-рендерингу
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_SMOOTH);

    // Ввімкнення освітлення
    glEnable(GL_LIGHTING);
    glEnable(GL_NORMALIZE);

    // Глобальне фонове освітлення
    GLfloat global_ambient[] = {0.1f, 0.1f, 0.1f, 1.0f};
    glLightModelfv(GL_LIGHT_MODEL_AMBIENT, global_ambient);

    // Параметри матеріалу за замовчуванням
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

    // Створення рухомих об'єктів
    for (int i = 0; i < 10; i++) {
        movingObjects.push_back(MovingObject());
    }

    // Створення контекстного меню
    createMenu();

    // Ініціалізація таймера
    previousTime = getTimeInSeconds();
}

// Налаштування освітлення
void setupLighting() {
    // Світло в центрі (зірка)
    GLfloat light_position[] = {0.0f, 0.0f, 0.0f, 1.0f};
    GLfloat light_ambient[] = {0.2f, 0.2f, 0.2f, 1.0f};
    GLfloat light_diffuse[] = {1.0f, 1.0f, 0.6f, 1.0f};
    GLfloat light_specular[] = {1.0f, 1.0f, 1.0f, 1.0f};

    glLightfv(MAIN_LIGHT, GL_POSITION, light_position);
    glLightfv(MAIN_LIGHT, GL_AMBIENT, light_ambient);
    glLightfv(MAIN_LIGHT, GL_DIFFUSE, light_diffuse);
    glLightfv(MAIN_LIGHT, GL_SPECULAR, light_specular);

    // Затухання світла
    glLightf(MAIN_LIGHT, GL_CONSTANT_ATTENUATION, 1.0f);
    glLightf(MAIN_LIGHT, GL_LINEAR_ATTENUATION, 0.01f);
    glLightf(MAIN_LIGHT, GL_QUADRATIC_ATTENUATION, 0.0005f);

    // Друге світло (червоне)
    GLfloat light2_position[] = {-10.0f, 8.0f, 5.0f, 1.0f};
    GLfloat light2_ambient[] = {0.05f, 0.0f, 0.0f, 1.0f};
    GLfloat light2_diffuse[] = {1.0f, 0.2f, 0.2f, 1.0f};
    GLfloat light2_specular[] = {1.0f, 0.4f, 0.4f, 1.0f};

    glLightfv(SECOND_LIGHT, GL_POSITION, light2_position);
    glLightfv(SECOND_LIGHT, GL_AMBIENT, light2_ambient);
    glLightfv(SECOND_LIGHT, GL_DIFFUSE, light2_diffuse);
    glLightfv(SECOND_LIGHT, GL_SPECULAR, light2_specular);

    // Затухання другого світла
    glLightf(SECOND_LIGHT, GL_CONSTANT_ATTENUATION, 1.0f);
    glLightf(SECOND_LIGHT, GL_LINEAR_ATTENUATION, 0.02f);
    glLightf(SECOND_LIGHT, GL_QUADRATIC_ATTENUATION, 0.001f);

    // Третє світло (синє)
    GLfloat light3_position[] = {10.0f, -8.0f, -5.0f, 1.0f};
    GLfloat light3_ambient[] = {0.0f, 0.0f, 0.05f, 1.0f};
    GLfloat light3_diffuse[] = {0.2f, 0.2f, 1.0f, 1.0f};
    GLfloat light3_specular[] = {0.4f, 0.4f, 1.0f, 1.0f};

    glLightfv(THIRD_LIGHT, GL_POSITION, light3_position);
    glLightfv(THIRD_LIGHT, GL_AMBIENT, light3_ambient);
    glLightfv(THIRD_LIGHT, GL_DIFFUSE, light3_diffuse);
    glLightfv(THIRD_LIGHT, GL_SPECULAR, light3_specular);

    // Затухання третього світла
    glLightf(THIRD_LIGHT, GL_CONSTANT_ATTENUATION, 1.0f);
    glLightf(THIRD_LIGHT, GL_LINEAR_ATTENUATION, 0.02f);
    glLightf(THIRD_LIGHT, GL_QUADRATIC_ATTENUATION, 0.001f);

    // Увімкнення/вимкнення джерел світла відповідно до налаштувань
    if (mainLightEnabled) glEnable(MAIN_LIGHT);
    else glDisable(MAIN_LIGHT);

    if (secondLightEnabled) glEnable(SECOND_LIGHT);
    else glDisable(SECOND_LIGHT);

    if (thirdLightEnabled) glEnable(THIRD_LIGHT);
    else glDisable(THIRD_LIGHT);
}

// Малювання зірки
void drawStar() {
    glPushAttrib(GL_LIGHTING_BIT);

    // Зробити зірку самосвітною
    GLfloat mat_emission[] = {1.0f, 0.9f, 0.5f, 1.0f};
    glMaterialfv(GL_FRONT, GL_EMISSION, mat_emission);

    // Застосування текстури зірки
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, starTexture);

    glPushMatrix();
    glRotatef(90.0f, 1.0f, 0.0f, 0.0f);

    // Малювання сфери зірки
    GLUquadricObj *quadric = gluNewQuadric();
    gluQuadricTexture(quadric, GL_TRUE);
    gluSphere(quadric, 2.0f, 30, 30);
    gluDeleteQuadric(quadric);

    glPopMatrix();
    glDisable(GL_TEXTURE_2D);
    glPopAttrib();
}

// Малювання планети
void drawPlanet() {
    glPushAttrib(GL_LIGHTING_BIT);

    // Встановлення матеріалу планети
    GLfloat mat_ambient[] = {0.4f, 0.4f, 0.4f, 1.0f};
    GLfloat mat_diffuse[] = {0.8f, 0.8f, 0.8f, 1.0f};
    GLfloat mat_specular[] = {0.5f, 0.5f, 0.5f, 1.0f};
    GLfloat mat_shininess[] = {25.0f};
    GLfloat mat_emission[] = {0.0f, 0.0f, 0.0f, 1.0f}; // Без самосвітіння

    glMaterialfv(GL_FRONT, GL_AMBIENT, mat_ambient);
    glMaterialfv(GL_FRONT, GL_DIFFUSE, mat_diffuse);
    glMaterialfv(GL_FRONT, GL_SPECULAR, mat_specular);
    glMaterialfv(GL_FRONT, GL_SHININESS, mat_shininess);
    glMaterialfv(GL_FRONT, GL_EMISSION, mat_emission);

    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, moonTexture);

    glPushMatrix();
    glRotatef(90.0f, 1.0f, 0.0f, 0.0f);

    // Створення сфери планети
    GLUquadricObj *quadric = gluNewQuadric();
    gluQuadricTexture(quadric, GL_TRUE);
    gluSphere(quadric, 0.8f, 20, 20);
    gluDeleteQuadric(quadric);

    glPopMatrix();
    glDisable(GL_TEXTURE_2D);
    glPopAttrib();
}

// Малювання рухомого об'єкта
void drawMovingObject(const MovingObject& obj) {
    glPushAttrib(GL_LIGHTING_BIT);

    // Різні кольори для різних об'єктів
    GLfloat r = 0.3f + ((float)(&obj - &movingObjects[0]) / movingObjects.size()) * 0.7f;
    GLfloat g = 0.8f - ((float)(&obj - &movingObjects[0]) / movingObjects.size()) * 0.5f;
    GLfloat b = ((float)(&obj - &movingObjects[0]) / movingObjects.size()) * 0.8f;

    GLfloat mat_ambient[] = {r * 0.4f, g * 0.4f, b * 0.4f, 1.0f};
    GLfloat mat_diffuse[] = {r, g, b, 1.0f};
    GLfloat mat_specular[] = {0.8f, 0.8f, 0.8f, 1.0f};
    GLfloat mat_shininess[] = {40.0f};

    glMaterialfv(GL_FRONT, GL_AMBIENT, mat_ambient);
    glMaterialfv(GL_FRONT, GL_DIFFUSE, mat_diffuse);
    glMaterialfv(GL_FRONT, GL_SPECULAR, mat_specular);
    glMaterialfv(GL_FRONT, GL_SHININESS, mat_shininess);

    glPushMatrix();
    glTranslatef(obj.x, obj.y, obj.z);
    glRotatef(obj.rotX, 1.0f, 0.0f, 0.0f);
    glRotatef(obj.rotY, 0.0f, 1.0f, 0.0f);
    glRotatef(obj.rotZ, 0.0f, 0.0f, 1.0f);

    // Малювання об'єкта (октаедр)
    glBegin(GL_TRIANGLES);
    // Верхня частина
    glNormal3f(0.0f, 1.0f, 0.0f);
    glVertex3f(0.0f, obj.size, 0.0f);
    glVertex3f(-obj.size, 0.0f, 0.0f);
    glVertex3f(0.0f, 0.0f, obj.size);

    glNormal3f(0.0f, 1.0f, 0.0f);
    glVertex3f(0.0f, obj.size, 0.0f);
    glVertex3f(0.0f, 0.0f, obj.size);
    glVertex3f(obj.size, 0.0f, 0.0f);

    glNormal3f(0.0f, 1.0f, 0.0f);
    glVertex3f(0.0f, obj.size, 0.0f);
    glVertex3f(obj.size, 0.0f, 0.0f);
    glVertex3f(0.0f, 0.0f, -obj.size);

    glNormal3f(0.0f, 1.0f, 0.0f);
    glVertex3f(0.0f, obj.size, 0.0f);
    glVertex3f(0.0f, 0.0f, -obj.size);
    glVertex3f(-obj.size, 0.0f, 0.0f);

    // Нижня частина
    glNormal3f(0.0f, -1.0f, 0.0f);
    glVertex3f(0.0f, -obj.size, 0.0f);
    glVertex3f(-obj.size, 0.0f, 0.0f);
    glVertex3f(0.0f, 0.0f, obj.size);

    glNormal3f(0.0f, -1.0f, 0.0f);
    glVertex3f(0.0f, -obj.size, 0.0f);
    glVertex3f(0.0f, 0.0f, obj.size);
    glVertex3f(obj.size, 0.0f, 0.0f);

    glNormal3f(0.0f, -1.0f, 0.0f);
    glVertex3f(0.0f, -obj.size, 0.0f);
    glVertex3f(obj.size, 0.0f, 0.0f);
    glVertex3f(0.0f, 0.0f, -obj.size);

    glNormal3f(0.0f, -1.0f, 0.0f);
    glVertex3f(0.0f, -obj.size, 0.0f);
    glVertex3f(0.0f, 0.0f, -obj.size);
    glVertex3f(-obj.size, 0.0f, 0.0f);
    glEnd();

    glPopMatrix();
    glPopAttrib();
}

// Малювання меж сцени (для демонстрації)
void drawSceneBounds() {
    glPushAttrib(GL_LIGHTING_BIT | GL_LINE_BIT);
    glDisable(GL_LIGHTING);

    glColor3f(0.2f, 0.2f, 0.2f);
    glLineWidth(1.0f);

    glBegin(GL_LINE_LOOP);
    glVertex3f(sceneBounds[0], sceneBounds[2], sceneBounds[4]);
    glVertex3f(sceneBounds[1], sceneBounds[2], sceneBounds[4]);
    glVertex3f(sceneBounds[1], sceneBounds[3], sceneBounds[4]);
    glVertex3f(sceneBounds[0], sceneBounds[3], sceneBounds[4]);
    glEnd();

    glBegin(GL_LINE_LOOP);
    glVertex3f(sceneBounds[0], sceneBounds[2], sceneBounds[5]);
    glVertex3f(sceneBounds[1], sceneBounds[2], sceneBounds[5]);
    glVertex3f(sceneBounds[1], sceneBounds[3], sceneBounds[5]);
    glVertex3f(sceneBounds[0], sceneBounds[3], sceneBounds[5]);
    glEnd();

    glBegin(GL_LINES);
    glVertex3f(sceneBounds[0], sceneBounds[2], sceneBounds[4]);
    glVertex3f(sceneBounds[0], sceneBounds[2], sceneBounds[5]);

    glVertex3f(sceneBounds[1], sceneBounds[2], sceneBounds[4]);
    glVertex3f(sceneBounds[1], sceneBounds[2], sceneBounds[5]);

    glVertex3f(sceneBounds[1], sceneBounds[3], sceneBounds[4]);
    glVertex3f(sceneBounds[1], sceneBounds[3], sceneBounds[5]);

    glVertex3f(sceneBounds[0], sceneBounds[3], sceneBounds[4]);
    glVertex3f(sceneBounds[0], sceneBounds[3], sceneBounds[5]);
    glEnd();

    glPopAttrib();
}

// Функція відображення
void display(void) {
    // Обчислення deltaTime
    currentTime = getTimeInSeconds();
    deltaTime = currentTime - previousTime;
    previousTime = currentTime;

    // Налаштування освітлення
    setupLighting();

    // Очищення буферів
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Налаштування камери
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    gluLookAt(cameraX, cameraY, cameraZ,
              0.0f, 0.0f, 0.0f,
              0.0, 1.0, 0.0);

    // Малювання зірки в центрі
    drawStar();

    // Малювання планети на орбіті
    glPushMatrix();

    // Обертання орбіти
    glRotatef(planetRevolution, 0.0f, 1.0f, 0.0f);

    // Відстань від зірки
    glTranslatef(8.0f, 0.0f, 0.0f);

    // Нахил осі планети (23.5 градусів)
    glRotatef(-23.5f, 0.0f, 0.0f, 1.0f);

    // Обертання планети навколо своєї осі
    glRotatef(planetRotation, 0.0f, 1.0f, 0.0f);

    drawPlanet();
    glPopMatrix();

    // Малювання рухомих об'єктів з ламаною траєкторією
    for (const auto& obj : movingObjects) {
        drawMovingObject(obj);
    }

    // Малювання меж сцени
    drawSceneBounds();

    // Заміна буферів
    glutSwapBuffers();
}

// Функція зміни розміру вікна
void reshape(int width, int height) {
    height = (height == 0) ? 1 : height;

    glViewport(0, 0, width, height);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(50.0f, (GLfloat)width / (GLfloat)height, 0.1f, 100.0f);

    glMatrixMode(GL_MODELVIEW);
}

// Функція анімації
void animate(int value) {
    if (animationActive) {
        // Оновлення кутів обертання
        planetRotation += rotationSpeed * deltaTime;
        if (planetRotation > 360.0f) planetRotation -= 360.0f;

        planetRevolution += rotationSpeed * 0.1f * deltaTime;
        if (planetRevolution > 360.0f) planetRevolution -= 360.0f;

        // Оновлення положення рухомих об'єктів
        for (auto& obj : movingObjects) {
            obj.update(moveSpeed, deltaTime);
        }
    }

    glutPostRedisplay();
    glutTimerFunc(16, animate, 0); // ~60 кадрів/сек
}

// Обробник натискання клавіш
void keyboardDown(unsigned char key, int x, int y) {
    keyStates[key] = true;
}

// Обробник відпускання клавіш
void keyboardUp(unsigned char key, int x, int y) {
    keyStates[key] = false;
}
// Оновлення камери на основі натиснутих клавіш
void updateCamera() {
    const float cameraSpeed = moveSpeed * deltaTime;

    if (freeLookMode) {
        // Рух камери вперед-назад
        if (keyStates['w']) {
            cameraZ -= cameraSpeed;
        }
        if (keyStates['s']) {
            cameraZ += cameraSpeed;
        }

        // Рух камери вліво-вправо
        if (keyStates['a']) {
            cameraX -= cameraSpeed;
        }
        if (keyStates['d']) {
            cameraX += cameraSpeed;
        }

        // Рух камери вгору-вниз
        if (keyStates['q']) {
            cameraY += cameraSpeed;
        }
        if (keyStates['e']) {
            cameraY -= cameraSpeed;
        }

        // Повернення до початкового положення
        if (keyStates['r']) {
            cameraX = 0.0f;
            cameraY = 0.0f;
            cameraZ = 20.0f;
        }
    }
}

// Обробник переміщення миші (для режиму вільного огляду)
void mouseMotion(int x, int y) {
    static int lastX = -1;
    static int lastY = -1;

    if (lastX == -1) {
        lastX = x;
        lastY = y;
        return;
    }

    // Центр вікна
    int width = glutGet(GLUT_WINDOW_WIDTH);
    int height = glutGet(GLUT_WINDOW_HEIGHT);
    int centerX = width / 2;
    int centerY = height / 2;

    if (freeLookMode) {
        float sensitivity = 0.1f;
        float dx = (x - lastX) * sensitivity;
        float dy = (y - lastY) * sensitivity;

        // Поворот камери (в майбутніх версіях, якщо потрібно)
        // Тут можна реалізувати обертання камери за курсором

        // Повернення курсору в центр вікна
        if (x != centerX || y != centerY) {
            glutWarpPointer(centerX, centerY);
            lastX = centerX;
            lastY = centerY;
        }
    } else {
        lastX = x;
        lastY = y;
    }
}

// Обробник контекстного меню
void menuCallback(int value) {
    switch (value) {
        case MENU_TOGGLE_MAIN_LIGHT:
            mainLightEnabled = !mainLightEnabled;
            break;
        case MENU_TOGGLE_SECOND_LIGHT:
            secondLightEnabled = !secondLightEnabled;
            break;
        case MENU_TOGGLE_THIRD_LIGHT:
            thirdLightEnabled = !thirdLightEnabled;
            break;
        case MENU_INCREASE_ROTATION_SPEED:
            rotationSpeed *= 1.5f;
            break;
        case MENU_DECREASE_ROTATION_SPEED:
            rotationSpeed /= 1.5f;
            break;
        case MENU_INCREASE_MOVE_SPEED:
            moveSpeed *= 1.5f;
            break;
        case MENU_DECREASE_MOVE_SPEED:
            moveSpeed /= 1.5f;
            break;
        case MENU_RESET_SETTINGS:
            // Скидання всіх налаштувань до початкових значень
            rotationSpeed = 60.0f;
            moveSpeed = 5.0f;
            mainLightEnabled = true;
            secondLightEnabled = false;
            thirdLightEnabled = false;
            break;
        case MENU_TOGGLE_ANIMATION:
            animationActive = !animationActive;
            break;
        case MENU_EXIT:
            exit(0);
            break;
    }
    glutPostRedisplay();
}

// Створення контекстного меню
void createMenu() {
    int menu = glutCreateMenu(menuCallback);

    // Додавання пунктів меню
    glutAddMenuEntry("Toggle main lighting", MENU_TOGGLE_MAIN_LIGHT);
    glutAddMenuEntry("toggle additional red light", MENU_TOGGLE_SECOND_LIGHT);
    glutAddMenuEntry("Toggle additional blue light", MENU_TOGGLE_THIRD_LIGHT);
    glutAddMenuEntry("--------------", 0);
    glutAddMenuEntry("Increase rotation speed", MENU_INCREASE_ROTATION_SPEED);
    glutAddMenuEntry("Decrease rotation speed", MENU_DECREASE_ROTATION_SPEED);
    glutAddMenuEntry("--------------", 0);
    glutAddMenuEntry("Increase movement speed", MENU_INCREASE_MOVE_SPEED);
    glutAddMenuEntry("Decrease movement speed", MENU_DECREASE_MOVE_SPEED);
    glutAddMenuEntry("--------------", 0);
    glutAddMenuEntry("Toggle animation", MENU_TOGGLE_ANIMATION);
    glutAddMenuEntry("Reset settings", MENU_RESET_SETTINGS);
    glutAddMenuEntry("Exit", MENU_EXIT);

    // Прив'язка меню до правої кнопки миші
    glutAttachMenu(GLUT_RIGHT_BUTTON);
}

// Функція для обробки клавіш керування (натискання)
void handleKeyboardInput() {
    float cameraSpeed = moveSpeed * deltaTime;

    // Оновлення камери на основі клавіш
    updateCamera();

    // Додаткові команди керування
    if (keyStates['p']) {
        animationActive = !animationActive;
        keyStates['p'] = false; // щоб уникнути повторних перемикань
    }

    if (keyStates['f']) {
        freeLookMode = !freeLookMode;
        keyStates['f'] = false; // щоб уникнути повторних перемикань

        // Зміна видимості курсору залежно від режиму
        glutSetCursor(freeLookMode ? GLUT_CURSOR_NONE : GLUT_CURSOR_INHERIT);
    }

    if (keyStates['1']) {
        mainLightEnabled = !mainLightEnabled;
        keyStates['1'] = false;
    }

    if (keyStates['2']) {
        secondLightEnabled = !secondLightEnabled;
        keyStates['2'] = false;
    }

    if (keyStates['3']) {
        thirdLightEnabled = !thirdLightEnabled;
        keyStates['3'] = false;
    }
}

// Функція простоювання для постійного оновлення стану
void idle() {
    // Обробка натиснутих клавіш
    handleKeyboardInput();

    // Потрібно перемалювати сцену
    glutPostRedisplay();
}

// Головна функція
int main(int argc, char** argv) {
    // Ініціалізація GLUT
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize(800, 600);
    glutCreateWindow("Космічна сцена з OpenGL");

    glutFullScreen();

    // Реєстрація обробників подій
    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutKeyboardFunc(keyboardDown);
    glutKeyboardUpFunc(keyboardUp);
    glutPassiveMotionFunc(mouseMotion);
    glutIdleFunc(idle);

    // Ініціалізація OpenGL
    init();

    // Запуск анімації
    glutTimerFunc(16, animate, 0);

    // Основний цикл GLUT
    glutMainLoop();

    return 0;
}