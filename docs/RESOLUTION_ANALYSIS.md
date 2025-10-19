# Análise de Resolução e Renderização no DropBlocks

## Fluxo Completo: Da Resolução aos Pixels

### 1️⃣ **Inicialização da Janela**
```
src/app/GameInitializer.cpp (linha 139-146)
```

```cpp
SDL_DisplayMode dm;
SDL_GetCurrentDisplayMode(0, &dm);  // Obtém resolução do monitor
int SW = dm.w, SH = dm.h;           // Ex: 1920×1080

win = SDL_CreateWindow("DropBlocks", ..., SW, SH,
                      SDL_WINDOW_FULLSCREEN | SDL_WINDOW_ALLOW_HIGHDPI);
ren = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
```

**Flags Importantes:**
- `SDL_WINDOW_FULLSCREEN`: Usa resolução nativa do monitor
- `SDL_WINDOW_ALLOW_HIGHDPI`: Permite DPI scaling (importante!)

---

### 2️⃣ **Obtenção da Resolução Real de Renderização**
```
src/render/LayoutHelpers.cpp (linha 14-20)
```

```cpp
void db_layoutCalculate(LayoutCache& layout, SDL_Renderer* renderer) {
    int w, h;
    SDL_GetRendererOutputSize(renderer, &w, &h);  // ⚠️ CRÍTICO!
    layout.SWr = w;  // Screen Width (real)
    layout.SHr = h;  // Screen Height (real)
}
```

**Por que `SDL_GetRendererOutputSize()` e não `SDL_GetCurrentDisplayMode()`?**

| Monitor | DisplayMode | RendererOutputSize | Diferença |
|---------|-------------|-------------------|-----------|
| 1920×1080 (FullHD) | 1920×1080 | 1920×1080 | Nenhuma |
| 3840×2160 (4K) | 3840×2160 | 3840×2160 | Nenhuma |
| 1920×1080 (HiDPI 2x) | 1920×1080 | 3840×2160 | **DOBRO!** |

No Windows com "Display Scaling 200%":
- Sistema reporta: 1920×1080 (lógico)
- Renderer desenha em: 3840×2160 (físico)
- **Sem correção**: distorção nos retângulos arredondados!

---

### 3️⃣ **Cálculo do Canvas 16:9**
```
src/render/LayoutHelpers.cpp (linha 22-30)
```

```cpp
// Calcular área de renderização 16:9 centralizada
if (layout.SWr * 9 >= layout.SHr * 16) {
    // Tela mais LARGA que 16:9 → barras laterais
    layout.CH = layout.SHr;
    layout.CW = (layout.CH * 16) / 9;
    layout.CX = (layout.SWr - layout.CW) / 2;  // Centralizar
    layout.CY = 0;
} else {
    // Tela mais ALTA que 16:9 → barras superiores/inferiores
    layout.CW = layout.SWr;
    layout.CH = (layout.CW * 9) / 16;
    layout.CX = 0;
    layout.CY = (layout.SHr - layout.CH) / 2;  // Centralizar
}
```

**Variáveis do Canvas:**
- `SWr, SHr`: Resolução real do renderer
- `CW, CH`: Largura/altura do canvas 16:9
- `CX, CY`: Posição do canvas (para centralização)

**Exemplos:**

| Monitor | SWr×SHr | CW×CH | CX, CY | Barras |
|---------|---------|-------|--------|--------|
| 1920×1080 (16:9) | 1920×1080 | 1920×1080 | 0, 0 | Nenhuma |
| 1920×1200 (16:10) | 1920×1200 | 1920×1080 | 0, 60 | Topo/Base |
| 2560×1080 (21:9) | 2560×1080 | 1920×1080 | 320, 0 | Laterais |
| 800×600 (4:3) | 800×600 | 800×450 | 0, 75 | Topo/Base |

---

### 4️⃣ **Cálculo da Escala do HUD**
```
src/render/LayoutHelpers.cpp (linha 32-37)
```

```cpp
layout.scale = HUD_FIXED_SCALE;  // Configurável (default: 6)
layout.GAP1 = BORDER + GAP1_SCALE * layout.scale;
layout.GAP2 = BORDER + GAP2_SCALE * layout.scale;
layout.bannerW = 8 * layout.scale + 24;
layout.panelTarget = (int)(layout.CW * 0.28);
```

**Variáveis Configuráveis:**
- `HUD_FIXED_SCALE` (default.cfg: 6): Multiplicador de escala de texto/UI
- `GAP1_SCALE` (default.cfg: 10): Espaçamento 1 × scale
- `GAP2_SCALE` (default.cfg: 10): Espaçamento 2 × scale
- `BORDER` (código: 10): Borda fixa em pixels

**Como afetam o layout:**
```
Para HUD_FIXED_SCALE = 6:
- Texto de 5×7 pixels → 30×42 pixels
- GAP1 = 10 + (10 × 6) = 70 pixels
- Banner width = 8 × 6 + 24 = 72 pixels
```

---

### 5️⃣ **Cálculo do Tamanho das Células**
```
src/render/LayoutHelpers.cpp (linha 39-41)
```

```cpp
int cellW = layout.usableLeftW / COLS;  // 10 colunas
int cellH = (layout.CH - 2 * BORDER) / ROWS;  // 20 linhas
layout.cellBoard = std::min(std::max(8, cellW), cellH);
```

**Lógica:**
1. Calcula espaço disponível horizontal para o tabuleiro
2. Divide por 10 colunas → `cellW`
3. Calcula espaço vertical
4. Divide por 20 linhas → `cellH`
5. Usa o **menor** valor (para manter proporção)
6. **Mínimo: 8 pixels** (para telas muito pequenas)

**Exemplo (1920×1080):**
```
CH = 1080
usableLeftW ≈ 500
cellW = 500 / 10 = 50
cellH = (1080 - 20) / 20 = 53
cellBoard = min(50, 53) = 50 pixels ✓
```

---

### 6️⃣ **Posicionamento dos Elementos**
```
src/render/LayoutHelpers.cpp (linha 43-67)
```

```cpp
layout.GW = layout.cellBoard * COLS;      // Game board Width
layout.GH = layout.cellBoard * ROWS;      // Game board Height
layout.BX = layout.CX + BORDER;           // Banner X
layout.BY = layout.CY + (layout.CH - layout.GH) / 2;  // Banner Y (centralizado)
layout.GX = layout.BX + layout.BW + layout.statsMargin + layout.statsBoxW + layout.statsMargin;
layout.panelX = layout.GX + layout.GW + layout.GAP2;
layout.panelW = layout.CX + layout.CW - layout.panelX - BORDER;
```

**Layout Final:**
```
┌─────────────────────────────────────────┐
│ CX=320                     Canvas 16:9  │
│  ┌──┬──────────┬───────────────────┐    │
│  │B │ STATS    │  GAME BOARD       │ P  │
│  │A │          │  10×20 células    │ A  │
│  │N │ PIECES   │                   │ N  │
│  │N │ I: 999   │                   │ E  │
│  │E │ O: 999   │   NEXT            │ L  │
│  │R │ ...      │   Preview         │    │
│  └──┴──────────┴───────────────────┘ H  │
│                                      U  │
│                                      D  │
└─────────────────────────────────────────┘
 SWr = 2560 (real screen width)
```

---

### 7️⃣ **Renderização dos Retângulos Arredondados**
```
src/render/Layers.cpp (linhas 46, 144, 189, 274, 363)
src/render/Primitives.cpp (linha 106-146)
```

```cpp
// Desenha painel com cantos arredondados
drawRoundedFilled(renderer, layout.panelX, layout.panelY, 
                  layout.panelW, layout.panelH, 12, r, g, b, a);
```

**Algoritmo (pixel-perfect circle):**
```cpp
for (int yy=0; yy<h; ++yy){
    for (int xx=0; xx<w; ++xx){
        // Para cada canto, verifica se está dentro do círculo
        int dx = ... , dy = ...;
        if (dx*dx + dy*dy > rad*rad) continue;  // Fora do círculo
        SDL_RenderDrawPoint(r, x + xx, y + yy);
    }
}
```

**Por que funciona agora:**
- Usa coordenadas **físicas** do renderer (via `SDL_GetRendererOutputSize`)
- Cada pixel é desenhado em coordenadas 1:1
- Círculo mantém proporção correta (dx² + dy² = r²)
- **Sem distorção!**

---

## Variáveis de Configuração

### Em `default.cfg`:
```ini
# Layout
HUD_FIXED_SCALE = 6      # Escala do texto/UI (1-10)
GAP1_SCALE = 10          # Espaçamento entre banner e stats
GAP2_SCALE = 10          # Espaçamento entre board e painel
ROUNDED_PANELS = 1       # 1=arredondado, 0=retangular
```

### Em `dropblocks.cpp`:
```cpp
const int COLS = 10;     # Colunas do tabuleiro
const int ROWS = 20;     # Linhas do tabuleiro
int BORDER = 10;         # Borda em pixels
```

---

## Impacto da Resolução

### Resoluções Baixas (800×600)
```
cellBoard ≈ 20px
HUD_FIXED_SCALE = 3-4 (recomendado)
Painéis menores, menos detalhes
```

### Resoluções Médias (1920×1080)
```
cellBoard ≈ 50px
HUD_FIXED_SCALE = 6 (padrão)
Layout ideal
```

### Resoluções Altas (3840×2160 - 4K)
```
cellBoard ≈ 100px
HUD_FIXED_SCALE = 10-12 (recomendado)
Painéis muito grandes, texto pequeno se não ajustar
```

---

## Fluxo Resumido

```
1. Monitor → SDL_GetRendererOutputSize() → SWr×SHr (resolução física)
2. SWr×SHr → Cálculo 16:9 → CW×CH + CX,CY (canvas centralizado)
3. CW×CH → Cálculo de células → cellBoard (tamanho das peças)
4. cellBoard → Layout dos elementos → BX,BY,GX,GY,panelX,panelY
5. Layout → Renderização → Retângulos arredondados pixel-perfect
```

---

## Problemas Resolvidos

### ❌ ANTES (com bug):
```cpp
SDL_GetCurrentDisplayMode(0, &dm);  // Resolução LÓGICA
// Monitor 1920×1080 com scaling 200%
// dm.w = 1920, dm.h = 1080
// Mas renderer desenha em 3840×2160!
// → Distorção nos círculos
```

### ✅ DEPOIS (corrigido):
```cpp
SDL_GetRendererOutputSize(renderer, &w, &h);  // Resolução FÍSICA
// w = 3840, h = 2160 (real)
// Cálculos usam resolução correta
// → Círculos perfeitos!
```

---

## Conclusão

A resolução afeta:
1. **Tamanho das células**: Quanto maior a resolução, maiores as células
2. **Escala do UI**: Deve ajustar `HUD_FIXED_SCALE` para resoluções diferentes
3. **Precisão dos círculos**: Usa coordenadas físicas do renderer
4. **Centralização**: Canvas 16:9 sempre centralizado com barras pretas
5. **Performance**: Mais pixels = mais trabalho (especialmente em 4K)

**Recomendação**: Para suportar múltiplas resoluções, implementar auto-scaling do `HUD_FIXED_SCALE` baseado na resolução.

