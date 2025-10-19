# Performance de Retângulos Arredondados

## Métodos Implementáveis (do mais lento ao mais rápido)

### 1. Método Atual - Pixel por Pixel (LENTO)
```cpp
// Complexidade: O(w × h)
// Para 300×500: 150.000 chamadas SDL_RenderDrawPoint()
for (yy=0; yy<h; ++yy)
    for (xx=0; xx<w; ++xx)
        SDL_RenderDrawPoint(r, x+xx, y+yy);
```
**Prós**: Perfeição matemática, sem distorção  
**Contras**: Muito lento para áreas grandes

---

### 2. Método por Linhas (RÁPIDO - original do código)
```cpp
// Complexidade: O(h)
// Para 300×500: 500 chamadas SDL_RenderFillRect()
for (int yy=0; yy<h; ++yy){
    int left = x, right = x + w - 1;
    if (yy < rad){
        int dy = rad-1-yy; 
        int dx = (int)std::floor(std::sqrt((double)rad*rad - (double)dy*dy));
        left = x + rad - dx; right = x + w - rad + dx - 1;
    }
    SDL_Rect line{ left, y + yy, right - left + 1, 1 };
    SDL_RenderFillRect(r, &line);
}
```
**Prós**: 300× mais rápido que pixel-a-pixel  
**Contras**: Pode ter micro-distorções em resoluções não-quadradas

---

### 3. Método Híbrido - RECOMENDADO (EQUILÍBRIO)
```cpp
// Desenha retângulo central + 4 cantos separadamente
// Cantos: 4 × O(rad²), Centro: 3 retângulos
SDL_RenderFillRect(r, {x+rad, y, w-2*rad, rad});        // Topo
SDL_RenderFillRect(r, {x, y+rad, w, h-2*rad});          // Centro
SDL_RenderFillRect(r, {x+rad, y+h-rad, w-2*rad, rad});  // Base
// + 4 cantos desenhados pixel-a-pixel apenas nos rad×rad
```
**Prós**: Rápido E perfeito  
**Contras**: Código mais complexo

---

### 4. Método com Texturas (MAIS RÁPIDO)
```cpp
// Pré-renderiza cantos arredondados em texturas na inicialização
// Durante o jogo, apenas compõe as texturas
SDL_RenderCopy(renderer, corner_texture_tl, NULL, {x, y, rad, rad});
SDL_RenderCopy(renderer, corner_texture_tr, NULL, {x+w-rad, y, rad, rad});
// ... + retângulos centrais
```
**Prós**: Extremamente rápido (GPU), anti-aliasing possível  
**Contras**: Usa memória VRAM, complexidade de implementação

---

### 5. Método com SDL_gfx (EXTERNO)
```cpp
// Biblioteca SDL_gfx fornece primitivas otimizadas
roundedBoxRGBA(renderer, x, y, x+w, y+h, rad, r, g, b, a);
```
**Prós**: Otimizado, testado, anti-aliasing  
**Contras**: Dependência externa

---

## Benchmark Estimado (para painel 300×500px, rad=12)

| Método | Chamadas SDL | FPS Estimado | Qualidade |
|--------|-------------|--------------|-----------|
| Pixel-a-pixel | 150.000 | ~60 fps | ⭐⭐⭐⭐⭐ Perfeito |
| Por linhas | 500 | ~200 fps | ⭐⭐⭐⭐ Ótimo |
| Híbrido | ~580 | ~180 fps | ⭐⭐⭐⭐⭐ Perfeito |
| Texturas | 8 | ~300+ fps | ⭐⭐⭐⭐⭐ Perfeito |
| SDL_gfx | 1 | ~250 fps | ⭐⭐⭐⭐⭐ Perfeito |

---

## Recomendação para DropBlocks

Para um jogo Tetris com ~6 painéis estáticos:
- **Usar método HÍBRIDO** ou **voltar ao método por LINHAS**
- Os painéis são redesenhados 60× por segundo, mas a geometria é simples
- Com 6 painéis × 500 linhas = 3000 chamadas/frame → ainda rápido
- **Alternativa**: Desenhar painéis em textura off-screen uma vez, reusar

---

## Implementação do Método Híbrido (Recomendado)

Ver: `src/render/Primitives_optimized.cpp` (exemplo)

