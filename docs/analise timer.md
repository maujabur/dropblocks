# Análise: Sistema de Countdown Timer para Kiosk

## Visão Geral
Implementação de um sistema de countdown timer configurável para limitar o tempo de jogo em modo kiosk, com controle via teclado e configuração através de arquivos .cfg.

## Estrutura do Projeto Atual

### 1. Sistema de Configuração
- **ConfigTypes.hpp**: Define estruturas de configuração
- **ConfigManager.cpp/hpp**: Gerencia carregamento e aplicação de configurações
- **ConfigProcessors.cpp**: Processa chaves de configuração específicas
- **ConfigApplicator.cpp**: Aplica configurações aos sistemas

### 2. Sistema de Input
- **InputManager.cpp/hpp**: Gerencia entrada unificada (teclado + joystick)
- **KeyboardInput.cpp/hpp**: Implementa entrada via teclado com timing DAS/ARR
- **InputTimingManager.cpp/hpp**: Sistema unificado de timing para inputs
- **IInputManager.hpp**: Interface base para gerenciamento de input

### 3. Sistema de Renderização
- **RenderManager.cpp/hpp**: Gerencia camadas de renderização
- **RenderLayer.hpp**: Interface base para camadas de renderização
- **LayoutCache.hpp**: Cache de layout virtual/físico
- **VirtualLayout.cpp/hpp**: Sistema de coordenadas virtuais

### 4. Estado do Jogo
- **GameState.cpp/hpp**: Estado principal do jogo
- **GameLoop.cpp/hpp**: Loop principal do jogo
- **GameTypes.hpp**: Tipos e estruturas do jogo

## Análise Técnica

### Componentes Necessários

#### 1. Estrutura de Configuração do Timer
```cpp
struct TimerConfig {
    bool enabled = false;           // Timer desabilitado por padrão
    int durationSeconds = 60;      // Duração em segundos (1 min padrão)
    ElementLayout layout;           // Posição e aparência visual
    bool showWarningAt30s = true;   // Aviso aos 30 segundos
    bool showWarningAt10s = true;   // Aviso aos 10 segundos
    RGB normalColor{255,255,255};   // Cor normal
    RGB warningColor{255,255,0};    // Cor de aviso
    RGB criticalColor{255,0,0};     // Cor crítica
};
```

#### 2. Classe TimerSystem
```cpp
class TimerSystem {
private:
    TimerConfig config_;
    Uint32 startTime_;
    Uint32 remainingTime_;
    bool isActive_;
    bool isWarning_;
    bool isCritical_;
    
public:
    void start();
    void pause();
    void resume();
    void reset();
    void toggle();
    bool isExpired() const;
    int getRemainingSeconds() const;
    std::string getFormattedTime() const;
    void update();
};
```

#### 3. Camada de Renderização do Timer
```cpp
class TimerRenderLayer : public RenderLayer {
private:
    TimerSystem* timerSystem_;
    
public:
    void render(SDL_Renderer* renderer, const GameState& state, const LayoutCache& layout) override;
    std::string getName() const override { return "Timer"; }
    int getZOrder() const override { return 100; } // Alta prioridade
};
```

### Integração com Sistemas Existentes

#### 1. ConfigTypes.hpp
Adicionar `TimerConfig` à estrutura principal de configuração, seguindo o padrão das outras configurações (LayoutConfig, AudioConfig, etc.).

#### 2. ConfigProcessors.cpp
Implementar função `processTimerConfigs()` para processar chaves como:
- `TIMER_ENABLED`
- `TIMER_DURATION_SECONDS`
- `TIMER_X`, `TIMER_Y`, `TIMER_WIDTH`, `TIMER_HEIGHT`
- `TIMER_FILL`, `TIMER_OUTLINE`, `TIMER_TEXT_COLOR`
- Etc.

#### 3. KeyboardInput.hpp
Adicionar novo timer para toggle do countdown:
```cpp
InputTimingManager::DirectionTimer timerToggleTimer_;
bool shouldToggleTimer() { 
    return timingManager_.shouldTriggerOnce(isKeyActive(SDL_SCANCODE_T), timerToggleTimer_); 
}
```

#### 4. GameState.hpp/cpp
Integrar TimerSystem como dependência:
```cpp
private:
    std::unique_ptr<TimerSystem> timer_;

public:
    TimerSystem& getTimer();
    const TimerSystem& getTimer() const;
```

#### 5. GameLoop.cpp
Integrar verificação de timer no loop principal:
```cpp
// No loop principal, após handleInput
if (state.getTimer().isExpired()) {
    state.setGameOver(true);
    // Opcional: Som de fim de tempo
}

// Verificar toggle do timer
if (inputManager.shouldToggleTimer()) {
    state.getTimer().toggle();
}
```

### Configurações Propostas (.cfg)

```ini
# ===========================
#   COUNTDOWN TIMER (KIOSK)
# ===========================
TIMER_ENABLED=0                    # 0=desabilitado, 1=habilitado
TIMER_DURATION_SECONDS=300         # Duração em segundos (5 minutos)
TIMER_X=50                         # Posição X (coordenadas virtuais)
TIMER_Y=50                         # Posição Y
TIMER_WIDTH=200                    # Largura
TIMER_HEIGHT=80                    # Altura
TIMER_FILL=#000000                 # Cor de fundo
TIMER_OUTLINE=#FFFFFF              # Cor da borda
TIMER_TEXT_COLOR=#FFFFFF           # Cor do texto normal
TIMER_WARNING_COLOR=#FFFF00        # Cor de aviso (30s)
TIMER_CRITICAL_COLOR=#FF0000       # Cor crítica (10s)
TIMER_SHOW_WARNING_AT_30S=1        # Avisar aos 30s
TIMER_SHOW_WARNING_AT_10S=1        # Avisar aos 10s
TIMER_BORDER_RADIUS=5              # Raio da borda arredondada
TIMER_BORDER_THICKNESS=2           # Espessura da borda
```

### Controles de Teclado

Adicionar tecla **T** para toggle do timer:
- **T** - Liga/Desliga o countdown timer durante o jogo
- Timer pode ser ativado/desativado mesmo se iniciado desabilitado na configuração

### Comportamento do Timer

1. **Início**: Timer inicia automaticamente quando novo jogo começa (se habilitado)
2. **Pausa**: Timer pausa quando jogo está pausado
3. **Reset**: Timer reseta quando jogo é reiniciado
4. **Fim**: Quando timer chega a 0, força game over
5. **Toggle**: Tecla T permite ligar/desligar durante o jogo

### Renderização

1. **Display**: Formato MM:SS (ex: 05:00, 01:30, 00:10)
2. **Cores**: 
   - Normal: Branco/configurável
   - Aviso (≤30s): Amarelo
   - Crítico (≤10s): Vermelho piscante
3. **Posição**: Configurável via .cfg, sugestão canto superior direito
4. **Z-Order**: Alta prioridade para ficar sempre visível

### Arquivos a Modificar

#### Novos Arquivos
1. `include/timer/TimerSystem.hpp`
2. `src/timer/TimerSystem.cpp`
3. `include/render/TimerRenderLayer.hpp`
4. `src/render/TimerRenderLayer.cpp`

#### Arquivos Existentes a Modificar
1. `include/ConfigTypes.hpp` - Adicionar TimerConfig
2. `src/config/ConfigProcessors.cpp` - Adicionar processTimerConfigs()
3. `include/input/KeyboardInput.hpp` - Adicionar shouldToggleTimer()
4. `src/input/KeyboardInput.cpp` - Implementar toggle timer
5. `include/input/IInputManager.hpp` - Adicionar interface shouldToggleTimer()
6. `include/input/InputManager.hpp` - Implementar shouldToggleTimer()
7. `include/app/GameState.hpp` - Adicionar TimerSystem
8. `src/app/GameState.cpp` - Integrar TimerSystem
9. `src/app/GameLoop.cpp` - Adicionar lógica do timer
10. `include/di/DependencyContainer.hpp` - Registrar TimerSystem
11. `src/di/DependencyContainer.cpp` - Implementar registro
12. Arquivos .cfg - Adicionar configurações do timer

### Dependências

- **SDL2**: Para timing (SDL_GetTicks())
- **Sistema de Layout Virtual**: Para posicionamento
- **Sistema de Renderização**: Para desenhar o timer
- **Sistema de Input**: Para controle via teclado
- **Sistema de Configuração**: Para configurabilidade

### Considerações de Performance

- Timer usa SDL_GetTicks() que é eficiente
- Renderização apenas quando necessário
- Sem impacto no gameplay principal
- Formatação de string apenas quando valor muda

### Testes Necessários

1. Configuração via .cfg funciona corretamente
2. Timer inicia/para/reseta conforme esperado
3. Toggle via teclado funciona
4. Cores mudam nas transições corretas
5. Game over forçado quando timer expira
6. Timer pausa com o jogo
7. Posicionamento responsivo em diferentes resoluções

## Conclusão

A implementação do countdown timer seguirá os padrões arquiteturais existentes do projeto, integrando-se naturalmente com os sistemas de configuração, input, renderização e estado do jogo. A modularidade permitirá fácil manutenção e extensibilidade futura.