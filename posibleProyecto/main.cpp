#include "global.h"
#include "shaders.h"
#include "systemInfo.h"
#include "camera.h"
#include "MeshLibrary.h"
#include "Input.h"
#include "Renderer.h"
#include "Transform.h"
#include "Entidad.h"
#include "drawObjects.h"
#include "Paths.h"
#include "ShadowSystem.h"
#include "AssetLoadData.h"
#include "MapData.h"
#include "webPage.h"
#include "jsonManager.h"
#include "simulationData.h"
#include "CombatSimulation.h"
#include <algorithm>

int shouldContinue(GLFWwindow* window) {
    return glfwWindowShouldClose(window) == 0;
}

void updateFrame(GLFWwindow* window) {
    glfwSwapBuffers(window);
    glfwPollEvents();
} 

// Comprueba las teclas que controlan las sombras, es un ejemplo, lo dejamos por si en algun momento lo 
// usamos de base
/*
void updateShadowInput(GLFWwindow* window, ShadowSystem& shadows, std::vector<SceneLight>& luces) {
    static bool f5PreviouslyPressed = false;
    static bool f6PreviouslyPressed = false;
    static bool f7PreviouslyPressed = false;

    bool f5Pressed = glfwGetKey(window, GLFW_KEY_F5) == GLFW_PRESS;
    bool f6Pressed = glfwGetKey(window, GLFW_KEY_F6) == GLFW_PRESS;
    bool f7Pressed = glfwGetKey(window, GLFW_KEY_F7) == GLFW_PRESS;

    if (f5Pressed && !f5PreviouslyPressed)
        shadows.setEnabled(!shadows.isEnabled());

    if (f6Pressed && !f6PreviouslyPressed && luces.size() > 0)
        luces[0].castsShadow = !luces[0].castsShadow;

    if (f7Pressed && !f7PreviouslyPressed && luces.size() > 1)
        luces[1].castsShadow = !luces[1].castsShadow;

    f5PreviouslyPressed = f5Pressed;
    f6PreviouslyPressed = f6Pressed;
    f7PreviouslyPressed = f7Pressed;
}*/
 
std::vector<SceneLight> setLights() {
    std::vector<SceneLight> lights = {
        {
            {
                glm::vec3(0.0f),
                8.0f,
                glm::vec3(1.0f, 0.8f, 0.5f),
                2.0f
            },
            true  // Luz que orbita: proyecta sombras
        },
        {
            {
                glm::vec3(-3.0f, 2.0f, 0.0f),
                6.0f,
                glm::vec3(0.3f, 0.5f, 1.0f),
                2.0f
            },
            false // Luz fija: no proyecta sombras
        },
        {
            {
                glm::vec3(-80.0f, 200.0f, -60.0f),   // sol: muy alto y lejos, asi su luz llega casi paralela a toda la arena
                1000.0f,                             // alcance muy por encima de la distancia: apenas se atenua
                glm::vec3(1.0f, 0.95f, 0.85f),       // blanco calido
                1.2f
            },
            false   // sin sombras: un cubemap repartido en 100 m da sombras muy bastas
        }
    };
    return lights;
}

constexpr float MAX_SECONDS_PER_SCREEN_FRAME = 0.25f;

void buclePrincipaLTiempoReal(GLFWwindow* window, SystemInfo& sysInfo) {
    simulationData simData;
    simData.init(window);

    // Parte temporar, cuando leamos el JSON de luces deberia hacerse en el init de simData
    std::vector<SceneLight> data = setLights();
    simData.changeLights(data);

    const float secondsPerCombatFrame = CombatSimulation::SECONDS_PER_TICK;   // el replay graba un frame por tick
    float unplayedSeconds = 0.0f;   // tiempo real que aun no se ha convertido en frames del combate

    while (shouldContinue(window)) {
        unplayedSeconds += (std::min) (sysInfo.getDeltaTime(), MAX_SECONDS_PER_SCREEN_FRAME);
        while (unplayedSeconds >= secondsPerCombatFrame) {
            simData.framesInfo.frame++;
            unplayedSeconds -= secondsPerCombatFrame;
        }
        if (simData.framesInfo.frame >= simData.framesInfo.combat.size()) {
            simData.framesInfo.frame = 0;
        }

        simData.procesMainBodyLoopRealTime(window, sysInfo);
    }

    simData.destroy();
}

int guranteWindowCreated(GLFWwindow* window) {
    if (!window) {
        std::cerr << "No se pudo crear la ventana\n";
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);

    glfwSwapInterval(1);   // una imagen por refresco del monitor: no satura la GPU y la grabacion no pierde imagenes

    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK) {
        std::cerr << "No se pudo inicializar GLEW\n";
        return -1;
    }

    std::cout << "OpenGL: " << glGetString(GL_VERSION) << "\n";
    return 0;
}

GLFWwindow* createWindow() {
    GLFWwindow* window = nullptr;

    if (!glfwInit()) {
        std::cerr << "No se pudo inicializar GLFW\n"; 
    }
    else
    {
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

        GLFWmonitor* monitor = glfwGetPrimaryMonitor();
        const GLFWvidmode* mode = glfwGetVideoMode(monitor);

        glfwWindowHint(GLFW_RED_BITS, mode->redBits);
        glfwWindowHint(GLFW_GREEN_BITS, mode->greenBits);
        glfwWindowHint(GLFW_BLUE_BITS, mode->blueBits);
        glfwWindowHint(GLFW_REFRESH_RATE, mode->refreshRate);

        glfwWindowHint(GLFW_DECORATED, GLFW_FALSE);   // sin bordes y del tamaño de la pantalla: parece pantalla completa pero se graba sin cortes
        window = glfwCreateWindow(mode->width, mode->height, "Battle Mage", nullptr, nullptr);
        if (window != nullptr) glfwSetWindowPos(window, 0, 0);
    }

    return window;
}

// TEMPORAL (L1): se borra cuando el cargador este comprobado
void testObjLoader() {
    const std::vector<std::string> testFiles = {
        "posibleProyecto/assets/test/cubo_quads_vt_vn.obj",
        "posibleProyecto/assets/test/cubo_quads_v_vn.obj",
        "posibleProyecto/assets/test/cubo_quads_sin_vn.obj",
        "posibleProyecto/assets/test/arbol_2_materiales.obj",
        "posibleProyecto/assets/test/malo_dos_vertices.obj",
        "posibleProyecto/assets/test/malo_indice_negativo.obj",
        "posibleProyecto/assets/test/malo_fuera_de_rango.obj",
        "posibleProyecto/assets/test/dos_objetos.obj",
        "posibleProyecto/assets/test/cubo_textura.obj"
    };

    for (const std::string& testFile : testFiles) {
        try {
            const std::vector<ObjPiece> pieces = loadObj(dataPath(testFile));
            std::cout << "[TEST] " << testFile << ": " << pieces.size() << " piezas\n";
            for (const ObjPiece& piece : pieces) {
                const glm::vec4& color = piece.material.color;
                std::cout << "    shape '" << piece.shapeName << "', material '" << piece.material.name
                    << "' color " << color.r << " " << color.g << " " << color.b
                    << " textura '" << piece.material.albedoTexture.string() << "': "
                    << piece.verts.size() / FLOATS_PER_VERTEX << " vertices / " << piece.idx.size() << " indices\n";
            }
        }
        catch (const std::exception& ex) {
            std::cout << "[TEST] ERROR " << ex.what() << "\n";
        }
    }
}

int createSimulation() {
    GLFWwindow* window = createWindow();

    SystemInfo sysInfo; // Se tiene que hacer despues de createWindow, respetar orden

    if (window == nullptr)
        return -1;

    sysInfo.getCurrentState(window);

    if (guranteWindowCreated(window) != 0)
        return -1;

    glViewport(0, 0, sysInfo.getWidht(), sysInfo.getHeight());

    buclePrincipaLTiempoReal(window, sysInfo);

    glfwTerminate();

    return 0;
}

// Simula el combate entero sin graficos y lo deja grabado para que los graficos lo reproduzcan
void simulateCombatToReplay() {
    CombatSimulation simulation;
    simulation.setup(dataPath("db/combat.json"));
    while (!simulation.isFinished())
        simulation.step();
    simulation.printResult();
    simulation.recordEnding();
    simulation.writeReplay(dataPath("db/combate_generado.json"));
}

int main()
{
    //sendForm();   // cuando se active: antes de simular, porque es el formulario el que escribe combat.json
    simulateCombatToReplay();
    return createSimulation();
}