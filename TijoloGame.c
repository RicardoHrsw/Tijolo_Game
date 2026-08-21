#include <stdio.h>
#include <stdlib.h>
#include <curses.h>  // ncurses: entrada de teclado, desenho e temporização

#define LARGURA 10
#define ALTURA 20

#define COR_PECA 1  // Peça a cair
#define COR_FIXA 2  // Peças já travadas

#define MIN_LINHAS  (ALTURA + 4)
#define MIN_COLUNAS 32

int tabuleiro[ALTURA][LARGURA] = {0};

int px = LARGURA / 2;
int py = 0;
int jogo_rodando = 1;
int tem_cor = 0;  

void iniciarCores() {
    if (!has_colors()) return;  

    start_color();

    init_pair(COR_PECA, COLOR_BLACK, COLOR_BLUE);
    init_pair(COR_FIXA, COLOR_BLACK, COLOR_WHITE);

    tem_cor = 1;
}


void desenharBloco(int par) {
    if (!tem_cor) {
        printw("[]");
        return;
    }
    attron(COLOR_PAIR(par));
    printw("  ");  
    attroff(COLOR_PAIR(par));
}


void iniciarEcra() {
    initscr();             
    cbreak();               
    noecho();              
    curs_set(0);            
    keypad(stdscr, TRUE);   
    nodelay(stdscr, TRUE);  

    iniciarCores();
}

void desenharTela() {
    erase();  // Limpa o ecra 

    printw("=== TIJOLO GAME ===\n");
    for (int y = 0; y < ALTURA; y++) {
        printw("<!");
        for (int x = 0; x < LARGURA; x++) {
            if (x == px && y == py) {
                desenharBloco(COR_PECA); // Peça a cair
            } else if (tabuleiro[y][x]) {
                desenharBloco(COR_FIXA); // Peça travada
            } else {
                printw(" ."); // Fundo
            }
        }
        printw("!>\n");
    }
    printw("  ====================\n");
    printw("  A:Esq  D:Dir  S:Baixo  Q:Sair");

    refresh();  
}

void processarEntrada() {
    int c = getch();
    if (c == ERR) return;  

    switch (c) {
        case KEY_LEFT:  case 'a': case 'A':
            if (px > 0 && !tabuleiro[py][px - 1]) px--;
            break;
        case KEY_RIGHT: case 'd': case 'D':
            if (px < LARGURA - 1 && !tabuleiro[py][px + 1]) px++;
            break;
        case KEY_DOWN:  case 's': case 'S':
            if (py < ALTURA - 1 && !tabuleiro[py + 1][px]) py++;
            break;
        case 'q': case 'Q':
            jogo_rodando = 0;
            break;
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
    iniciarEcra();

    if (LINES < MIN_LINHAS || COLS < MIN_COLUNAS) {
        endwin();
        printf("Terminal pequeno demais: precisa de %dx%d, tem %dx%d.\n",
               MIN_COLUNAS, MIN_LINHAS, COLS, LINES);
        return 1;
    }

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

        napms(50);
    }

    endwin(); 

    printf("\nGAME OVER! Obrigado por jogar o Tijolo Game!\n");
    return 0;
}
