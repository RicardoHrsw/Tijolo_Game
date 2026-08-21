#include <stdio.h>
#include <stdlib.h>
#include <string.h>  // memcpy, memset: copiar e limpar linhas do tabuleiro
#include <time.h>    // time: semente do gerador aleatorio
#include <curses.h>  // ncurses: entrada de teclado, desenho e temporizacao

#define LARGURA 10
#define ALTURA 20

#define NUM_PECAS 7
#define TAM 4  // lado da matriz que guarda cada peca

#define PAR_DA_PECA(i) ((i) + 1)

#define MIN_LINHAS  (ALTURA + 4)
#define MIN_COLUNAS 56  

typedef struct {
    int forma[TAM][TAM];
    int lado;
    int cor;       
    short rgb[3];  
} Peca;

const Peca PECAS[NUM_PECAS] = {
    { {{0,0,0,0},
       {1,1,1,1},
       {0,0,0,0},
       {0,0,0,0}}, 4, COLOR_CYAN,    {   0, 950,1000} },  // I - ciano
    { {{1,1,0,0},
       {1,1,0,0},
       {0,0,0,0},
       {0,0,0,0}}, 2, COLOR_YELLOW,  {1000, 850,   0} },  // O - amarelo
    { {{0,1,0,0},
       {1,1,1,0},
       {0,0,0,0},
       {0,0,0,0}}, 3, COLOR_MAGENTA, { 812, 624,1000} },  // T - lilas (azul escuro se eu usar o powershell...)
    { {{0,1,1,0},
       {1,1,0,0},
       {0,0,0,0},
       {0,0,0,0}}, 3, COLOR_GREEN,   {  50, 950, 200} },  // S - verde
    { {{1,1,0,0},
       {0,1,1,0},
       {0,0,0,0},
       {0,0,0,0}}, 3, COLOR_RED,     {1000, 150, 150} },  // Z - vermelho
    { {{1,0,0,0},
       {1,1,1,0},
       {0,0,0,0},
       {0,0,0,0}}, 3, COLOR_BLUE,    { 250, 500,1000} },  // J - azul
    { {{0,0,1,0},
       {1,1,1,0},
       {0,0,0,0},
       {0,0,0,0}}, 3, COLOR_WHITE,   {1000, 550,   0} },  // L - laranja
};

// Pontuacao classica do Tetris: quantas mais linhas de uma so vez elas mais valem
const int PONTOS_POR_LINHAS[] = {0, 40, 100, 300, 1200};

// 0 = celula vazia; qualquer outro valor e o par de cor da peca que ali travou.
int tabuleiro[ALTURA][LARGURA] = {0};

int peca_atual;       // indice em PECAS
int forma[TAM][TAM];  // forma da peca a cair, ja com a rotacao aplicada
int px;               // coluna do canto superior esquerdo da matriz da peca
int py;               // linha do canto superior esquerdo da matriz da peca
int jogo_rodando = 1;
int tem_cor = 0;
int pontos = 0;
int linhas_feitas = 0;
int contador_frames = 0;  // frames desde a ultima descida por gravidade

void iniciarCores() {
    if (!has_colors()) return;

    start_color();

    int intensas = COLORS >= 16;

    int propria = intensas && can_change_color();

    for (int i = 0; i < NUM_PECAS; i++) {
        int cor = PECAS[i].cor + (intensas ? 8 : 0);

        if (propria) {
            init_color(cor, PECAS[i].rgb[0], PECAS[i].rgb[1], PECAS[i].rgb[2]);
        }

        if (init_pair(PAR_DA_PECA(i), COLOR_BLACK, cor) == ERR) return;
    }

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

// Verifica se a forma dada cabe na posicao (nx, ny) sem sair do tabuleiro
// nem bater em pecas ja travadas.
int podeColocar(const int f[TAM][TAM], int nx, int ny) {
    for (int y = 0; y < TAM; y++) {
        for (int x = 0; x < TAM; x++) {
            if (!f[y][x]) continue;  // quadrado vazio da matriz, nao conta

            int tx = nx + x;
            int ty = ny + y;

            if (tx < 0 || tx >= LARGURA) return 0;  // paredes
            if (ty >= ALTURA) return 0;             // chao
            if (tabuleiro[ty][tx]) return 0;        // peca travada
        }
    }
    return 1;
}

// Devolve o par de cor se a peca a cair ocupa a celula (y, x), 0 caso contrario.
int pecaEm(int y, int x) {
    int ly = y - py;
    int lx = x - px;

    if (ly < 0 || ly >= TAM || lx < 0 || lx >= TAM) return 0;

    return forma[ly][lx] ? PAR_DA_PECA(peca_atual) : 0;
}

void novaPeca() {
    peca_atual = rand() % NUM_PECAS;
    memcpy(forma, PECAS[peca_atual].forma, sizeof forma);

    px = (LARGURA - TAM) / 2;
    py = 0;

    if (!podeColocar(forma, px, py)) {
        jogo_rodando = 0;
        memset(forma, 0, sizeof forma);
    }
}

// Apaga as linhas que ficaram cheias e deixa cair tudo o que estava por cima.
// Devolve quantas linhas apagou.
int limparLinhas() {
    int limpas = 0;

    // Varre de baixo para cima, que e o sentido em que as linhas descem.
    for (int y = ALTURA - 1; y >= 0; y--) {
        int cheia = 1;
        for (int x = 0; x < LARGURA; x++) {
            if (!tabuleiro[y][x]) { cheia = 0; break; }
        }
        if (!cheia) continue;

        // Puxa cada linha de cima uma posicao para baixo e limpa o topo.
        for (int l = y; l > 0; l--) {
            memcpy(tabuleiro[l], tabuleiro[l - 1], sizeof tabuleiro[l]);
        }
        memset(tabuleiro[0], 0, sizeof tabuleiro[0]);

        limpas++;
        y++;  
    }

    return limpas;
}

void travarPeca() {
    for (int y = 0; y < TAM; y++) {
        for (int x = 0; x < TAM; x++) {
            if (forma[y][x]) tabuleiro[py + y][px + x] = PAR_DA_PECA(peca_atual);
        }
    }

    int limpas = limparLinhas();
    linhas_feitas += limpas;
    pontos += PONTOS_POR_LINHAS[limpas];

    novaPeca();
}

void rodarPeca() {
    int novo[TAM][TAM] = {0};
    int n = PECAS[peca_atual].lado;

    for (int y = 0; y < n; y++) {
        for (int x = 0; x < n; x++) {
            novo[y][x] = forma[n - 1 - x][y];
        }
    }

    // Encostada a uma parede a peca rodada nao cabe no sitio onde esta. Antes
    // de desistir da rotacao tenta-se afasta-la um ou dois quadrados para o
    // lado -- e o chamado wall kick.
    const int desvios[] = {0, -1, 1, -2, 2};
    for (size_t i = 0; i < sizeof desvios / sizeof desvios[0]; i++) {
        if (podeColocar(novo, px + desvios[i], py)) {
            memcpy(forma, novo, sizeof forma);
            px += desvios[i];
            return;
        }
    }
}

void desenharTela() {
    erase();  // Limpa o ecra

    printw("=== TIJOLO GAME ===   Pontos: %-6d Linhas: %d\n", pontos, linhas_feitas);
    for (int y = 0; y < ALTURA; y++) {
        printw("<!");
        for (int x = 0; x < LARGURA; x++) {
            int par = pecaEm(y, x);           // Peca a cair
            if (!par) par = tabuleiro[y][x];  // Peca travada

            if (par) desenharBloco(par);
            else     printw(" .");  // Fundo
        }
        printw("!>\n");
    }
    printw("  ====================\n");
    printw("  A:Esq  D:Dir  S:Baixo  W:Rodar  Q:Sair");

    refresh();
}

void processarEntrada() {
    int c;

    while ((c = getch()) != ERR) {
        switch (c) {
            case KEY_LEFT:  case 'a': case 'A':
                if (podeColocar(forma, px - 1, py)) px--;
                break;
            case KEY_RIGHT: case 'd': case 'D':
                if (podeColocar(forma, px + 1, py)) px++;
                break;
            case KEY_DOWN:  case 's': case 'S':
                if (podeColocar(forma, px, py + 1)) {
                    py++;
                    contador_frames = 0;
                }
                break;
            case KEY_UP:    case 'w': case 'W':
                rodarPeca();
                break;
            case 'q': case 'Q':
                jogo_rodando = 0;
                return;  
        }
    }
}

void atualizarLogica() {
    if (podeColocar(forma, px, py + 1)) py++;
    else travarPeca();
}

int main() {
    iniciarEcra();

    if (LINES < MIN_LINHAS || COLS < MIN_COLUNAS) {
        endwin();
        printf("Terminal pequeno demais: precisa de %dx%d, tem %dx%d.\n",
               MIN_COLUNAS, MIN_LINHAS, COLS, LINES);
        return 1;
    }

    srand((unsigned) time(NULL));
    novaPeca();

    int velocidade = 10;

    while (jogo_rodando) {
        processarEntrada();

        contador_frames++;
        if (contador_frames >= velocidade) {
            atualizarLogica();
            contador_frames = 0;
        }

        desenharTela();
        napms(50);
    }

    endwin();

    printf("\nGAME OVER! Fizeste %d pontos em %d linhas. Obrigado por jogar o Tijolo Game!\n",
           pontos, linhas_feitas);
    return 0;
}
