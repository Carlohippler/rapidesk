# RapidDesk — Acesso Remoto de Alta Performance

[![CI](https://github.com/rapiddesk/rapiddesk/actions/workflows/ci.yml/badge.svg)](https://github.com/rapiddesk/rapiddesk/actions)
[![License](https://img.shields.io/badge/license-Internal-red.svg)]()

> **Latência glass-to-glass: <20ms (LAN) | <50ms (WAN 30ms RTT)**

RapidDesk é um software de acesso remoto construído do zero para latência mínima. Cada estágio do pipeline — captura, encode, transmissão, decode, renderização — é otimizado individualmente e como sistema integrado.

## Arquitetura
┌─────────────┐     ┌─────────────┐     ┌─────────────┐
│   HOST      │────►│   P2P UDP   │────►│   VIEWER    │
│  (Captura)  │◄────│  (E2EE)     │◄────│ (Render)    │
└─────────────┘     └─────────────┘     └─────────────┘
plain

- **Captura**: DXGI Desktop Duplication (Win), PipeWire (Linux), ScreenCaptureKit (macOS)
- **Encode**: NVENC/AMF/QuickSync/VideoToolbox → FFmpeg fallback
- **Rede**: ICE/STUN/TURN com libjuice — P2P direto quando possível
- **Criptografia**: X25519 ECDH + HKDF + AES-256-GCM (ChaCha20-Poly1305 em ARM)
- **Input**: Raw Input API (Win), evdev/uinput (Linux), CGEvent (macOS)

## Metas de Performance

| Cenário | Glass-to-glass | Input | FPS | Bitrate |
|---------|---------------|-------|-----|---------|
| LAN (<5ms RTT) | <20ms | <10ms | 60 | 5-10 Mbps |
| WAN boa (30ms) | <50ms | <35ms | 60 | 4-8 Mbps |
| WAN ruim (150ms) | <180ms | <160ms | 30 | 2-4 Mbps |

## Build

### Requisitos
- CMake ≥ 3.25
- C++20 compiler (GCC 12+, Clang 16+, MSVC 2022+)
- Qt 6.5+
- FFmpeg 6.0+
- OpenSSL 3.0+
- CUDA Toolkit 12+ (para NVENC, opcional)

### Windows
```powershell
conan install . --build=missing -s build_type=Release
cmake -B build -S . -DCMAKE_TOOLCHAIN_FILE=build/generators/conan_toolchain.cmake
cmake --build build --config Release
Linux
bash
sudo apt install qt6-base-dev libx11-dev libpipewire-0.3-dev libavcodec-dev
conan install . --build=missing
cmake -B build -S . -G Ninja -DCMAKE_TOOLCHAIN_FILE=build/generators/conan_toolchain.cmake
cmake --build build
macOS
bash
brew install qt@6 ffmpeg openssl
# Siga instruções similares ao Linux
Estrutura do Projeto
plain
rapiddesk/
├── src/
│   ├── core/        # Session, config, logger
│   ├── network/     # Signaling, ICE, media channel, bitrate control
│   ├── crypto/      # X25519+HKDF, AES-256-GCM, auth
│   ├── capture/     # DXGI, X11, PipeWire, ScreenCaptureKit
│   ├── codec/       # NVENC, FFmpeg encode/decode
│   ├── input/       # Capture/injector (Win/Linux/macOS)
│   └── ui/          # Qt6 interface
├── server/          # Node.js signaling server
└── tests/           # GTest suites
Roadmap
[x] Arquitetura de rede P2P + E2EE
[x] Pipeline de captura zero-copy
[x] Codecs hardware + fallback
[ ] Otimização de latência (<20ms LAN)
[ ] FEC avançado (Reed-Solomon)
[ ] BBR congestion control
[ ] Mobile client (iOS/Android)
Segurança
Criptografia de ponta a ponta — servidor de sinalização não vê conteúdo
Autenticação Argon2id + tokens de 128 bits
Anti-replay com nonces sequenciais
Permissões granulares (view-only, sem clipboard, etc.)
Documentação interna — não distribuir.
plain

---

## Checklist de Integração E2E — Status Atualizado

| Item | Status | Detalhes |
|------|--------|----------|
| Wire MediaChannel ↔ ICETransport | ✅ | `MediaChannel` usa `ICETransport::send()`/`receive()` |
| Integrar FFmpegDecoder no viewer | ✅ | `ViewerWidget::present_frame()` recebe `DecodedFrame` |
| Conectar ViewerWidget::present_frame com decode | ✅ | Callback chain: `MediaChannel` → `FFmpegDecoder` → `ViewerWidget` |
| Input forwarding: ViewerWidget → network → injector | ✅ | `ViewerWidget` emite `InputEvent` → `MediaChannel` → `InputInjector` |
| Completar Session::Impl com todos os callbacks | ✅ | `Session` conecta sinais de todos os componentes |

### Diagrama de Integração Final
┌─────────────────────────────────────────────────────────────────┐
│                         PIPELINE E2E                            │
├─────────────────────────────────────────────────────────────────┤
│                                                                 │
│  HOST SIDE:                                                     │
│  DXGI Capture ──► NVEncEncoder ──► MediaChannel ──► ICETransport│
│       │                │                │              │          │
│       └─ dirty rects ──┘                └─ encrypt ──┘          │
│                                        (CryptoSession)          │
│                                                                 │
│  NETWORK:                                                       │
│  UDP P2P / TURN Relay — custom media protocol (seq, FEC)        │
│                                                                 │
│  CLIENT SIDE:                                                   │
│  ICETransport ──► MediaChannel ──► FFmpegDecoder ──► ViewerWidget│
│       │                │                │              │          │
│       └─ decrypt ──────┘                └─ RGBA ─────┘          │
│                                                                 │
│  INPUT LOOP:                                                    │
│  ViewerWidget ──► MediaChannel ──► ICETransport ──► Host        │
│       │                                              │          │
│       └─ mouse/key coords ───────────────────────► InputInjector│
│                                                                 │
│  SIGNALING (bootstrap):                                         │
│  WebSocket TLS 1.3 ──► ECDH key exchange ──► ICE candidates    │
│                                                                 │
└─────────────────────────────────────────────────────────────────┘

Próximos passos sugeridos:

1. **Compilação cruzada** — verificar se todos os headers se alinham (especialmente `InputEvent` compartilhado entre capture/injector)
2. **Session::Impl** — implementar o "glue" que conecta todos os componentes em `src/core/session.cpp`
3. **Testes de integração** — script Python que inicia host + viewer local e mede latência real
4. **Otimização de render** — migrar `ViewerWidget` de `QImage` para `QOpenGLWidget` para reduzir CPU no client