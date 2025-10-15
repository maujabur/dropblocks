# DropBlocks - Debugging no Raspberry Pi

## Problema: Segmentation Fault

Se você está enfrentando segmentation fault ao executar o DropBlocks no Raspberry Pi, siga este guia de debugging.

## Soluções Implementadas

### 1. Verificações de Segurança Adicionadas

O código foi modificado para incluir verificações de segurança em todas as funções críticas:

- **Verificação de índices de array**: Todas as funções agora verificam se os índices estão dentro dos limites válidos
- **Verificação de ponteiros nulos**: Verificações adicionais para evitar acesso a memória inválida
- **Logging de debug**: Mensagens de erro detalhadas para identificar onde o problema ocorre

### 2. Scripts de Build e Debug

#### Compilação
```bash
chmod +x build_raspberry_pi.sh
./build_raspberry_pi.sh
```

#### Debugging
```bash
chmod +x debug_raspberry_pi.sh
./debug_raspberry_pi.sh
```

### 3. Configuração Otimizada

Use o arquivo `raspberry_pi.cfg` que foi criado com configurações otimizadas para Raspberry Pi:

```bash
export DROPBLOCKS_CFG=raspberry_pi.cfg
./dropblocks_debug
```

## Possíveis Causas do Segmentation Fault

### 1. **Problemas de SDL2**
- SDL2 não instalado corretamente
- Incompatibilidade de versão
- Problemas com drivers de vídeo

### 2. **Problemas de Memória**
- Acesso a arrays fora dos limites
- Ponteiros nulos
- Corrupção de memória

### 3. **Problemas de Arquitetura ARM**
- Incompatibilidade com instruções x86
- Problemas com floating point
- Alinhamento de memória

## Passos de Debugging

### 1. Verificar Dependências
```bash
# Verificar se SDL2 está instalado
pkg-config --exists sdl2 && echo "SDL2 OK" || echo "SDL2 NOT FOUND"

# Verificar versão
pkg-config --modversion sdl2

# Verificar dependências do executável
ldd dropblocks_debug
```

### 2. Executar com Debugging
```bash
# Versão com verificações de segurança
./dropblocks_debug

# Com GDB
gdb ./dropblocks_debug
(gdb) run
(gdb) bt
```

### 3. Verificar Logs
O programa agora imprime mensagens de debug detalhadas. Procure por:
- `ERROR:` - Indica problemas específicos
- `Initializing...` - Mostra o progresso da inicialização
- `Randomizer initialized` - Confirma que as peças foram carregadas

### 4. Verificar Arquivos de Configuração
```bash
# Verificar se os arquivos existem
ls -la *.cfg *.pieces

# Testar com configuração específica
export DROPBLOCKS_CFG=raspberry_pi.cfg
./dropblocks_debug
```

## Soluções Específicas

### Se o problema for SDL2:
```bash
sudo apt-get update
sudo apt-get install -y libsdl2-dev libsdl2-2.0-0
```

### Se o problema for memória:
```bash
# Verificar memória disponível
free -h

# Executar com valgrind
valgrind --tool=memcheck ./dropblocks_debug
```

### Se o problema for arquitetura:
```bash
# Verificar arquitetura
uname -m

# Compilar com flags específicas para ARM
g++ -march=armv7-a -mfpu=neon -mfloat-abi=hard dropblocks.cpp -o dropblocks
```

## Configurações Recomendadas para Raspberry Pi

1. **Desabilitar efeitos visuais pesados** (já configurado em `raspberry_pi.cfg`)
2. **Reduzir resolução** se necessário
3. **Usar versão debug** para identificar problemas
4. **Verificar logs** para mensagens de erro

## Contato e Suporte

Se o problema persistir, execute o script de debug e envie:
1. A saída completa do `./debug_raspberry_pi.sh`
2. A saída do `ldd dropblocks_debug`
3. A saída do `uname -a`
4. A versão do SDL2 (`pkg-config --modversion sdl2`)

## Arquivos Modificados

- `dropblocks.cpp` - Adicionadas verificações de segurança
- `build_raspberry_pi.sh` - Script de compilação otimizado
- `debug_raspberry_pi.sh` - Script de debugging
- `raspberry_pi.cfg` - Configuração otimizada para Raspberry Pi

