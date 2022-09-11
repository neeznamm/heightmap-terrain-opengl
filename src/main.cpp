#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <shader_t.h>
#include <camera.h>
//#include <model.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include <iostream>
#include <vector>

#include "imgui.h"
#include "imgui_impl_opengl3.h"
#include "imgui_impl_glfw.h"


void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void key_callback(GLFWwindow* window, int key, int scancode, int action, int modifiers);
void processInput(GLFWwindow *window);
unsigned int loadTexture(const char *path);
unsigned int loadCubemap(std::vector<std::string> faces);

// settings
const unsigned int SCR_WIDTH = 1920;
const unsigned int SCR_HEIGHT = 1080;
const unsigned int NUM_PATCH_PTS = 4;

// camera
Camera camera(glm::vec3(67.0f, 15.5f, 169.9f),
              glm::vec3(0.0f, 1.0f, 0.0f),
              -128.1f, -42.4f);
float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;

// timing
float deltaTime = 0.0f;
float lastFrame = 0.0f;

int main()
{
    // glfw: initialize and configure
    // ------------------------------
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_SAMPLES, 4); //AA

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    // glfw window creation
    // --------------------
    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "KG Proektna", NULL, NULL);
    if (window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetKeyCallback(window, key_callback);

    // tell GLFW to capture our mouse
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);

    // glad: load all OpenGL function pointers
    // ---------------------------------------
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }
    
    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    // Setup Platform/Renderer bindings
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 410");
    // Setup Dear ImGui style
    ImGui::StyleColorsDark();

    //tesselation settings
    GLint maxTessLevel;
    glGetIntegerv(GL_MAX_TESS_GEN_LEVEL, &maxTessLevel);
    std::cout << "Max available tess level: " << maxTessLevel << std::endl;

    // configure global opengl state
    // -----------------------------
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);

    glEnable(GL_MULTISAMPLE);  //AA
    
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // build and compile our shader program
    // ------------------------------------
    Shader tessHeightMapShader("./src/shaders/vertex.shader",
                               "./src/shaders/fragment.shader", nullptr,
                               "./src/shaders/tessellation_control.shader",
                               "./src/shaders/tessellation_eval.shader");
    
    Shader cloudShader("./src/shaders/cloud_vertex.shader",
                               "./src/shaders/cloud_fragment.shader");

    Shader skyboxShader("./src/shaders/skybox_vertex.shader",
                               "./src/shaders/skybox_fragment.shader");

    //MODELS
    //Shader modelShader("src/shaders/model_v.shader", "src/shaders/model_f.shader");

    // load and create a texture
    // -------------------------
    unsigned int heightMap;
    glGenTextures(1, &heightMap);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, heightMap); // all upcoming GL_TEXTURE_2D operations now have effect on this texture object
    // set the texture wrapping parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);	// set texture wrapping to GL_REPEAT (default wrapping method)
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    // set texture filtering parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    // load image, create texture and generate mipmaps
    int width, height, nrChannels;
    unsigned char *data = stbi_load("./src/terrainmaps/heightmap.png", &width, &height, &nrChannels, 0);
    if (data)
    {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
        tessHeightMapShader.setInt("heightMap", 0);
        std::cout << "Loaded heightmap of size " << height << " x " << width << std::endl;
    }
    else
    {
        std::cout << "Failed to load texture" << std::endl;
    }
    stbi_image_free(data);

    // set up vertex data (and buffer(s)) and configure vertex attributes
    // ------------------------------------------------------------------
    std::vector<float> vertices;
    vertices.reserve(8180);

    unsigned rez = 20;
    for(unsigned i = 0; i <= rez-1; i++)
    {
        for(unsigned j = 0; j <= rez-1; j++)
        {
            vertices.push_back(-width/2.0f + width*i/(float)rez); // v.x
            vertices.push_back(0.0f); // v.y
            vertices.push_back(-height/2.0f + height*j/(float)rez); // v.z
            vertices.push_back(i / (float)rez); // u
            vertices.push_back(j / (float)rez); // v

            vertices.push_back(-width/2.0f + width*(i+1)/(float)rez); // v.x
            vertices.push_back(0.0f); // v.y
            vertices.push_back(-height/2.0f + height*j/(float)rez); // v.z
            vertices.push_back((i+1) / (float)rez); // u
            vertices.push_back(j / (float)rez); // v

            vertices.push_back(-width/2.0f + width*i/(float)rez); // v.x
            vertices.push_back(0.0f); // v.y
            vertices.push_back(-height/2.0f + height*(j+1)/(float)rez); // v.z
            vertices.push_back(i / (float)rez); // u
            vertices.push_back((j+1) / (float)rez); // v

            vertices.push_back(-width/2.0f + width*(i+1)/(float)rez); // v.x
            vertices.push_back(0.0f); // v.y
            vertices.push_back(-height/2.0f + height*(j+1)/(float)rez); // v.z
            vertices.push_back((i+1) / (float)rez); // u
            vertices.push_back((j+1) / (float)rez); // v
        }
    }
    std::cout << "Loaded " << rez*rez << " patches of 4 control points each" << std::endl;
    std::cout << "Processing " << rez*rez*4 << " vertices in vertex shader" << std::endl;

    float vertices_skybox[] = {
        // positions          // texcoords
        -0.5f, -0.5f, -0.5f,  0.0f,  0.0f,
         0.5f, -0.5f, -0.5f,  1.0f,  0.0f,
         0.5f,  0.5f, -0.5f,  1.0f,  1.0f,
         0.5f,  0.5f, -0.5f,  1.0f,  1.0f,
        -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,
        -0.5f, -0.5f, -0.5f,  0.0f,  0.0f,

        -0.5f, -0.5f,  0.5f,  0.0f,  0.0f,
         0.5f, -0.5f,  0.5f,  1.0f,  0.0f,
         0.5f,  0.5f,  0.5f,  1.0f,  1.0f,
         0.5f,  0.5f,  0.5f,  1.0f,  1.0f,
        -0.5f,  0.5f,  0.5f,  0.0f,  1.0f,
        -0.5f, -0.5f,  0.5f,  0.0f,  0.0f,

        -0.5f,  0.5f,  0.5f,  1.0f,  0.0f,
        -0.5f,  0.5f, -0.5f,  1.0f,  1.0f,
        -0.5f, -0.5f, -0.5f,  0.0f,  1.0f,
        -0.5f, -0.5f, -0.5f,  0.0f,  1.0f,
        -0.5f, -0.5f,  0.5f,  0.0f,  0.0f,
        -0.5f,  0.5f,  0.5f,  1.0f,  0.0f,

         0.5f,  0.5f,  0.5f,  1.0f,  0.0f,
         0.5f,  0.5f, -0.5f,  1.0f,  1.0f,
         0.5f, -0.5f, -0.5f,  0.0f,  1.0f,
         0.5f, -0.5f, -0.5f,  0.0f,  1.0f,
         0.5f, -0.5f,  0.5f,  0.0f,  0.0f,
         0.5f,  0.5f,  0.5f,  1.0f,  0.0f,

        -0.5f, -0.5f, -0.5f,  0.0f,  1.0f,
         0.5f, -0.5f, -0.5f,  1.0f,  1.0f,
         0.5f, -0.5f,  0.5f,  1.0f,  0.0f,
         0.5f, -0.5f,  0.5f,  1.0f,  0.0f,
        -0.5f, -0.5f,  0.5f,  0.0f,  0.0f,
        -0.5f, -0.5f, -0.5f,  0.0f,  1.0f,

        -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,
         0.5f,  0.5f, -0.5f,  1.0f,  1.0f,
         0.5f,  0.5f,  0.5f,  1.0f,  0.0f,
         0.5f,  0.5f,  0.5f,  1.0f,  0.0f,
        -0.5f,  0.5f,  0.5f,  0.0f,  0.0f,
        -0.5f,  0.5f, -0.5f,  0.0f,  1.0f
    };

    for(auto vertex : vertices_skybox) vertices.push_back(vertex);

    unsigned int VAO, VBO;
    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);

    glGenBuffers(1, &VBO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * vertices.size(), &vertices[0], GL_STATIC_DRAW);

    // position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    // texCoord attribute
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(sizeof(float) * 3));
    glEnableVertexAttribArray(1);

    glPatchParameteri(GL_PATCH_VERTICES, NUM_PATCH_PTS);

    tessHeightMapShader.use();
    //normal map
    unsigned int normalMap = loadTexture("./src/terrainmaps/normalmap.png");
    tessHeightMapShader.setInt("normalMap", 1);

    //specular map
    unsigned int specularMap = loadTexture("./src/terrainmaps/specularmap.png");
    tessHeightMapShader.setInt("specularMap", 8);

    //texture blend map
    unsigned int textureBlendMap = loadTexture("./src/terrainmaps/textureblendmap.png");
    tessHeightMapShader.setInt("textureBlendMap", 2);

    //terrain texturing
    unsigned int texture3 = loadTexture("./src/textures/texture3.jpg");
    tessHeightMapShader.setInt("texture3",3);
    unsigned int texture4 = loadTexture("./src/textures/texture4.jpg");
    tessHeightMapShader.setInt("texture4",4);
    unsigned int texture5 = loadTexture("./src/textures/texture5.jpg");
    tessHeightMapShader.setInt("texture5",5);
    unsigned int texture6 = loadTexture("./src/textures/texture6.jpg");
    tessHeightMapShader.setInt("texture6",6);

    // lighting
    glm::vec3 lightPos(625.2f, 205.0f, 1600.0f);
    glm::vec3 lightColor(0.9f, 0.9f, 1.0f);
    float ambientStrength = 0.85f;
    float shininess = 0.7f;
    float diffuseStrength = 0.3f;
    float specularStrength = 0.4f;
    
    tessHeightMapShader.setVec3("viewPos", camera.Position);

    cloudShader.use();
    cloudShader.setVec3("rayOrigin", camera.Position);
    glm::vec4 cloudBaseColor(0.7f, 0.7f, 0.7f, 0.0f);
    glm::vec3 rayColor1(1.0f, 0.95f, 0.5f);
    glm::vec3 rayColor2(0.5f, 0.8f, 0.55f);
    float densityMultiplier = 0.9f;
    float sizeAmountRatio = 345.528f;
    glm::vec3 perlinSeed1(53.157f, 9.67f, 1.107f);
    glm::vec3 perlinSeed2(17.127f, 6.173f, 17.79f);
    glm::vec3 perlinSeed3(27.53f, 1.97f, 7.139f);
    float cloudYtranslation = -2000.0f;
    

    // MODELS
    /*
    modelShader.use();
    modelShader.setVec3("lightColor", 0.9f, 0.9f, 1.0f);
    modelShader.setVec3("lightPos", lightPos);
    modelShader.setVec3("viewPos", camera.Position);
    
    stbi_set_flip_vertically_on_load(true);
    Model tree1("./src/resources/objects/tree1/tree1.obj");
    unsigned int modelDiffuseMap = loadTexture("./src/resources/objects/tree1/diffuse.jpeg");
    modelShader.setInt("modelDiffuse", 7);
    unsigned int modelSpecularMap = loadTexture("./src/resources/objects/tree1/specular.png");
    modelShader.setInt("modelSpecular", 8);
    */

    //SKYBOX
    std::vector<std::string> faces
    {
    "./src/skybox/skyrender0001.bmp", //right
    "./src/skybox/skyrender0004.bmp", //left
    "./src/skybox/skyrender0003.bmp", //top
    "./src/skybox/skyrender0004.bmp", //bottom
    "./src/skybox/skyrender0005.bmp", //front
    "./src/skybox/skyrender0002.bmp" //back (nevidlivo)
    };
    
    unsigned int cubemapTexture = loadCubemap(faces);
    skyboxShader.use();
    skyboxShader.setInt("skybox", 0);
    float skyboxIntensity = 1.0f;

    // render loop
    // -----------
    while (!glfwWindowShouldClose(window))
    {
        // per-frame time logic
        // --------------------
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        // input
        // -----
        processInput(window);

        // render
        // ------
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // feed inputs to dear imgui, start new frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        //SKYBOX
        glDepthFunc(GL_LEQUAL);
        skyboxShader.use();

        //uniforms for GUI control
        skyboxShader.setFloat("skyboxIntensity", skyboxIntensity);

        glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100000.0f);
        glm::mat4 view = glm::mat4(glm::mat3(camera.GetViewMatrix())); // remove translation from the view matrix
        skyboxShader.setMat4("projection", projection);
        skyboxShader.setMat4("view", view);

        glBindVertexArray(VAO);
        glActiveTexture(GL_TEXTURE9);
        glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTexture);
        glDrawArrays(GL_TRIANGLES, NUM_PATCH_PTS*rez*rez, 36);
        glDepthFunc(GL_LESS); // set depth function back to default

        // be sure to activate shader when setting uniforms/drawing objects
        tessHeightMapShader.use();
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, heightMap);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, normalMap);
        glActiveTexture(GL_TEXTURE8);
        glBindTexture(GL_TEXTURE_2D, specularMap);
        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, textureBlendMap);
        glActiveTexture(GL_TEXTURE3);
        glBindTexture(GL_TEXTURE_2D, texture3);
        glActiveTexture(GL_TEXTURE4);
        glBindTexture(GL_TEXTURE_2D, texture4);
        glActiveTexture(GL_TEXTURE5);
        glBindTexture(GL_TEXTURE_2D, texture5);
        glActiveTexture(GL_TEXTURE6);
        glBindTexture(GL_TEXTURE_2D, texture6);

        //uniforms for GUI control
        tessHeightMapShader.setVec3("lightColor", lightColor);
        tessHeightMapShader.setVec3("lightPos", lightPos);
        tessHeightMapShader.setFloat("ambientStrength", ambientStrength);
        tessHeightMapShader.setFloat("diffuseStrength", diffuseStrength);
        tessHeightMapShader.setFloat("specularStrength", specularStrength);
        tessHeightMapShader.setFloat("shininess", shininess);

        // view/projection transformations
        projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100000.0f);
        view = camera.GetViewMatrix();
        tessHeightMapShader.setMat4("projection", projection);
        tessHeightMapShader.setMat4("view", view);

        // world transformation
        glm::mat4 model = glm::mat4(1.0f);
        tessHeightMapShader.setMat4("model", model);

        // render terrain
        glBindVertexArray(VAO);
        //glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        glDrawArrays(GL_PATCHES, 0, NUM_PATCH_PTS*rez*rez);

        //render clouds
        cloudShader.use();

        //uniforms for GUI control
        cloudShader.setVec4("cloudBaseColor", cloudBaseColor);
        cloudShader.setVec3("rayColor1", rayColor1);
        cloudShader.setVec3("rayColor2", rayColor2);
        cloudShader.setFloat("densityMultiplier", densityMultiplier);
        cloudShader.setFloat("sizeAmountRatio", sizeAmountRatio);
        cloudShader.setVec3("perlinSeed1", perlinSeed1);
        cloudShader.setVec3("perlinSeed2", perlinSeed2);
        cloudShader.setVec3("perlinSeed3", perlinSeed3);

        cloudShader.setMat4("projection", projection);
        cloudShader.setMat4("view", view);
        model = glm::translate(model, glm::vec3(0.0f, cloudYtranslation, 0.0f));
        model = glm::scale(model, glm::vec3(5000.0f, 5000.0f, 5000.0f));
        cloudShader.setMat4("model", model);

        glDrawArrays(GL_TRIANGLES, NUM_PATCH_PTS*rez*rez+30, 6);

        for(int i=0;i<16;++i) {
            model = glm::translate(model, glm::vec3(0.0f, -0.0005f, 0.0f));
            cloudShader.setMat4("model", model);
            glDrawArrays(GL_TRIANGLES, NUM_PATCH_PTS*rez*rez+30, 6);
        }

        model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(0.0f, cloudYtranslation, 0.0f));
        model = glm::scale(model, glm::vec3(5000.0f, 5000.0f, 5000.0f));
        cloudShader.setMat4("model", model);
        for(int i=0;i<16;++i) {
            model = glm::translate(model, glm::vec3(0.0f, 0.0005f, 0.0f));
            cloudShader.setMat4("model", model);
            glDrawArrays(GL_TRIANGLES, NUM_PATCH_PTS*rez*rez+30, 6);
        }

        // MODELS
        /*
        modelShader.use();
        glActiveTexture(GL_TEXTURE7);
        glBindTexture(GL_TEXTURE_2D, modelDiffuseMap);
        glActiveTexture(GL_TEXTURE8);
        glBindTexture(GL_TEXTURE_2D, modelSpecularMap);
        modelShader.setMat4("projection", projection);
        modelShader.setMat4("view", view);
        model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(-220.0f, -12.5f, 150.0f));
        model = glm::scale(model, glm::vec3(0.5f, 0.5f, 0.5f));	
        modelShader.setMat4("model", model);
        tree1.Draw(modelShader);
        model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(-260.0f, -12.5f, 155.0f)); 
        model = glm::scale(model, glm::vec3(0.5f, 0.5f, 0.5f));	
        model = glm::rotate(model, (float)glm::radians(45.0), glm::vec3(0.0,1.0,0.0));
        modelShader.setMat4("model", model);
        tree1.Draw(modelShader);
        model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(-260.0f, -12.5f, 105.0f)); 
        model = glm::scale(model, glm::vec3(0.5f, 0.5f, 0.5f));	
        model = glm::rotate(model, (float)glm::radians(45.0), glm::vec3(0.0,1.0,0.0));
        modelShader.setMat4("model", model);
        tree1.Draw(modelShader);
        */

       // render GUI
        ImGui::SetNextWindowSize(ImVec2((float)400.0f, (float)175.0f));
        ImGui::Begin("Terrain lighting");
        ImGui::ColorEdit3("lightColor", (float*)&lightColor);
        ImGui::SliderFloat3("lightPos", (float*)&lightPos, -2000.0f, 2000.0f);
        ImGui::SliderFloat("ambientStrength", (float*)&ambientStrength, 0.0f, 1.0f);
        ImGui::SliderFloat("diffuseStrength", (float*)&diffuseStrength, 0.0f, 1.0f);
        ImGui::SliderFloat("specularStrength", (float*)&specularStrength, 0.0f, 1.0f);
        ImGui::SliderFloat("shininess?", (float*)&shininess, 0.0f, 1.0f);
        ImGui::End();

        ImGui::SetNextWindowSize(ImVec2((float)400.0f, (float)55.0f));
        ImGui::Begin("Skybox");
        ImGui::SliderFloat("skyboxIntensity", (float*)&skyboxIntensity, 0.0f, 1.5f);
        ImGui::End();

        ImGui::SetNextWindowSize(ImVec2((float)400.0f, (float)240.0f));
        ImGui::Begin("Clouds");
        ImGui::ColorEdit4("cloudBaseColor", (float*)&cloudBaseColor);
        ImGui::ColorEdit3("rayColor1", (float*)&rayColor1);
        ImGui::ColorEdit3("rayColor2", (float*)&rayColor2);
        ImGui::SliderFloat("densityMultiplier", (float*)&densityMultiplier, 0.0f, 5.0f);
        ImGui::SliderFloat("size/amount", (float*)&sizeAmountRatio, 0.0f, 1000.0f);
        ImGui::SliderFloat3("perlinSeed1", (float*)&perlinSeed1, 0.0f, 100.0f);
        ImGui::SliderFloat3("perlinSeed2", (float*)&perlinSeed2, 0.0f, 100.0f);
        ImGui::SliderFloat3("perlinSeed3", (float*)&perlinSeed3, 0.0f, 100.0f);
        ImGui::SliderFloat("y-translation", (float*)&cloudYtranslation, -2500.0f, 1000.0f);
        ImGui::End();

        // Render dear imgui into screen
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        // glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
        // -------------------------------------------------------------------------------
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    

    // optional: de-allocate all resources once they've outlived their purpose:
    // ------------------------------------------------------------------------
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);

    // glfw: terminate, clearing all previously allocated GLFW resources.
    // ------------------------------------------------------------------
    glfwTerminate();
    return 0;
}

// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
// ---------------------------------------------------------------------------------------------------------
void processInput(GLFWwindow *window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        camera.ProcessKeyboard(FORWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        camera.ProcessKeyboard(BACKWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        camera.ProcessKeyboard(LEFT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        camera.ProcessKeyboard(RIGHT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS)
        camera.ProcessKeyboard(LOOKRIGHT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS)
        camera.ProcessKeyboard(LOOKLEFT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS)
        camera.ProcessKeyboard(LOOKUP, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS)
        camera.ProcessKeyboard(LOOKDOWN, deltaTime);
}

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
// ---------------------------------------------------------------------------------------------
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    // make sure the viewport matches the new window dimensions; note that width and
    // height will be significantly larger than specified on retina displays.
    glViewport(0, 0, width, height);
}

// glfw: whenever a key event occurs, this callback is called
// ---------------------------------------------------------------------------------------------
void key_callback(GLFWwindow* window, int key, int scancode, int action, int modifiers)
{
    if(action == GLFW_PRESS)
    {
        switch(key)
        {
            default:
                break;
        }
    }
}

unsigned int loadTexture(char const * path)
{
    unsigned int textureID;
    glGenTextures(1, &textureID);

    int width, height, nrComponents;
    unsigned char *data = stbi_load(path, &width, &height, &nrComponents, 0);
    if (data)
    {
        GLenum format;
        if (nrComponents == 1)
            format = GL_RED;
        else if (nrComponents == 3)
            format = GL_RGB;
        else if (nrComponents == 4)
            format = GL_RGBA;

        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        stbi_image_free(data);
    }
    else
    {
        std::cout << "Texture failed to load at path: " << path << std::endl;
        stbi_image_free(data);
    }
    return textureID;
}

unsigned int loadCubemap(std::vector<std::string> faces)
{
    unsigned int textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);

    int width, height, nrChannels;
    for (unsigned int i = 0; i < faces.size(); i++)
    {
        unsigned char *data = stbi_load(faces[i].c_str(), &width, &height, &nrChannels, 0);
        if (data)
        {
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 
                         0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data
            );
            stbi_image_free(data);
        }
        else
        {
            std::cout << "Cubemap tex failed to load at path: " << faces[i] << std::endl;
            stbi_image_free(data);
        }
    }
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    return textureID;
}  

