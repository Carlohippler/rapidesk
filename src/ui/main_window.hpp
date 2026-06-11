// src/ui/main_window.hpp
#pragma once
#include <string>
#include <array>

class MainWindow {
public:
    MainWindow();
    ~MainWindow() = default;

    // Inicializa o estilo visual escuro (Dark Theme) customizado
    void apply_custom_dark_theme();

    // Chamado a cada frame para renderizar a interface
    void render_ui();

private:
    // Estados da Interface
    bool is_connected_ = false;
    bool show_settings_dropdown_ = false;
    bool show_advanced_settings_ = false;

    // Buffers de texto para os inputs
    std::array<char, 32> my_desk_id_ = { "123 456 789" }; // ID gerado para esta máquina
    std::array<char, 32> target_desk_id_ = { "" };        // ID que o usuário quer conectar
    std::array<char, 64> security_password_ = { "" };     // Senha de acesso remoto

    // --- PRE-INICIALIZAÇÃO PARA CONFIGURAÇÕES FUTURAS (DISCRETAS) ---
    struct RemoteConfig {
        bool enable_clipboard_sync = true;
        bool allow_keyboard_mouse = true;
        int  video_codec_selection = 0;      // 0: NVENC, 1: FFmpeg/CPU, 2: Auto
        int  target_fps = 60;                // 30, 60, 120 FPS
        bool limit_bitrate = false;
        int  max_bitrate_mbps = 10;
        bool audio_passthrough = false;
    } config_;

    // Funções internas para desenhar cada tela
    void draw_connection_screen();
    void draw_remote_viewer_toolbar();
    void draw_settings_overlay();
};