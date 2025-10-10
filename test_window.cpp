#include <SDL2/SDL.h>
#include <stdio.h>

int main() {
    printf("=== WINDOW CREATION TEST ===\n");
    printf("Testing SDL2 window creation on Raspberry Pi\n");
    fflush(stdout);
    
    // Inicializar SDL2
    printf("DEBUG: Initializing SDL2...\n");
    fflush(stdout);
    
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_EVENTS | SDL_INIT_AUDIO | SDL_INIT_GAMECONTROLLER) != 0) {
        printf("ERROR: SDL_Init failed: %s\n", SDL_GetError());
        return 1;
    }
    
    printf("DEBUG: SDL2 initialized successfully\n");
    fflush(stdout);
    
    // Obter modo de display
    printf("DEBUG: Getting display mode...\n");
    fflush(stdout);
    
    SDL_DisplayMode dm; 
    SDL_GetCurrentDisplayMode(0, &dm);
    int SW = dm.w, SH = dm.h;
    
    printf("DEBUG: Display mode: %dx%d\n", SW, SH);
    fflush(stdout);
    
    // Criar janela
    printf("DEBUG: Creating window...\n");
    fflush(stdout);
    
    SDL_Window* win = SDL_CreateWindow("DropBlocks", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, SW, SH,
                                       SDL_WINDOW_FULLSCREEN | SDL_WINDOW_ALLOW_HIGHDPI);
    if (!win) { 
        printf("ERROR: Window creation failed: %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }
    
    printf("DEBUG: Window created successfully\n");
    fflush(stdout);
    
    // Criar renderer
    printf("DEBUG: Creating renderer...\n");
    fflush(stdout);
    
    SDL_Renderer* ren = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!ren) { 
        printf("ERROR: Renderer creation failed: %s\n", SDL_GetError());
        SDL_DestroyWindow(win);
        SDL_Quit();
        return 1;
    }
    
    printf("DEBUG: Renderer created successfully\n");
    fflush(stdout);
    
    // Teste básico de renderização
    printf("DEBUG: Testing basic rendering...\n");
    fflush(stdout);
    
    SDL_SetRenderDrawColor(ren, 0, 0, 0, 255);
    SDL_RenderClear(ren);
    SDL_RenderPresent(ren);
    
    printf("DEBUG: Basic rendering test completed\n");
    fflush(stdout);
    
    // Aguardar um pouco
    printf("DEBUG: Waiting 2 seconds...\n");
    fflush(stdout);
    SDL_Delay(2000);
    
    // Limpeza
    printf("DEBUG: Cleaning up...\n");
    fflush(stdout);
    
    SDL_DestroyRenderer(ren);
    SDL_DestroyWindow(win);
    SDL_Quit();
    
    printf("DEBUG: Test completed successfully!\n");
    fflush(stdout);
    
    return 0;
}
