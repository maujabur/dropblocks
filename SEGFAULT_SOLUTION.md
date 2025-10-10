# Solução para Segmentation Fault no Raspberry Pi

## Problema Identificado

O segmentation fault está ocorrendo durante a inicialização do SDL2, antes mesmo de qualquer coisa aparecer na tela. Isso indica problemas fundamentais na inicialização do sistema gráfico.

## Soluções Implementadas

### 1. **Debugging Detalhado da Inicialização**

Adicionei mensagens de debug em cada etapa da inicialização:

- **SDL2 Subsystems**: Inicialização passo a passo de cada subsistema
- **Display Detection**: Verificação de displays disponíveis
- **Window Creation**: Teste de criação de janela (modo janela primeiro, depois fullscreen)
- **Renderer Creation**: Teste de renderer (acelerado primeiro, software como fallback)
- **Game Initialization**: Debug detalhado do carregamento de peças e configurações

### 2. **Teste de Inicialização Básica**

Criei `test_init.cpp` que testa apenas a inicialização do SDL2:

```bash
./test_init
```

Este teste verifica:
- Inicialização do SDL2
- Detecção de displays
- Criação de janela
- Criação de renderer
- Renderização básica

### 3. **Inicialização Robusta do SDL2**

Modifiquei a inicialização para ser mais robusta:

- **Inicialização passo a passo**: Cada subsistema é inicializado separadamente
- **Fallbacks**: Se renderer acelerado falhar, tenta software
- **Modo janela primeiro**: Tenta criar janela normal antes de fullscreen
- **Tratamento de erros**: Mensagens detalhadas para cada falha

### 4. **Scripts de Build e Debug**

- `build_raspberry_pi.sh`: Compila todas as versões
- `debug_raspberry_pi.sh`: Executa debugging com valgrind e gdb
- `raspberry_pi.cfg`: Configuração otimizada para Raspberry Pi

## Como Usar

### 1. **Compilar**
```bash
chmod +x build_raspberry_pi.sh
./build_raspberry_pi.sh
```

### 2. **Testar Inicialização**
```bash
./test_init
```

Se este teste falhar, o problema é na inicialização básica do SDL2.

### 3. **Debugging Detalhado**
```bash
chmod +x debug_raspberry_pi.sh
./debug_raspberry_pi.sh
```

### 4. **Executar o Jogo**
```bash
# Versão com debug
./dropblocks_debug

# Versão otimizada
./dropblocks
```

## Possíveis Causas do Segmentation Fault

### 1. **Problemas de SDL2**
- SDL2 não instalado corretamente
- Incompatibilidade de versão
- Problemas com drivers de vídeo

### 2. **Problemas de Display**
- Nenhum display disponível
- Modo de display inválido
- Problemas com X11/Wayland

### 3. **Problemas de Renderer**
- Renderer acelerado não suportado
- Problemas com OpenGL/OpenGL ES
- Falta de suporte a hardware

### 4. **Problemas de Arquitetura ARM**
- Incompatibilidade com instruções x86
- Problemas com floating point
- Alinhamento de memória

## Diagnóstico Passo a Passo

### 1. **Verificar Dependências**
```bash
# Verificar SDL2
pkg-config --exists sdl2 && echo "SDL2 OK" || echo "SDL2 NOT FOUND"
pkg-config --modversion sdl2

# Verificar dependências
ldd test_init
```

### 2. **Executar Teste de Inicialização**
```bash
./test_init
```

**Se falhar aqui**: Problema na inicialização básica do SDL2

### 3. **Executar com Debugging**
```bash
gdb ./test_init
(gdb) run
(gdb) bt
```

### 4. **Verificar Logs do Sistema**
```bash
# Verificar logs do sistema
dmesg | tail -20

# Verificar logs do X11
cat /var/log/Xorg.0.log | tail -20
```

## Soluções Específicas

### Se o problema for SDL2:
```bash
sudo apt-get update
sudo apt-get install -y libsdl2-dev libsdl2-2.0-0
```

### Se o problema for display:
```bash
# Verificar se está rodando em modo gráfico
echo $DISPLAY

# Verificar displays disponíveis
xrandr
```

### Se o problema for renderer:
```bash
# Verificar suporte a OpenGL
glxinfo | grep "OpenGL"

# Verificar suporte a hardware
lspci | grep VGA
```

## Arquivos Modificados

- `dropblocks.cpp`: Adicionado debugging detalhado na inicialização
- `test_init.cpp`: Teste de inicialização básica
- `build_raspberry_pi.sh`: Script de compilação atualizado
- `debug_raspberry_pi.sh`: Script de debugging
- `raspberry_pi.cfg`: Configuração otimizada

## Próximos Passos

1. **Execute o teste de inicialização**: `./test_init`
2. **Se falhar**: Use GDB para identificar onde exatamente está falhando
3. **Se passar**: Execute o jogo com debug: `./dropblocks_debug`
4. **Reporte os resultados**: Me envie a saída completa dos testes

## Contato

Se o problema persistir, envie:
1. A saída completa do `./test_init`
2. A saída do `ldd test_init`
3. A saída do `pkg-config --modversion sdl2`
4. A saída do `uname -a`
5. A saída do `echo $DISPLAY`

