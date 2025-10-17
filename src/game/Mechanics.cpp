#include "game/Mechanics.hpp"
#include "pieces/Piece.hpp"
#include <vector>
#include "audio/AudioSystem.hpp"

bool collides(const Active& a, const std::vector<std::vector<Cell>>& g, int dx, int dy, int drot){
    int R = (a.rot + drot + 4)%4;
    extern std::vector<Piece> PIECES;
    for (const auto& p : PIECES[a.idx].rot[R]){
        int x = a.x + dx + p.first;
        int y = a.y + dy + p.second;
        if (y < 0) continue;
        if (x<0 || x>=COLS || y>=ROWS) return true;
        if (g[y][x].occ) return true;
    }
    return false;
}

void lockPiece(const Active& a, std::vector<std::vector<Cell>>& g){
    extern std::vector<Piece> PIECES;
    const auto &pc = PIECES[a.idx];
    for (const auto& p : pc.rot[a.rot]){
        int x=a.x+p.first;
        int y=a.y+p.second;
        if (y>=0 && y<ROWS && x>=0 && x<COLS){ g[y][x].occ=true; g[y][x].r=pc.r; g[y][x].g=pc.g; g[y][x].b=pc.b; }
    }
}


void rotateWithKicks(Active& act, const std::vector<std::vector<Cell>>& grid, int dir, AudioSystem& audio){
    extern std::vector<Piece> PIECES; const auto& p = PIECES[act.idx]; int from = act.rot; int to = (act.rot + (dir>0?1:3)) % 4;
    if(p.hasPerTransKicks){ int dirIdx = (dir>0?0:1); const auto& lst = p.kicksPerTrans[dirIdx][from]; for(auto [kx,ky] : lst){ if(!collides(act, grid, kx, ky, dir)){ act.x+=kx; act.y+=ky; act.rot=to; if(kx||ky) audio.playKickSound(); return; } } }
    if(p.hasKicks){ const auto& lst = (dir>0? p.kicksCW : p.kicksCCW); for(auto [kx,ky] : lst){ if(!collides(act, grid, kx, ky, dir)){ act.x+=kx; act.y+=ky; act.rot=to; if(kx||ky) audio.playKickSound(); return; } } }
    { int minX=999, maxX=-999; for (auto [px,py] : p.rot[to]) { int x = act.x + px; if (x < minX) minX = x; if (x > maxX) maxX = x; } int dx=0; if (minX < 0) dx = -minX; else if (maxX >= COLS) dx = (COLS - 1) - maxX; if (dx != 0) { const std::pair<int,int> boundTests[] = {{dx,0},{dx,-1}}; for (auto [kx,ky] : boundTests) { if (!collides(act, grid, kx, ky, dir)) { act.x+=kx; act.y+=ky; act.rot=to; if(kx||ky) audio.playKickSound(); return; } } } }
    { const std::pair<int,int> tests[] = {{0,0},{-1,0},{1,0},{0,-1},{-1,-1},{1,-1},{0,-2},{-2,0},{2,0},{0,1}}; for (auto [kx,ky] : tests) { if (!collides(act, grid, kx, ky, dir)) { act.x+=kx; act.y+=ky; act.rot=to; if(kx||ky) audio.playKickSound(); return; } } }
    if(!collides(act, grid, 0, 0, dir)) { act.rot=to; return; } int sx=(dir>0?1:-1); if(!collides(act, grid, sx, 0, dir)) { act.x+=sx; act.rot=to; return; } if(!collides(act, grid, 0,-1, dir)) { act.y-=1; act.rot=to; return; }
}
