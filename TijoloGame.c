#include <stdio.h>
#include <stdlib.h>
#include <conio.h>   // Para _kbhit() e _getch() no Windows
#include <windows.h> // Para Sleep() e manipulação da consola
#define LARGURA 10
#define ALTURA 20

int tabuleiro[ALTURA][LARGURA] = {0};

int px = LARGURA / 2;
int py = 0;
int jogo_rodando = 1;

void esconderCursor() {
    HANDLE consoleHandle = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_CURSOR_INFO info;
    info.dwSize = 100;
    info.bVisible = FALSE;
    SetConsoleCursorInfo(consoleHandle, &info);
}

void moverCursorParaTopo() {
    HANDLE consoleHandle = GetStdHandle(STD_OUTPUT_HANDLE);
    COORD pos = {0, 0};
    SetConsoleCursorPosition(consoleHandle, pos);
}

void desenharTela() {
    moverCursorParaTopo(); 
    
    printf("=== TIJOLO GAME ===\n");
    for (int y = 0; y < ALTURA; y++) {
        printf("<!");
        for (int x = 0; x < LARGURA; x++) {
            if (x == px && y == py) {
                printf("[]"); // Peça a cair
            } else if (tabuleiro[y][x]) {
                printf("[]"); // Peça travada
            } else {
                printf(" ."); // Fundo
            }
        }
        printf("!>\n");
    }
    printf("  ====================\n");
    printf("  A:Esq  D:Dir  S:Baixo  Q:Sair\n");
}

void processarEntrada() {
    
    if (_kbhit()) {
        char c = _getch(); 
        switch (c) {
            case 'a': case 'A': 
                if (px > 0 && !tabuleiro[py][px - 1]) px--; 
                break;
            case 'd': case 'D': 
                if (px < LARGURA - 1 && !tabuleiro[py][px + 1]) px++; 
                break;
            case 's': case 'S': 
                if (py < ALTURA - 1 && !tabuleiro[py + 1][px]) py++; 
                break;
            case 'q': case 'Q': 
                jogo_rodando = 0; 
                break;
        }
    }
}

void atualizarLogica() {
    if (py >= ALTURA - 1 || tabuleiro[py + 1][px] == 1) {
        tabuleiro[py][px] = 1; 
        
        px = LARGURA / 2;
        py = 0;
        
        if (tabuleiro[py][px] == 1) {
            jogo_rodando = 0;
        }
    } else {
        py++; 
    }
}

int main() {
    esconderCursor();
    system("cls"); 

    int contador_frames = 0;
    int velocidade = 10; 

    while (jogo_rodando) {
        desenharTela();
        processarEntrada();
        
        contador_frames++;
        if (contador_frames >= velocidade) {
            atualizarLogica();
            contador_frames = 0;
        }
        
        Sleep(50);
    }

    printf("\nGAME OVER! Obrigado por jogar o Tijolo Game!\n");
    return 0;
}