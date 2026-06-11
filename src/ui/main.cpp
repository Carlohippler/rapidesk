// src/ui/main.cpp
#include <iostream>
#include <GLFW/glfw3.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include "main_window.hpp"

int main() {
    // 1. Inicializar a biblioteca de janelas (GLFW)
    if (!glfwInit()) return -1;

    // Configurações de contexto do OpenGL
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // 2. Criar a janela nativa do Windows onde o RapidDesk vai rodar
    GLFWwindow* window = glfwCreateWindow(1280, 720, "RapidDesk Client", nullptr, nullptr);
    if (!window) {
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1); // Ativa V-Sync para evitar rasgos na tela

    // 3. Inicializar o Contexto do Dear ImGui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    // Instanciar a nossa classe de interface que criamos no passo anterior
    MainWindow rapid_desk_ui;

    // Inicializar os mapeamentos de Input e Render do ImGui para GLFW/OpenGL
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330");

    // 4. LAÇO PRINCIPAL DA INTERFACE (Roda a cada frame da GPU)
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents(); // Captura cliques de mouse e teclado do Windows

        // Inicia o frame do ImGui
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // -> CHAMA A NOSSA INTERFACE INTUITIVA QUE DESENHAMOS
        rapid_desk_ui.render_ui();

        // Renderização final do frame
        ImGui::Render();
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClearColor(0.05f, 0.05f, 0.05f, 1.0f); // Fundo escuro atrás da janela
        glClear(GL_COLOR_BUFFER_BIT);

        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        glfwSwapBuffers(window); // Desenha tudo na tela
    }

    // 5. Limpeza de memória ao fechar
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}