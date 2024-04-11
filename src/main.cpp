#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <learnopengl/filesystem.h>
#include <learnopengl/shader.h>
#include <learnopengl/camera.h>
#include <learnopengl/model.h>

#include <iostream>

void framebuffer_size_callback(GLFWwindow *window, int width, int height);

void mouse_callback(GLFWwindow *window, double xpos, double ypos);

void scroll_callback(GLFWwindow *window, double xoffset, double yoffset);

void processInput(GLFWwindow *window);

void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods);

unsigned int loadTexture(char const* path);

// settings
const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;

// camera

float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;

// timing
float deltaTime = 0.0f;
float lastFrame = 0.0f;

struct PointLight {
    glm::vec3 position;
    glm::vec3 ambient;
    glm::vec3 diffuse;
    glm::vec3 specular;

    float constant;
    float linear;
    float quadratic;
};

struct ProgramState {
    glm::vec3 clearColor = glm::vec3(0);
    bool ImGuiEnabled = false;
    bool grayscaleEnabled = false;
    Camera camera;
    bool CameraMouseMovementUpdateEnabled = true;
    glm::vec3 roomPosition = glm::vec3(0.0f);
    float roomScale = 0.5f;
    glm::vec3 horsePosition = glm::vec3(-37.0f, 0.0f, -35.0f);
    float horseScale = 0.05f;
    glm::vec3 lightbeamPos = glm::vec3(0.0f, 0.0f, 0.0f);
    PointLight pointLight1;
    PointLight pointLight2;
    PointLight pointLight3;
    ProgramState()
            : camera(glm::vec3(0.0f, 0.0f, 3.0f)) {}

    void SaveToFile(std::string filename);

    void LoadFromFile(std::string filename);
};

void ProgramState::SaveToFile(std::string filename) {
    std::ofstream out(filename);
    out << clearColor.r << '\n'
        << clearColor.g << '\n'
        << clearColor.b << '\n'
        << ImGuiEnabled << '\n'
        << grayscaleEnabled << '\n'
        << camera.Position.x << '\n'
        << camera.Position.y << '\n'
        << camera.Position.z << '\n'
        << camera.Front.x << '\n'
        << camera.Front.y << '\n'
        << camera.Front.z << '\n';
}

void ProgramState::LoadFromFile(std::string filename) {
    std::ifstream in(filename);
    if (in) {
        in >> clearColor.r
           >> clearColor.g
           >> clearColor.b
           >> ImGuiEnabled
           >> grayscaleEnabled
           >> camera.Position.x
           >> camera.Position.y
           >> camera.Position.z
           >> camera.Front.x
           >> camera.Front.y
           >> camera.Front.z;
    }
}

ProgramState *programState;

void DrawImGui(ProgramState *programState);

int main() {
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
    GLFWwindow *window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "LearnOpenGL", NULL, NULL);
    if (window == NULL) {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwSetKeyCallback(window, key_callback);
    // tell GLFW to capture our mouse
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // glad: load all OpenGL function pointers
    // ---------------------------------------
    if (!gladLoadGLLoader((GLADloadproc) glfwGetProcAddress)) {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    programState = new ProgramState;
    programState->LoadFromFile("resources/program_state.txt");
    if (programState->ImGuiEnabled) {
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    }
    // Init Imgui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    (void) io;

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330 core");

    // configure global opengl state
    // -----------------------------
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);

    // build and compile shaders
    // -------------------------
    Shader ourShader("resources/shaders/2.model_lighting.vs", "resources/shaders/2.model_lighting.fs");
    Shader screenShader("resources/shaders/screenShader.vs", "resources/shaders/screenShader.fs");
    Shader blendingShader("resources/shaders/blendingShader.vs", "resources/shaders/blendingShader.fs");

    // load models
    // -----------
    Model roomModel("resources/objects/blacklodge/untitled.obj");
    roomModel.SetShaderTextureNamePrefix("material.");

    Model horseModel("resources/objects/horsie/horse.obj");
    horseModel.SetShaderTextureNamePrefix("material.");

    PointLight& pointLight1 = programState->pointLight1;
    pointLight1.position = glm::vec3(5.6f, 8.7f, 26.5f);
    pointLight1.ambient = glm::vec3(0.2, 0.2, 0.2);
    pointLight1.diffuse = glm::vec3(0.6, 0.6, 0.6);
    pointLight1.specular = glm::vec3(1.0, 1.0, 1.0);

    pointLight1.constant = 0.5f;
    pointLight1.linear = 0.09f;
    pointLight1.quadratic = 0.0036f;

    PointLight& pointLight2 = programState->pointLight2;
    pointLight2.position = glm::vec3(20.0f, 8.7f, 26.5f);
    pointLight2.ambient = glm::vec3(0.0, 0.0, 0.2);
    pointLight2.diffuse = glm::vec3(0.0, 0.0, 0.6);
    pointLight2.specular = glm::vec3(0.0, 0.0, 1.0);

    pointLight2.constant = 0.5f;
    pointLight2.linear = 0.09f;
    pointLight2.quadratic = 0.0036f;

    PointLight& pointLight3 = programState->pointLight3;
    pointLight3.position = glm::vec3(-37.0f, 4.0f, -35.0f);
    pointLight3.ambient = glm::vec3(0.2, 0.2, 0.2);
    pointLight3.diffuse = glm::vec3(0.6, 0.6, 0.6);
    pointLight3.specular = glm::vec3(1.0, 1.0, 1.0);

    pointLight3.constant = 0.5f;
    pointLight3.linear = 0.09f;
    pointLight3.quadratic = 0.0036f;

    float quadVertices[] = {
            -1.0f,  1.0f,   0.0f, 1.0f,
            -1.0f, -1.0f,   0.0f, 0.0f,
             1.0f, -1.0f,  1.0f,0.0f,

             -1.0f,  1.0f,  0.0f, 1.0f,
              1.0f, -1.0f,  1.0f, 0.0f,
               1.0f, 1.0f,  1.0f, 1.0f

    };

    float transparentVertices[] = {
            0.0f,  0.5f, 0.0f, 0.0f, 0.0f,
            0.0f, -0.5f, 0.0f, 0.0f, 1.0f,
            1.0f,-0.5f,0.0f,1.0f, 1.0f,

             0.0f, 0.5f, 0.0f, 0.0f, 0.0f,
             1.0f, -0.5f, 0.0f, 1.0f, 1.0f,
             1.0f, 0.5f, 0.0f, 1.0f, 0.0f
    };

    unsigned int quadVAO, quadVBO;
    glGenVertexArrays(1, &quadVAO);
    glGenBuffers(1, &quadVBO);
    glBindVertexArray(quadVAO);
    glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));

    unsigned int transparentVAO, transparentVBO;
    glGenVertexArrays(1, &transparentVAO);
    glGenBuffers(1, &transparentVBO);
    glBindVertexArray(transparentVAO);
    glBindBuffer(GL_ARRAY_BUFFER, transparentVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(transparentVertices), transparentVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    glBindVertexArray(0);

    unsigned int transparentTexture = loadTexture(FileSystem::getPath("resources/textures/light.png").c_str());

    vector<glm::vec3> lights{
        glm::vec3(-39.1f, 4.54f, -30.0f),
        glm::vec3(-35.1f, 4.54f, -30.0f)
    };

    blendingShader.use();
    blendingShader.setInt("texture1", 0);

    screenShader.use();

    screenShader.setInt("screenTexture", 0);
    screenShader.setBool("grayscaleEnabled", programState->grayscaleEnabled);

    //framebuffer
    unsigned int fbo;
    glGenFramebuffers(1, &fbo);

    glBindFramebuffer(GL_FRAMEBUFFER, fbo);

    unsigned int textureColorbuffer;
    glGenTextures(1, &textureColorbuffer);
    glBindTexture(GL_TEXTURE_2D, textureColorbuffer);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, textureColorbuffer, 0);

    unsigned int rbo;
    glGenRenderbuffers(1, &rbo);
    glBindRenderbuffer(GL_RENDERBUFFER, rbo);

    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, SCR_WIDTH, SCR_HEIGHT);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, rbo);

    if(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE){
        std::cout << "Framebuffer works!";
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // draw in wireframe
    //glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

    // render loop
    // -----------
    while (!glfwWindowShouldClose(window)) {
        // per-frame time logic
        // --------------------
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        // input
        // -----
        processInput(window);


        glBindFramebuffer(GL_FRAMEBUFFER, fbo);
        glEnable(GL_DEPTH_TEST);

        // render
        // ------
        glClearColor(programState->clearColor.r, programState->clearColor.g, programState->clearColor.b, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

        // don't forget to enable shader before setting uniforms

        ourShader.use();
        ourShader.setVec3("pointLight1.position", pointLight1.position);
        ourShader.setVec3("pointLight1.ambient", pointLight1.ambient);
        ourShader.setVec3("pointLight1.diffuse", pointLight1.diffuse);
        ourShader.setVec3("pointLight1.specular", pointLight1.specular);
        ourShader.setFloat("pointLight1.constant", pointLight1.constant);
        ourShader.setFloat("pointLight1.linear", pointLight1.linear);
        ourShader.setFloat("pointLight1.quadratic", pointLight1.quadratic);

        ourShader.setVec3("pointLight2.position", pointLight2.position);
        ourShader.setVec3("pointLight2.ambient", pointLight2.ambient);
        ourShader.setVec3("pointLight2.diffuse", pointLight2.diffuse);
        ourShader.setVec3("pointLight2.specular", pointLight2.specular);
        ourShader.setFloat("pointLight2.constant", pointLight2.constant);
        ourShader.setFloat("pointLight2.linear", pointLight2.linear);
        ourShader.setFloat("pointLight2.quadratic", pointLight2.quadratic);

        ourShader.setVec3("pointLight3.position", pointLight3.position);
        ourShader.setVec3("pointLight3.ambient", pointLight3.ambient);
        ourShader.setVec3("pointLight3.diffuse", pointLight3.diffuse);
        ourShader.setVec3("pointLight3.specular", pointLight3.specular);
        ourShader.setFloat("pointLight3.constant", pointLight3.constant);
        ourShader.setFloat("pointLight3.linear", pointLight3.linear);
        ourShader.setFloat("pointLight3.quadratic", pointLight3.quadratic);

        ourShader.setVec3("viewPosition", programState->camera.Position);
        ourShader.setFloat("material.shininess", 32.0f);

        // view/projection transformations
        glm::mat4 projection = glm::perspective(glm::radians(programState->camera.Zoom),
                                                (float) SCR_WIDTH / (float) SCR_HEIGHT, 0.1f, 100.0f);
        glm::mat4 view = programState->camera.GetViewMatrix();
        ourShader.setMat4("projection", projection);
        ourShader.setMat4("view", view);

        // render the loaded model
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model,
                               programState->roomPosition); // translate it down so it's at the center of the scene
        model = glm::scale(model, glm::vec3(programState->roomScale));
        ourShader.setMat4("model", model);
        roomModel.Draw(ourShader);

        model = glm::mat4(1.0f);
        model = glm::translate(model, programState->horsePosition);
        model = glm::scale(model, glm::vec3(programState->horseScale));
        ourShader.setMat4("model", model);
        horseModel.Draw(ourShader);

        glDisable(GL_CULL_FACE);
        blendingShader.use();
        blendingShader.setMat4("projection", projection);
        blendingShader.setMat4("view", view);

        std::sort(lights.begin(), lights.end(), [cameraPosition = programState->camera.Position](const glm::vec3& a, const glm::vec3& b) {
            float d1 = glm::distance(a, cameraPosition);
            float d2 = glm::distance(b, cameraPosition);
            return d1 > d2;
        });

        glBindVertexArray(transparentVAO);
        glBindTexture(GL_TEXTURE_2D, transparentTexture);
        for(int i = 0; i < lights.size(); i++){
            model = glm::mat4(1.0f);
            model = glm::translate(model, lights[i]);
            model = glm::rotate(model, glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
            model = glm::scale(model, glm::vec3(10.0f));
            blendingShader.setMat4("model", model);
            glDrawArrays(GL_TRIANGLES, 0, 6);
        }

        //blendingShader.setMat4("model", model);
        //glDrawArrays(GL_TRIANGLES, 0, 6);

        glEnable(GL_CULL_FACE);

        if (programState->ImGuiEnabled)
            DrawImGui(programState);

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glDisable(GL_DEPTH_TEST);

        glClear(GL_COLOR_BUFFER_BIT);

        screenShader.use();
        screenShader.setBool("grayscaleEnabled", programState->grayscaleEnabled);
        glBindVertexArray(quadVAO);
        glBindTexture(GL_TEXTURE_2D, textureColorbuffer);
        glDrawArrays(GL_TRIANGLES, 0, 6);

        // glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
        // -------------------------------------------------------------------------------
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    programState->SaveToFile("resources/program_state.txt");
    delete programState;
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    // glfw: terminate, clearing all previously allocated GLFW resources.
    // ------------------------------------------------------------------
    glfwTerminate();
    return 0;
}

// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
// ---------------------------------------------------------------------------------------------------------
void processInput(GLFWwindow *window) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        programState->camera.ProcessKeyboard(FORWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        programState->camera.ProcessKeyboard(BACKWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        programState->camera.ProcessKeyboard(LEFT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        programState->camera.ProcessKeyboard(RIGHT, deltaTime);
}

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
// ---------------------------------------------------------------------------------------------
void framebuffer_size_callback(GLFWwindow *window, int width, int height) {
    // make sure the viewport matches the new window dimensions; note that width and
    // height will be significantly larger than specified on retina displays.
    glViewport(0, 0, width, height);

}

// glfw: whenever the mouse moves, this callback is called
// -------------------------------------------------------
void mouse_callback(GLFWwindow *window, double xpos, double ypos) {
    if (firstMouse) {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos; // reversed since y-coordinates go from bottom to top

    lastX = xpos;
    lastY = ypos;

    if (programState->CameraMouseMovementUpdateEnabled)
        programState->camera.ProcessMouseMovement(xoffset, yoffset);
}

// glfw: whenever the mouse scroll wheel scrolls, this callback is called
// ----------------------------------------------------------------------
void scroll_callback(GLFWwindow *window, double xoffset, double yoffset) {
    programState->camera.ProcessMouseScroll(yoffset);
}

void DrawImGui(ProgramState *programState) {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();


    {
        ImGui::Begin("The Black Lodge");
        ImGui::Text("Press 'F' for grayscale mode");
        ImGui::ColorEdit3("Background color", (float *) &programState->clearColor);
        ImGui::DragFloat3("Room position", (float*)&programState->roomPosition);
        ImGui::DragFloat("Room scale", &programState->roomScale, 0.05, 0.1, 4.0);
        ImGui::DragFloat3("Horse position", (float*)&programState->horsePosition);
        ImGui::DragFloat("Horse scale", &programState->horseScale, 0.05, 0.1, 4.0);

        ImGui::End();
    }

    {
        ImGui::Begin("Camera info");
        const Camera& c = programState->camera;
        ImGui::Text("Camera position: (%f, %f, %f)", c.Position.x, c.Position.y, c.Position.z);
        ImGui::Text("(Yaw, Pitch): (%f, %f)", c.Yaw, c.Pitch);
        ImGui::Text("Camera front: (%f, %f, %f)", c.Front.x, c.Front.y, c.Front.z);
        ImGui::Checkbox("Camera mouse update", &programState->CameraMouseMovementUpdateEnabled);
        ImGui::End();
    }

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods) {
    if (key == GLFW_KEY_F1 && action == GLFW_PRESS) {
        programState->ImGuiEnabled = !programState->ImGuiEnabled;
        if (programState->ImGuiEnabled) {
            programState->CameraMouseMovementUpdateEnabled = false;
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        } else {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        }
    }

    if(key == GLFW_KEY_F && action == GLFW_PRESS){
        if(glfwGetKey(window, GLFW_KEY_F) == GLFW_PRESS)
            if(programState->grayscaleEnabled)
                programState->grayscaleEnabled = false;
            else
                programState->grayscaleEnabled = true;
    }
}

unsigned int loadTexture(char const* path){
    unsigned int textureID;
    glGenTextures(1, &textureID);

    stbi_set_flip_vertically_on_load(true);

    int width, height, nrComponents;
    unsigned char *data = stbi_load(path, &width, &height, &nrComponents, 0);
    if(data){
        GLenum format;
        if(nrComponents == 1)
            format = GL_RED;
        else if(nrComponents == 3)
            format = GL_RGB;
        else if(nrComponents == 4)
            format = GL_RGBA;

        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0 ,format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, format == GL_RGBA ? GL_CLAMP_TO_EDGE : GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, format == GL_RGBA ? GL_CLAMP_TO_EDGE : GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        stbi_image_free(data);
    }else{
        std:;cout << "Texture failed to load at path: " << path << '\n';
        stbi_image_free(data);
    }

    stbi_set_flip_vertically_on_load(false);

    return textureID;
}