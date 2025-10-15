# DropBlocks - Guia Completo de Configuração 🎮

Este documento explica **TODAS** as configurações possíveis do DropBlocks, incluindo arquivos `.cfg` e `.pieces`.

## 📋 Índice

1. [Como Usar Configurações Personalizadas](#como-usar-configurações-personalizadas)
2. [Arquivos de Configuração (.cfg)](#arquivos-de-configuração-cfg)
3. [Arquivos de Peças (.pieces)](#arquivos-de-peças-pieces)
4. [Exemplos Práticos](#exemplos-práticos)
5. [Troubleshooting](#troubleshooting)

---

## 🚀 Como Usar Configurações Personalizadas

### Método 1: Variável de Ambiente (Recomendado)

```bash
# Windows (PowerShell)
$env:DROPBLOCKS_CFG="meu_tema.cfg"; ./dropblocks.exe

# Windows (CMD)
set DROPBLOCKS_CFG=meu_tema.cfg && dropblocks.exe

# Linux/macOS
DROPBLOCKS_CFG=meu_tema.cfg ./dropblocks
```

### Método 2: Arquivo Padrão

O jogo procura por arquivos de configuração nesta ordem:
1. `default.cfg`
2. `dropblocks.cfg`

### Método 3: Linha de Comando

```bash
# Especificar arquivo de configuração
./dropblocks meu_tema.cfg

# Especificar arquivo de peças
./dropblocks meu_tema.cfg minhas_pecas.pieces
```

---

## ⚙️ Arquivos de Configuração (.cfg)

### 📝 Formato Básico

```ini
# Comentários começam com # ou ;
# Formato: CHAVE = VALOR

TITLE_TEXT = MEU TETRIS
BG = #08080C
ENABLE_BANNER_SWEEP = true
```

### 🎨 Configurações de Cores

#### Cores Básicas
| Chave | Descrição | Formato | Padrão |
|-------|-----------|---------|--------|
| `BG` | Cor de fundo | `#RRGGBB` ou `RRGGBB` | `#08080C` |
| `BOARD_EMPTY` | Cor das células vazias | `#RRGGBB` ou `RRGGBB` | `#1C1C24` |
| `PANEL_FILL` | Cor de preenchimento dos painéis | `#RRGGBB` ou `RRGGBB` | `#181820` |
| `PANEL_OUTLINE` | Cor da borda dos painéis | `#RRGGBB` ou `RRGGBB` | `#5A5A78` |

#### Cores do Banner
| Chave | Descrição | Formato | Padrão |
|-------|-----------|---------|--------|
| `BANNER_BG` | Fundo do banner | `#RRGGBB` ou `RRGGBB` | `#002800` |
| `BANNER_OUTLINE` | Borda do banner | `#RRGGBB` ou `RRGGBB` | `#003C00` |
| `BANNER_TEXT` | Texto do banner | `#RRGGBB` ou `RRGGBB` | `#78FF78` |

#### Cores do HUD
| Chave | Descrição | Formato | Padrão |
|-------|-----------|---------|--------|
| `HUD_LABEL` | Textos de label | `#RRGGBB` ou `RRGGBB` | `#C8C8DC` |
| `HUD_SCORE` | Cor da pontuação | `#RRGGBB` ou `RRGGBB` | `#FFF078` |
| `HUD_LINES` | Cor das linhas | `#RRGGBB` ou `RRGGBB` | `#B4FFB4` |
| `HUD_LEVEL` | Cor do nível | `#RRGGBB` ou `RRGGBB` | `#B4C8FF` |

#### Cores do Painel NEXT
| Chave | Descrição | Formato | Padrão |
|-------|-----------|---------|--------|
| `NEXT_FILL` | Fundo do painel NEXT | `#RRGGBB` ou `RRGGBB` | `#12121A` |
| `NEXT_OUTLINE` | Borda do painel NEXT | `#RRGGBB` ou `RRGGBB` | `#50506E` |
| `NEXT_LABEL` | Texto "NEXT" | `#RRGGBB` ou `RRGGBB` | `#DCDCDC` |

#### Cores do Overlay (Game Over)
| Chave | Descrição | Formato | Padrão |
|-------|-----------|---------|--------|
| `OVERLAY_FILL` | Fundo do overlay | `#RRGGBB` ou `RRGGBB` | `#000000` |
| `OVERLAY_OUTLINE` | Borda do overlay | `#RRGGBB` ou `RRGGBB` | `#C8C8DC` |
| `OVERLAY_TOP` | Texto principal | `#RRGGBB` ou `RRGGBB` | `#FFA0A0` |
| `OVERLAY_SUB` | Texto secundário | `#RRGGBB` ou `RRGGBB` | `#DCDCDC` |

#### Cores das Peças
| Chave | Descrição | Formato | Padrão |
|-------|-----------|---------|--------|
| `PIECE0` | Cor da peça 0 | `#RRGGBB` ou `RRGGBB` | `#DC5050` |
| `PIECE1` | Cor da peça 1 | `#RRGGBB` ou `RRGGBB` | `#50B478` |
| `PIECE2` | Cor da peça 2 | `#RRGGBB` ou `RRGGBB` | `#5078DC` |
| `PIECE3` | Cor da peça 3 | `#RRGGBB` ou `RRGGBB` | `#DCA050` |
| `PIECE4` | Cor da peça 4 | `#RRGGBB` ou `RRGGBB` | `#A050DC` |
| `PIECE5` | Cor da peça 5 | `#RRGGBB` ou `RRGGBB` | `#50DCDC` |
| `PIECE6` | Cor da peça 6 | `#RRGGBB` ou `RRGGBB` | `#DCDC50` |
| `PIECE7` | Cor da peça 7 | `#RRGGBB` ou `RRGGBB` | `#DC50DC` |
| `PIECE8+` | Cores adicionais | `#RRGGBB` ou `RRGGBB` | `#C8C8C8` |

### 🎛️ Configurações de Transparência

| Chave | Descrição | Range | Padrão |
|-------|-----------|-------|--------|
| `PANEL_OUTLINE_A` | Transparência da borda dos painéis | 0-255 | 200 |
| `BANNER_OUTLINE_A` | Transparência da borda do banner | 0-255 | 180 |
| `NEXT_OUTLINE_A` | Transparência da borda do NEXT | 0-255 | 160 |
| `OVERLAY_FILL_A` | Transparência do fundo do overlay | 0-255 | 200 |
| `OVERLAY_OUTLINE_A` | Transparência da borda do overlay | 0-255 | 120 |

### 🎨 Configurações de Layout

| Chave | Descrição | Range | Padrão |
|-------|-----------|-------|--------|
| `TITLE_TEXT` | Texto do banner (A-Z e espaço) | String | `"---H A C K T R I S"` |
| `ROUNDED_PANELS` | Painéis arredondados | 0-1 | 1 |
| `HUD_FIXED_SCALE` | Escala do HUD | 1-20 | 6 |
| `GAP1_SCALE` | Espaço banner ↔ tabuleiro | 1-50 | 10 |
| `GAP2_SCALE` | Espaço tabuleiro ↔ painel | 1-50 | 10 |

### ✨ Efeitos Visuais

#### Sweep do Banner
| Chave | Descrição | Range | Padrão |
|-------|-----------|-------|--------|
| `ENABLE_BANNER_SWEEP` | Ativar sweep no banner | true/false | true |
| `SWEEP_SPEED_PXPS` | Velocidade do sweep (px/s) | 0.1-100.0 | 15.0 |
| `SWEEP_BAND_H_S` | Altura da banda (em scale) | 1-100 | 30 |
| `SWEEP_ALPHA_MAX` | Intensidade máxima | 0-255 | 100 |
| `SWEEP_SOFTNESS` | Suavidade (0.1-1.0) | 0.1-1.0 | 0.7 |

#### Sweep Global
| Chave | Descrição | Range | Padrão |
|-------|-----------|-------|--------|
| `ENABLE_GLOBAL_SWEEP` | Ativar sweep global | true/false | true |
| `SWEEP_G_SPEED_PXPS` | Velocidade global (px/s) | 0.1-100.0 | 20.0 |
| `SWEEP_G_BAND_H_PX` | Altura da banda (px) | 10-500 | 100 |
| `SWEEP_G_ALPHA_MAX` | Intensidade máxima | 0-255 | 50 |
| `SWEEP_G_SOFTNESS` | Suavidade | 0.1-1.0 | 0.9 |

#### Scanlines
| Chave | Descrição | Range | Padrão |
|-------|-----------|-------|--------|
| `SCANLINE_ALPHA` | Intensidade das scanlines | 0-255 | 20 |

### 🎵 Configurações de Áudio

| Chave | Descrição | Range | Padrão |
|-------|-----------|-------|--------|
| `AUDIO_MASTER_VOLUME` | Volume geral | 0.0-1.0 | 1.0 |
| `AUDIO_SFX_VOLUME` | Volume dos efeitos sonoros | 0.0-1.0 | 0.8 |
| `AUDIO_AMBIENT_VOLUME` | Volume ambiente | 0.0-1.0 | 1.0 |
| `ENABLE_MOVEMENT_SOUNDS` | Sons de movimento | true/false | true |
| `ENABLE_AMBIENT_SOUNDS` | Sons ambiente | true/false | true |
| `ENABLE_COMBO_SOUNDS` | Sons de combo | true/false | true |
| `ENABLE_LEVEL_UP_SOUNDS` | Sons de level up | true/false | true |

### 🎲 Configurações de Peças

| Chave | Descrição | Range | Padrão |
|-------|-----------|-------|--------|
| `PIECES_FILE` | Arquivo de peças | String | `""` |
| `PREVIEW_GRID` | Tamanho da grade NEXT | 4-12 | 6 |

### 🎯 Configurações de Randomizer

| Chave | Descrição | Valores | Padrão |
|-------|-----------|---------|--------|
| `RAND_TYPE` | Tipo de randomizer | `simple`, `bag` | `simple` |
| `RAND_BAG_SIZE` | Tamanho da bag | 0-20 | 0 |

### 🎨 Grid Colorido do NEXT

| Chave | Descrição | Formato | Padrão |
|-------|-----------|---------|--------|
| `NEXT_GRID_DARK_COLOR` | Cor escura da grade | `#RRGGBB` ou `RRGGBB` | `#181818` |
| `NEXT_GRID_LIGHT_COLOR` | Cor clara da grade | `#RRGGBB` ou `RRGGBB` | `#1E1E1E` |

---

## 🧩 Arquivos de Peças (.pieces)

### 📝 Formato Básico

```ini
[SET]
NAME = Nome do Set
PREVIEWGRID = 4
RANDOMIZER = bag
BAGSIZE = 7

[PIECE.I]
NAME = I-Piece
COLOR = #00FFFF
ROTATIONS = auto
BASE = (0,0);(1,0);(2,0);(3,0)
KICKS.CW = (0,0);(-2,0);(1,0);(-2,-1);(1,2)
KICKS.CCW = (0,0);(-1,0);(2,0);(-1,2);(2,-1)
```

### 🎯 Configurações do Set

| Chave | Descrição | Valores | Padrão |
|-------|-----------|---------|--------|
| `NAME` | Nome do conjunto | String | `"Custom Set"` |
| `PREVIEWGRID` | Tamanho da grade de preview | 4-12 | 4 |
| `RANDOMIZER` | Tipo de randomizer | `simple`, `bag` | `simple` |
| `BAGSIZE` | Tamanho da bag | 1-20 | 7 |

### 🧩 Configurações das Peças

#### Cabeçalho da Peça
```ini
[PIECE.X]
NAME = Nome da Peça
COLOR = #RRGGBB
ROTATIONS = auto|explicit
```

#### Rotações Automáticas
```ini
ROTATIONS = auto
BASE = (0,0);(1,0);(0,1);(1,1)
KICKS.CW = (0,0)
KICKS.CCW = (0,0)
```

#### Rotações Explícitas
```ini
ROTATIONS = explicit
ROT0 = (-1,0);(0,0);(+1,0);(+2,0)
ROT1 = (0,-1);(0,0);(0,+1);(0,+2)
ROT2 = sameas:Rot0
ROT3 = sameas:Rot1
```

#### Sistema de Kicks SRS
```ini
# Kicks por transição (recomendado)
KICKS.CW.0-1 = (0,0);(-1,0);(-1,1);(0,-2);(-1,-2)
KICKS.CW.1-2 = (0,0);(1,0);(1,-1);(0,2);(1,2)
KICKS.CW.2-3 = (0,0);(1,0);(1,1);(0,-2);(1,-2)
KICKS.CW.3-0 = (0,0);(-1,0);(-1,-1);(0,2);(-1,2)

KICKS.CCW.0-3 = (0,0);(1,0);(1,1);(0,-2);(1,-2)
KICKS.CCW.3-2 = (0,0);(-1,0);(-1,-1);(0,2);(-1,2)
KICKS.CCW.2-1 = (0,0);(-1,0);(-1,1);(0,-2);(-1,-2)
KICKS.CCW.1-0 = (0,0);(1,0);(1,-1);(0,2);(1,2)
```

### 📐 Formato de Coordenadas

- **Coordenadas**: `(x,y)` onde x é horizontal, y é vertical
- **Sinais**: `+` ou `-` para direção (opcional)
- **Separador**: `;` entre coordenadas
- **Exemplo**: `(0,0);(1,0);(0,1);(1,1)`

### 🎨 Cores das Peças

- **Formato**: `#RRGGBB` ou `RRGGBB`
- **Prioridade**: Arquivo `.cfg` > Arquivo `.pieces`
- **Padrão**: Cinza `#C8C8C8` se não especificado

---

## 💡 Exemplos Práticos

### 🎨 Tema Cyberpunk

```ini
# Configuração Cyberpunk
TITLE_TEXT = CYBER TETRIS
BG = #0A0F2A
BOARD_EMPTY = #1A1F3A
PANEL_FILL = #0F1A3F
PANEL_OUTLINE = #4A5A8F

BANNER_BG = #001122
BANNER_OUTLINE = #00AAFF
BANNER_TEXT = #00FFFF

HUD_LABEL = #E0F0FF
HUD_SCORE = #FFFF80
HUD_LINES = #80FF80
HUD_LEVEL = #80C0FF

# Efeitos intensos
ENABLE_BANNER_SWEEP = true
ENABLE_GLOBAL_SWEEP = true
SWEEP_SPEED_PXPS = 30.0
SWEEP_ALPHA_MAX = 80
SCANLINE_ALPHA = 35

# Cores neon das peças
PIECE0 = #FF0040
PIECE1 = #00FF80
PIECE2 = #0080FF
PIECE3 = #FF8000
PIECE4 = #8000FF
```

### 🌈 Tema Arco-Íris

```ini
# Configuração Arco-Íris
TITLE_TEXT = RAINBOW TETRIS
BG = #0A0A0A
BOARD_EMPTY = #1A1A1A

# Cores das peças em arco-íris
PIECE0 = #FF0000  ; Vermelho
PIECE1 = #FF8000  ; Laranja
PIECE2 = #FFFF00  ; Amarelo
PIECE3 = #80FF00  ; Verde-amarelo
PIECE4 = #00FF00  ; Verde
PIECE5 = #00FF80  ; Verde-ciano
PIECE6 = #00FFFF  ; Ciano
PIECE7 = #0080FF  ; Azul-ciano
PIECE8 = #0000FF  ; Azul
PIECE9 = #8000FF  ; Azul-roxo
```

### 🎵 Configuração de Áudio Personalizada

```ini
# Áudio personalizado
AUDIO_MASTER_VOLUME = 0.8
AUDIO_SFX_VOLUME = 0.6
AUDIO_AMBIENT_VOLUME = 0.4

# Desabilitar sons específicos
ENABLE_AMBIENT_SOUNDS = false
ENABLE_LEVEL_UP_SOUNDS = false
```

### 🧩 Peças Personalizadas

```ini
[SET]
NAME = Peças Personalizadas
PREVIEWGRID = 6
RANDOMIZER = bag
BAGSIZE = 5

[PIECE.PENTA]
NAME = Pentaminó
COLOR = #FF00FF
ROTATIONS = explicit
ROT0 = (0,0);(1,0);(2,0);(0,1);(1,1)
ROT1 = (0,0);(0,1);(0,2);(-1,1);(-1,2)
ROT2 = sameas:Rot0
ROT3 = sameas:Rot1
KICKS.CW = (0,0)
KICKS.CCW = (0,0)
```

---

## 🔧 Troubleshooting

### ❌ Problemas Comuns

**Configuração não carrega:**
- Verifique se o arquivo existe
- Verifique a sintaxe (chave = valor)
- Use variável de ambiente `DROPBLOCKS_CFG`

**Cores não aparecem:**
- Verifique formato: `#RRGGBB` ou `RRGGBB`
- Verifique se a chave está correta
- Cores das peças: arquivo `.cfg` tem prioridade

**Efeitos não funcionam:**
- Verifique valores booleanos: `true`/`false`
- Verifique ranges dos valores
- Reinicie o jogo após mudanças

**Peças não carregam:**
- Verifique sintaxe do arquivo `.pieces`
- Verifique se `PIECES_FILE` está correto
- Verifique coordenadas das peças

### ✅ Dicas de Performance

- **Scanlines**: Valores altos podem impactar performance
- **Sweep**: Velocidades muito altas podem causar lag
- **Cores**: Muitas peças coloridas podem impactar FPS
- **Áudio**: Volumes baixos economizam CPU

### 🎯 Validação de Configuração

```bash
# Testar configuração
DROPBLOCKS_CFG=teste.cfg ./dropblocks

# Verificar logs no console
# O jogo mostra qual arquivo foi carregado
```

---

## 📚 Referência Rápida

### 🎨 Cores Mais Usadas
- **Preto**: `#000000`
- **Branco**: `#FFFFFF`
- **Vermelho**: `#FF0000`
- **Verde**: `#00FF00`
- **Azul**: `#0000FF`
- **Amarelo**: `#FFFF00`
- **Ciano**: `#00FFFF`
- **Magenta**: `#FF00FF`

### ⚡ Valores Recomendados
- **HUD_FIXED_SCALE**: 4-8
- **SWEEP_SPEED_PXPS**: 15-30
- **SCANLINE_ALPHA**: 15-35
- **AUDIO_MASTER_VOLUME**: 0.7-1.0

### 🎯 Configurações por Tema
- **Retro**: Scanlines altas, cores saturadas
- **Moderno**: Sweep suave, cores pastel
- **Cyberpunk**: Efeitos intensos, cores neon
- **Minimalista**: Efeitos desabilitados, cores neutras

---

**Divirta-se personalizando seu DropBlocks!** 🎮✨
