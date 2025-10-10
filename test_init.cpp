/**
 * @file test_init.cpp
 * @brief Teste de inicialização básica do DropBlocks para Raspberry Pi
 * @author DropBlocks Team
 * @version 1.0
 * @date 2025
 * 
 * Versão simplificada para testar apenas a inicialização do SDL2
 * e identificar onde está ocorrendo o segmentation fault.
 */

#include <SDL2/SDL.h>
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char* argv[]) {
    printf("=== DROPBLOCKS INITIALIZATION TEST ===\n");
    printf("Testing SDL2 initialization step by step...\n");
    fflush(stdout);

    // Teste 1: Inicialização básica do SDL2
    printf("\n1. Testing SDL2 basic initialization...\n");
    fflush(stdout);
    
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        printf("ERROR: SDL_INIT_VIDEO failed: %s\n", SDL_GetError());
        return 1;
    }
    printf("SUCCESS: SDL_INIT_VIDEO OK\n");
    fflush(stdout);

    // Teste 2: Verificar displays
    printf("\n2. Testing display detection...\n");
    fflush(stdout);
    
    int numDisplays = SDL_GetNumVideoDisplays();
    printf("Number of displays: %d\n", numDisplays);
    fflush(stdout);
    
    if (numDisplays <= 0) {
        printf("ERROR: No displays available\n");
        SDL_Quit();
        return 1;
    }

    // Teste 3: Obter modo de display
    printf("\n3. Testing display mode detection...\n");
    fflush(stdout);
    
    SDL_DisplayMode dm;
    if (SDL_GetCurrentDisplayMode(0, &dm) != 0) {
        printf("ERROR: Failed to get display mode: %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }
    printf("Display mode: %dx%d, refresh rate: %d\n", dm.w, dm.h, dm.refresh_rate);
    fflush(stdout);

    // Teste 4: Criar janela (modo janela primeiro)
    printf("\n4. Testing window creation (windowed mode)...\n");
    fflush(stdout);
    
    SDL_Window* win = SDL_CreateWindow("DropBlocks Test", 
                                      SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                      800, 600, SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
    if (!win) {
        printf("ERROR: Failed to create windowed window: %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }
    printf("SUCCESS: Window created successfully\n");
    fflush(stdout);

    // Teste 5: Criar renderer
    printf("\n5. Testing renderer creation...\n");
    fflush(stdout);
    
    SDL_Renderer* ren = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED);
    if (!ren) {
        printf("WARNING: Accelerated renderer failed: %s\n", SDL_GetError());
        printf("Trying software renderer...\n");
        fflush(stdout);
        
        ren = SDL_CreateRenderer(win, -1, SDL_RENDERER_SOFTWARE);
        if (!ren) {
            printf("ERROR: Software renderer also failed: %s\n", SDL_GetError());
            SDL_DestroyWindow(win);
            SDL_Quit();
            return 1;
        }
    }
    printf("SUCCESS: Renderer created successfully\n");
    fflush(stdout);

    // Teste 6: Verificar informações do renderer
    printf("\n6. Testing renderer info...\n");
    fflush(stdout);
    
    SDL_RendererInfo info;
    if (SDL_GetRendererInfo(ren, &info) == 0) {
        printf("Renderer: %s\n", info.name);
        printf("Hardware accelerated: %s\n", (info.flags & SDL_RENDERER_ACCELERATED) ? "Yes" : "No");
    }
    fflush(stdout);

    // Teste 7: Renderizar algo simples
    printf("\n7. Testing basic rendering...\n");
    fflush(stdout);
    
    SDL_SetRenderDrawColor(ren, 0, 0, 0, 255);
    SDL_RenderClear(ren);
    
    SDL_SetRenderDrawColor(ren, 255, 0, 0, 255);
    SDL_Rect rect = {100, 100, 200, 200};
    SDL_RenderFillRect(ren, &rect);
    
    SDL_RenderPresent(ren);
    printf("SUCCESS: Basic rendering completed\n");
    fflush(stdout);

    // Teste 8: Aguardar um pouco
    printf("\n8. Waiting 2 seconds...\n");
    fflush(stdout);
    SDL_Delay(2000);

    // Cleanup
    printf("\n9. Cleaning up...\n");
    fflush(stdout);
    
    SDL_DestroyRenderer(ren);
    SDL_DestroyWindow(win);
    SDL_Quit();
    
    printf("\n=== ALL TESTS PASSED ===\n");
    printf("SDL2 initialization is working correctly!\n");
    fflush(stdout);
    
    return 0;
}

