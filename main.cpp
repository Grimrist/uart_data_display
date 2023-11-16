// Dear ImGui: standalone example application for GLFW + OpenGL 3, using programmable pipeline
// (GLFW is a cross-platform general purpose library for handling windows, inputs, OpenGL/Vulkan/Metal graphics context creation, etc.)

// Learn about Dear ImGui:
// - FAQ                  https://dearimgui.com/faq
// - Getting Started      https://dearimgui.com/getting-started
// - Documentation        https://dearimgui.com/docs (same as your local docs/ folder).
// - Introduction, links and more at the top of imgui.cpp

#include <vector>
#include <string>
#include <sstream>
#include "windows.h"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include <stdio.h>
#define GL_SILENCE_DEPRECATION
#if defined(IMGUI_IMPL_OPENGL_ES2)
#include <GLES2/gl2.h>
#endif
#include "glad/glad.h"
#include <GLFW/glfw3.h> // Will drag system OpenGL headers

// [Win32] Our example includes a copy of glfw3.lib pre-compiled with VS2010 to maximize ease of testing and compatibility with old VS compilers.
// To link with VS2010-era libraries, VS2015+ requires linking with legacy_stdio_definitions.lib, which we do using this pragma.
// Your own project should not be affected, as you are likely to link with a newer binary of GLFW that is adequate for your version of Visual Studio.
#if defined(_MSC_VER) && (_MSC_VER >= 1900) && !defined(IMGUI_DISABLE_WIN32_FUNCTIONS)
#pragma comment(lib, "legacy_stdio_definitions")
#endif

// This example can also compile and run with Emscripten! See 'Makefile.emscripten' for details.
#ifdef __EMSCRIPTEN__
#include "../libs/emscripten/emscripten_mainloop_stub.h"
#endif

#define DEFAULT_SIZE 20

static void glfw_error_callback(int error, const char* description)
{
    fprintf(stderr, "GLFW Error %d: %s\n", error, description);
}

int read_UART(HANDLE fileIO, char* buffer, int size)
{
    DWORD dwRead = 0;
    OVERLAPPED ReadState = {0};
    if (!ReadFile(fileIO, buffer, size, &dwRead, &ReadState)) {
        //fprintf(stderr, "Error: %d", GetLastError() );
        return 0;
    }
    //CloseHandle(ReadState.hEvent);
    return (int) dwRead;
}

HANDLE init_serial(char* com_port) {
    HANDLE h_Serial = CreateFile(com_port, 
                        GENERIC_READ | GENERIC_WRITE, 
                        0, 
                        0, 
                        OPEN_EXISTING,
                        FILE_ATTRIBUTE_NORMAL, 
                        0);
    if (h_Serial == INVALID_HANDLE_VALUE) {
        if (GetLastError() == ERROR_FILE_NOT_FOUND) {
            fprintf(stderr, "COM Port not found.\n");
            return NULL;
        }
        fprintf(stderr, "Handle initialization failed.");
        return NULL;
    }

    DCB dcbSerialParam = {0};
    dcbSerialParam.DCBlength = sizeof(dcbSerialParam);

    if (!GetCommState(h_Serial, &dcbSerialParam)) {
        fprintf(stderr, "Failed to get comm state.");
        return NULL;
    }

    dcbSerialParam.BaudRate = CBR_115200;
    dcbSerialParam.ByteSize = 8;
    dcbSerialParam.StopBits = ONESTOPBIT;
    dcbSerialParam.Parity = NOPARITY;

    if (!SetCommState(h_Serial, &dcbSerialParam)) {
        fprintf(stderr, "Failed to set comm state.");
        return NULL;
    }

    COMMTIMEOUTS timeout = {0};
    timeout.ReadIntervalTimeout = 1;
    timeout.ReadTotalTimeoutConstant = 1;
    timeout.ReadTotalTimeoutMultiplier = 1;
    timeout.WriteTotalTimeoutConstant = 1;
    timeout.WriteTotalTimeoutMultiplier = 1;
    if (!SetCommTimeouts(h_Serial, &timeout)) {
        fprintf(stderr, "Failed to set comm timeout.");
        return NULL;
    }

    return h_Serial;
}

// Main code
int main(int, char**)
{
    //Initialize Serial port variables
    std::string disp;
    DWORD dwRead = 0;
    int UART_Size = DEFAULT_SIZE;
    char sBuff[40 + 1] = {0};
    //*******************************

    glfwSetErrorCallback(glfw_error_callback);
    if (!glfwInit())
        return 1;

    // Decide GL+GLSL versions
#if defined(IMGUI_IMPL_OPENGL_ES2)
    // GL ES 2.0 + GLSL 100
    const char* glsl_version = "#version 100";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
#elif defined(__APPLE__)
    // GL 3.2 + GLSL 150
    const char* glsl_version = "#version 150";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);  // 3.2+ only
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);            // Required on Mac
#else
    // GL 3.0 + GLSL 130
    const char* glsl_version = "#version 130";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    //glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);  // 3.2+ only
    //glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);            // 3.0+ only
#endif

    // Create window with graphics context
    GLFWwindow* window = glfwCreateWindow(1280, 720, "Position Visualizer", nullptr, nullptr);
    if (window == nullptr)
        return 1;
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1); // Enable vsync
    gladLoadGL();
    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    //ImGui::StyleColorsLight();

    // Setup Platform/Renderer backends
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(glsl_version);

    // Load Fonts
    // - If no fonts are loaded, dear imgui will use the default font. You can also load multiple fonts and use ImGui::PushFont()/PopFont() to select them.
    // - AddFontFromFileTTF() will return the ImFont* so you can store it if you need to select the font among multiple.
    // - If the file cannot be loaded, the function will return a nullptr. Please handle those errors in your application (e.g. use an assertion, or display an error and quit).
    // - The fonts will be rasterized at a given size (w/ oversampling) and stored into a texture when calling ImFontAtlas::Build()/GetTexDataAsXXXX(), which ImGui_ImplXXXX_NewFrame below will call.
    // - Use '#define IMGUI_ENABLE_FREETYPE' in your imconfig file to use Freetype for higher quality font rendering.
    // - Read 'docs/FONTS.md' for more instructions and details.
    // - Remember that in C/C++ if you want to include a backslash \ in a string literal you need to write a double backslash \\ !
    // - Our Emscripten build process allows embedding fonts to be accessible at runtime from the "fonts/" folder. See Makefile.emscripten for details.
    //io.Fonts->AddFontDefault();
    //io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\segoeui.ttf", 18.0f);
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/DroidSans.ttf", 16.0f);
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/Roboto-Medium.ttf", 16.0f);
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/Cousine-Regular.ttf", 15.0f);
    //ImFont* font = io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\ArialUni.ttf", 18.0f, nullptr, io.Fonts->GetGlyphRangesJapanese());
    //IM_ASSERT(font != nullptr);

    // Our state
    bool flipX = false;
    bool flipY = false;
    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
    int virt_region[2] = {1000,1000};
    double vect_scale = 0.6;
    char com_port[6] = "COM1";
    HANDLE h_Serial = init_serial(com_port);
    if(h_Serial == NULL) {
        fprintf(stderr, "Failed to initialize COM Handle.");
    }
    // Main loop
#ifdef __EMSCRIPTEN__
    // For an Emscripten build we are disabling file-system access, so let's not attempt to do a fopen() of the imgui.ini file.
    // You may manually call LoadIniSettingsFromMemory() to load settings from your own storage.
    io.IniFilename = nullptr;
    EMSCRIPTEN_MAINLOOP_BEGIN
#else
    while (!glfwWindowShouldClose(window))
#endif
    {
        // Poll and handle events (inputs, window resize, etc.)
        // You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
        // - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application, or clear/overwrite your copy of the mouse data.
        // - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application, or clear/overwrite your copy of the keyboard data.
        // Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
        glfwPollEvents();
        // Prepare visualization logic
        read_UART(h_Serial, sBuff, UART_Size);
        disp.assign(sBuff, UART_Size);

        std::vector<std::string> posData(4);
        std::stringstream ss(disp);
        std::string word;
        for (int i = 0; i < 4; i++) {
            if (!ss.eof()) {
                std::getline(ss, word, ',');
                posData.at(i) = word;
            }
        }

        // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        {
            static float f = 0.0f;
            static int counter = 0;
            ImGui::Begin("Configuration");                   
            ImGui::InputText("COM Port", com_port, 6);
            if (ImGui::Button("Update Port", ImVec2(200,20))) {
                CloseHandle(h_Serial);
                h_Serial = init_serial(com_port);
                if(h_Serial == NULL) {
                    fprintf(stderr, "Failed to initialize COM Handle.");
                }
            }
            ImGui::InputInt("UART Msg Size", &UART_Size, 1, 2, 0);
            ImGui::Text("Virtual Region");
            ImGui::InputInt2("X & Y", virt_region, 0);
            ImGui::Text("Vector adjustment"); 
            ImGui::Checkbox("Flip X", &flipX);
            ImGui::Checkbox("Flip Y", &flipY);
            ImGui::InputDouble("Vector scaling", &vect_scale, 0.1F, 1.0F, "%.2f", 0);

            ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);
            ImGui::End();
        }
        {
            ImGui::Begin("Position View");  
            static int pos_x = 0;
            static int pos_y = 0;
            static float accel_x = 0;
            static float accel_y = 0;
            ImGui::Text("Processed: %s", disp.data());
            ImGui::Text("Raw data: %s", sBuff);
            ImGui::Text("Accel x: %s", posData[0].data());
            ImGui::Text("Accel y: %s", posData[1].data());
            ImDrawList* draw_list = ImGui::GetWindowDrawList();
            static ImVec4 col = ImVec4(1.0f, 1.0f, 0.4f, 1.0f); 
            const ImU32 col32 = ImColor(col); 
            static ImVec2 circle_pos = ImVec2(100,100);
            try {
                accel_x = -std::stof(posData[0]) * vect_scale;
                accel_y = std::stof(posData[1]) * vect_scale;
            }
            catch (...) {
                fprintf(stderr, "Failed to convert accel!\n");
            }
            try {
                pos_x = std::stoi(posData[2]);
                pos_y = std::stoi(posData[3]); 
            }
            catch (...) {
                fprintf(stderr, "Failed to convert pos!\n");
            }

            if (flipX) accel_x = -accel_x;
            if (flipY) accel_y = -accel_y;
            float center_x = ImGui::GetWindowPos().x + ImGui::GetWindowSize().x * 0.5f;
            float center_y = ImGui::GetWindowPos().y + ImGui::GetWindowSize().y * 0.5f;
            ImGui::Text("Pos x: %d", pos_x);
            ImGui::Text("Pos y: %d", pos_y);
            ImGui::Text("Arrow x: %9.9f", accel_x);
            ImGui::Text("Arrow y: %9.9f", accel_y);
            float scaler_x = ImGui::GetWindowSize().x / virt_region[0];
            float scaler_y = ImGui::GetWindowSize().y / virt_region[0];
            float finalpos_x = (float) pos_x * scaler_x + ImGui::GetWindowPos().x;
            float finalpos_y = (float) pos_y * scaler_y + ImGui::GetWindowPos().y;
            draw_list->AddRectFilled(ImVec2(finalpos_x, finalpos_y), 
                                    ImVec2(finalpos_x + 20, finalpos_y + 20), 
                                    col32, 
                                    0, 
                                    0);
            draw_list->AddLine(ImVec2(center_x, center_y), 
                                ImVec2(center_x + accel_y, 
                                center_y + accel_x), 
                                col32, 
                                1.0f);
            ImGui::End();
        }

        // Rendering
        ImGui::Render();
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClearColor(clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());


        glfwSwapBuffers(window);
    }
#ifdef __EMSCRIPTEN__
    EMSCRIPTEN_MAINLOOP_END;
#endif

    // Cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();
    CloseHandle(h_Serial);

    return 0;
}