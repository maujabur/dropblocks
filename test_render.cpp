/**
 * @file test_render.cpp
 * @brief Teste de renderização básica do DropBlocks
 * @author DropBlocks Team
 * @version 5.0
 * @date 2025
 * 
 * Testa apenas a renderização básica sem lógica de jogo
 */

#include <SDL2/SDL.h>
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char* argv[]) {
    printf("=== DROPBLOCKS RENDER TEST v5.1 ===\n");
    printf("Testing basic rendering without game logic...\n");
    fflush(stdout);

    // Inicializar SDL2
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        printf("ERROR: SDL_Init failed: %s\n", SDL_GetError());
        return 1;
    }
    printf("SUCCESS: SDL2 initialized\n");
    fflush(stdout);

    // Criar janela
    SDL_Window* win = SDL_CreateWindow("DropBlocks Render Test", 
                                      SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                      800, 600, SDL_WINDOW_SHOWN);
    if (!win) {
        printf("ERROR: Failed to create window: %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }
    printf("SUCCESS: Window created\n");
    fflush(stdout);

    // Criar renderer
    SDL_Renderer* ren = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED);
    if (!ren) {
        printf("WARNING: Accelerated renderer failed, trying software...\n");
        ren = SDL_CreateRenderer(win, -1, SDL_RENDERER_SOFTWARE);
        if (!ren) {
            printf("ERROR: Failed to create renderer: %s\n", SDL_GetError());
            SDL_DestroyWindow(win);
            SDL_Quit();
            return 1;
        }
    }
    printf("SUCCESS: Renderer created\n");
    fflush(stdout);

    // Teste de renderização básica
    printf("Testing basic rendering...\n");
    fflush(stdout);
    
    // Limpar tela
    SDL_SetRenderDrawColor(ren, 0, 0, 0, 255);
    SDL_RenderClear(ren);
    printf("SUCCESS: Screen cleared\n");
    fflush(stdout);
    
    // Desenhar um retângulo
    SDL_SetRenderDrawColor(ren, 255, 0, 0, 255);
    SDL_Rect rect = {100, 100, 200, 200};
    SDL_RenderFillRect(ren, &rect);
    printf("SUCCESS: Rectangle drawn\n");
    fflush(stdout);
    
    // Apresentar
    SDL_RenderPresent(ren);
    printf("SUCCESS: Frame presented\n");
    fflush(stdout);
    
    // Aguardar
    printf("Waiting 3 seconds...\n");
    fflush(stdout);
    SDL_Delay(3000);
    
    // Cleanup
    printf("Cleaning up...\n");
    fflush(stdout);
    SDL_DestroyRenderer(ren);
    SDL_DestroyWindow(win);
    SDL_Quit();
    
    printf("=== RENDER TEST COMPLETED SUCCESSFULLY ===\n");
    fflush(stdout);
    
    return 0;
}
