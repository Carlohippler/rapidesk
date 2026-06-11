// src/ui/main_window.cpp
#include "main_window.hpp"
#include <imgui.h>

MainWindow::MainWindow() {
    apply_custom_dark_theme();
}

void MainWindow::render_ui() {
    if (!is_connected_) {
        draw_connection_screen();
    }
    else {
        draw_remote_viewer_toolbar();
    }
}

// TELA INICIAL INSPIRADA NO ANYDESK / TEAMVIEWER
void MainWindow::draw_connection_screen() {
    // Pega o tamanho total atual da janela do Windows dinamicamente
    ImVec2 screen_size = ImGui::GetIO().DisplaySize;

    ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_Always);
    ImGui::SetNextWindowSize(screen_size, ImGuiCond_Always);

    ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_MenuBar;

    ImGui::Begin("RapidDesk Main Console", nullptr, window_flags);

    // BARRA DE MENU SUPERIOR
    if (ImGui::BeginMenuBar()) {
        if (ImGui::BeginMenu("Conexao")) {
            ImGui::MenuItem("Configurar Senha Nao Presencial...", NULL);
            ImGui::Separator();
            if (ImGui::MenuItem("Sair")) { /* Fechar App */ }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Extras")) {
            ImGui::MenuItem("Gravacao de Sessao Autogestao", NULL, false, false);
            ImGui::MenuItem("Transferencia de Arquivos", NULL, false, false);
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Ajuda")) {
            ImGui::MenuItem("Sobre o RapidDesk", NULL);
            ImGui::EndMenu();
        }
        ImGui::EndMenuBar();
    }

    // --- LAYOUT EM COLUNAS DINÂMICAS ---
    ImGui::Columns(2, "MainLayout", true);

    // Define a coluna da esquerda para ocupar sempre 25% da tela (mínimo de 180px e máximo de 260px)
    float sidebar_width = screen_size.x * 0.25f;
    if (sidebar_width < 180.0f) sidebar_width = 180.0f;
    if (sidebar_width > 260.0f) sidebar_width = 260.0f;
    ImGui::SetColumnWidth(0, sidebar_width);

    // ================= COLUNA ESQUERDA =================
    ImGui::Spacing();
    ImGui::TextColored(ImVec4(0.2f, 0.7f, 1.0f, 1.0f), "  RAPIDDESK v2.6");
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    // Selecionáveis ocupam toda a largura da coluna de forma automática
    ImGui::Selectable("   Controle Remoto", true);
    ImGui::Selectable("   Transferencia de Arquivos");
    ImGui::Selectable("   Lista de Computadores");
    ImGui::Selectable("   Historico de Sessoes");

    // Status fixado dinamicamente no rodapé da janela atual
    ImGui::SetCursorPosY(screen_size.y - 45.0f);
    ImGui::Separator();
    ImGui::Text("  Status:"); ImGui::SameLine();
    ImGui::TextColored(ImVec4(0.2f, 0.8f, 0.2f, 1.0f), "Pronto");

    ImGui::NextColumn();

    // ================= COLUNA DIREITA (CONTEÚDO PRINCIPAL) =================
    ImGui::Spacing();

    // Pega o espaço restante disponível na coluna da direita
    float content_width = ImGui::GetContentRegionAvail().x;

    // Bloco 1: Este Computador
    ImGui::BeginChild("EsteComputadorChild", ImVec2(content_width, 110), true);
    ImGui::TextColored(ImVec4(0.2f, 0.7f, 1.0f, 1.0f), "Este Dispositivo");
    ImGui::TextDisabled("Compartilhe este ID para que outros controlem esta maquina.");
    ImGui::Spacing();

    ImGui::Text("Seu endereco de acesso:"); ImGui::SameLine();
    ImGui::SetNextItemWidth(content_width * 0.4f); // Ocupa 40% do espaço disponível
    ImGui::InputText("##my_id", my_desk_id_.data(), my_desk_id_.size(), ImGuiInputTextFlags_ReadOnly);

    ImGui::SameLine();
    if (ImGui::Button("Copiar ID")) {
        ImGui::SetClipboardText(my_desk_id_.data());
    }
    ImGui::EndChild();

    ImGui::Spacing();

    // Bloco 2: Conectar a Parceiro
    ImGui::BeginChild("ConectarParceiroChild", ImVec2(content_width, 160), true);
    ImGui::TextColored(ImVec4(1.0f, 0.6f, 0.0f, 1.0f), "Controlar Dispositivo Remoto");
    ImGui::TextDisabled("Digite o ID do parceiro para iniciar o streaming de alta performance.");
    ImGui::Spacing();

    ImGui::Text("ID do Computador:"); ImGui::SameLine();
    ImGui::SetNextItemWidth(content_width * 0.5f); // Ocupa 50% do espaço disponível
    ImGui::InputTextWithHint("##target_id", "Ex: 000 000 000", target_desk_id_.data(), target_desk_id_.size());

    ImGui::Spacing();

    static int session_mode = 0;
    ImGui::RadioButton("Controle Total", &session_mode, 0); ImGui::SameLine();
    ImGui::RadioButton("Apenas Transferir Arquivos", &session_mode, 1);

    ImGui::Spacing();

    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.1f, 0.5f, 0.2f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.15f, 0.6f, 0.25f, 1.0f));
    // O botão de conectar agora cresce proporcionalmente à coluna
    if (ImGui::Button("CONECTAR AO PARCEIRO", ImVec2(content_width * 0.5f, 35))) {
        if (std::strlen(target_desk_id_.data()) > 0) {
            is_connected_ = true;
        }
    }
    ImGui::PopStyleColor(2);
    ImGui::EndChild();

    ImGui::Spacing();

    // Bloco 3: Dispositivos Recentes Flexíveis
    ImGui::TextDisabled("Dispositivos Recentes");
    ImGui::Separator();
    ImGui::Spacing();

    // Calcula quantos cartões cabem na largura atual dinamicamente
    float card_width = 180.0f;
    for (int i = 0; i < 3; i++) {
        // Se o próximo cartão for estourar a tela para a direita, ele joga para a linha de baixo
        if (i > 0 && (ImGui::GetCursorPosX() + card_width > screen_size.x)) {
            ImGui::NewLine();
        }

        char label[64];
        sprintf(label, "Desktop - Suporte %d\nID: 987 654 32%d", i + 1, i);
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.15f, 0.16f, 0.19f, 1.0f));
        if (ImGui::Button(label, ImVec2(card_width, 60))) {
            strcpy(target_desk_id_.data(), "987 654 320");
        }
        ImGui::PopStyleColor();
        ImGui::SameLine();
    }

    ImGui::Columns(1);
    ImGui::End();
}

// BARRA DE SESSÃO ATIVA (Viewer flutuante quando conectado)
void MainWindow::draw_remote_viewer_toolbar() {
    // Centraliza a barra flutuante no topo de forma exata, independentemente do monitor (1080p, 4K, Smart)
    ImGui::SetNextWindowPos(ImVec2(ImGui::GetIO().DisplaySize.x * 0.5f, 5.0f), ImGuiCond_Always, ImVec2(0.5f, 0.0f));
    ImGui::SetNextWindowBgAlpha(0.90f);

    ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove | ImGuiWindowFlags_AlwaysAutoResize;

    ImGui::Begin("##ToolbarViewer", nullptr, flags);

    ImGui::Text("Sessao Ativa: %s", target_desk_id_.data());
    ImGui::SameLine();
    ImGui::TextDisabled("| Latenia: 4ms (H.265/NVENC) |");
    ImGui::SameLine();

    if (ImGui::Button("Arquivos")) { /* ... */ }
    ImGui::SameLine();
    if (ImGui::Button("Acoes (Ctrl+Alt+Del)")) { /* ... */ }
    ImGui::SameLine();
    if (ImGui::Button("Ajustes")) { show_settings_dropdown_ = !show_settings_dropdown_; }

    ImGui::SameLine();
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.7f, 0.15f, 0.15f, 1.0f));
    if (ImGui::Button("Fechar Conexao")) {
        is_connected_ = false;
        show_settings_dropdown_ = false;
    }
    ImGui::PopStyleColor();

    if (show_settings_dropdown_) {
        // Abre o menu de ajustes alinhado perfeitamente abaixo do botão de forma relativa
        ImGui::SetNextWindowPos(ImVec2(ImGui::GetWindowPos().x, ImGui::GetWindowPos().y + ImGui::GetWindowSize().y + 5));
        ImGui::Begin("Ajustes Rapidos##Dropdown", &show_settings_dropdown_, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_AlwaysAutoResize);

        ImGui::TextDisabled("Qualidade de Imagem");
        ImGui::RadioButton("Priorizar Velocidade", &config_.video_codec_selection, 0);
        ImGui::RadioButton("Priorizar Qualidade (4:4:4)", &config_.video_codec_selection, 1);

        ImGui::Separator();
        ImGui::Checkbox("Bloquear Teclado/Mouse do Cliente", &config_.allow_keyboard_mouse);
        ImGui::Checkbox("Sincronizar Clipboards", &config_.enable_clipboard_sync);

        ImGui::End();
    }

    ImGui::End();
}

// ESTILO ESCURO PREMIUM (Substitui o visual cinza antigo por tons modernos de azul petróleo e grafite)
void MainWindow::apply_custom_dark_theme() {
    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding = 5.0f;
    style.FrameRounding = 4.0f;
    style.PopupRounding = 4.0f;
    style.ChildRounding = 5.0f;
    style.FrameBorderSize = 1.0f;

    ImVec4* colors = style.Colors;
    colors[ImGuiCol_Text] = ImVec4(0.90f, 0.92f, 0.95f, 1.00f);
    colors[ImGuiCol_WindowBg] = ImVec4(0.07f, 0.08f, 0.10f, 1.00f); // Fundo Grafite AnyDesk
    colors[ImGuiCol_ChildBg] = ImVec4(0.11f, 0.12f, 0.15f, 1.00f); // Sub-blocos internos
    colors[ImGuiCol_PopupBg] = ImVec4(0.13f, 0.14f, 0.18f, 1.00f);
    colors[ImGuiCol_Border] = ImVec4(0.18f, 0.20f, 0.24f, 1.00f);
    colors[ImGuiCol_FrameBg] = ImVec4(0.14f, 0.16f, 0.19f, 1.00f);
    colors[ImGuiCol_FrameBgHovered] = ImVec4(0.20f, 0.23f, 0.28f, 1.00f);
    colors[ImGuiCol_FrameBgActive] = ImVec4(0.24f, 0.28f, 0.35f, 1.00f);
    colors[ImGuiCol_MenuBarBg] = ImVec4(0.11f, 0.12f, 0.15f, 1.00f);
    colors[ImGuiCol_Button] = ImVec4(0.16f, 0.19f, 0.24f, 1.00f);
    colors[ImGuiCol_ButtonHovered] = ImVec4(0.22f, 0.27f, 0.35f, 1.00f);
    colors[ImGuiCol_ButtonActive] = ImVec4(0.28f, 0.35f, 0.45f, 1.00f);
    colors[ImGuiCol_Header] = ImVec4(0.18f, 0.23f, 0.32f, 1.00f); // Azul para itens ativos
    colors[ImGuiCol_HeaderHovered] = ImVec4(0.25f, 0.31f, 0.43f, 1.00f);
    colors[ImGuiCol_HeaderActive] = ImVec4(0.30f, 0.38f, 0.53f, 1.00f);
}