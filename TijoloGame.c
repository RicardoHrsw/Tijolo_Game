#include <stdio.h>
#include <stdlib.h>
#include <string.h>  // memcpy, memset: copiar e limpar linhas do tabuleiro
#include <time.h>    // time: semente do gerador aleatorio
#include <curses.h>  // ncurses: entrada de teclado, desenho e temporizacao

#define LARGURA 10
#define ALTURA 20

#define NUM_PECAS 7
#define TAM 4  // lado da matriz que guarda cada peca

// Os pares 1..NUM_PECAS sao as pecas; o seguinte fica para a sombra.
#define PAR_DA_PECA(i) ((i) + 1)
#define PAR_SOMBRA     (NUM_PECAS + 1)

// Cinzento da sombra: o indice 8 e o "preto intenso" da paleta de 16 cores,
// que no cmd.exe sai cinzento. Nao e usado por nenhuma peca (essas ficam nos
// indices 9 a 15), por isso pode ser mudado sem estragar as cores das pecas.
#define COR_SOMBRA 8

// Coluna onde comeca o painel lateral: as duas paredes mais as celulas do
// tabuleiro (cada uma ocupa dois caracteres) e ainda tres espacos de folga.
#define PAINEL_X (2 + LARGURA * 2 + 2 + 3)

#define MIN_LINHAS  (ALTURA + 4)
#define MIN_COLUNAS 56

// Ritmo de queda medido em frames de 50 ms: 10 frames = meio segundo por casa.
// A cada nivel desce um frame, ate ao limite de VELOCIDADE_MINIMA.
#define LINHAS_POR_NIVEL   10
#define VELOCIDADE_INICIAL 10
#define VELOCIDADE_MINIMA   2

#define FICHEIRO_RECORDE "recorde.txt"

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
int proxima_peca;     // indice da peca que entra a seguir, mostrada no painel
int forma[TAM][TAM];  // forma da peca a cair, ja com a rotacao aplicada
int px;               // coluna do canto superior esquerdo da matriz da peca
int py;               // linha do canto superior esquerdo da matriz da peca
int jogo_rodando = 1;
int tem_cor = 0;
int tem_sombra = 0;   // se ha cinzento disponivel para desenhar a sombra a cheio
int pontos = 0;
int linhas_feitas = 0;
int nivel = 1;
int velocidade = VELOCIDADE_INICIAL;  // frames entre duas descidas por gravidade
int recorde = 0;                      // melhor pontuacao de sempre, lida do ficheiro
int contador_frames = 0;              // frames desde a ultima descida por gravidade

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

    // Num terminal de 8 cores nao ha cinzento nenhum -- o unico tom entre o
    // preto e o branco seria o proprio fundo. Nesse caso a sombra passa a ser
    // desenhada com dois caracteres em vez de um bloco cheio.
    if (!intensas) return;

    if (propria) init_color(COR_SOMBRA, 400, 400, 400);

    if (init_pair(PAR_SOMBRA, COLOR_BLACK, COR_SOMBRA) != ERR) tem_sombra = 1;
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


// A sombra marca onde a peca vai aterrar. Sem cinzento disponivel desenha-se
// com dois pontos, que nao se confundem nem com o fundo (" .") nem com as
// pecas em texto ("[]") de um terminal sem cores.
void desenharSombra() {
    if (!tem_sombra) {
        printw("::");
        return;
    }
    attron(COLOR_PAIR(PAR_SOMBRA));
    printw("  ");
    attroff(COLOR_PAIR(PAR_SOMBRA));
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

// --- Melhor pontuacao -------------------------------------------------------
// Fica guardada num ficheiro de texto ao lado do executavel. Se o ficheiro nao
// existir (primeira partida) ou tiver lixo la dentro, comeca-se do zero em vez
// de dar erro: um recorde perdido nao e razao para o jogo nao arrancar.
int lerRecorde() {
    FILE *f = fopen(FICHEIRO_RECORDE, "r");
    if (!f) return 0;

    int valor = 0;
    if (fscanf(f, "%d", &valor) != 1 || valor < 0) valor = 0;

    fclose(f);
    return valor;
}

void gravarRecorde(int valor) {
    FILE *f = fopen(FICHEIRO_RECORDE, "w");
    if (!f) return;

    fprintf(f, "%d\n", valor);
    fclose(f);
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

// Linha onde a peca ficaria se caisse ate ao fim a partir de onde esta.
// Serve para dois sitios: desenhar a sombra e fazer a queda instantanea.
// O limite da ALTURA e uma salvaguarda: com a peca vazia -- o que acontece
// depois do fim do jogo -- nada trava a queda e o ciclo nunca mais parava.
int linhaDeAterragem() {
    int y = py;

    while (y < ALTURA && podeColocar(forma, px, y + 1)) y++;

    return y;
}

// Diz se a sombra ocupa a celula (y, x). Nao devolve cor porque a sombra tem
// sempre o mesmo aspeto, seja qual for a peca. A linha de aterragem vem de
// fora e nao e calculada aqui, senao era refeita para cada uma das 200
// celulas do tabuleiro em vez de uma vez por frame.
int sombraEm(int sombra_y, int y, int x) {
    int ly = y - sombra_y;
    int lx = x - px;

    if (ly < 0 || ly >= TAM || lx < 0 || lx >= TAM) return 0;

    return forma[ly][lx];
}

int sortearPeca() {
    return rand() % NUM_PECAS;
}

void novaPeca() {
    // A peca que entra e a que ja estava anunciada no painel; so depois se
    // sorteia a seguinte, senao o painel mostrava a peca que esta a cair.
    peca_atual = proxima_peca;
    proxima_peca = sortearPeca();

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

// A cada LINHAS_POR_NIVEL linhas sobe-se um nivel e a peca passa a cair um
// frame mais depressa. O limite existe para o jogo nao chegar a um ponto em
// que a peca desce mais depressa do que da para reagir.
void atualizarNivel() {
    nivel = linhas_feitas / LINHAS_POR_NIVEL + 1;

    velocidade = VELOCIDADE_INICIAL - (nivel - 1);
    if (velocidade < VELOCIDADE_MINIMA) velocidade = VELOCIDADE_MINIMA;
}

void travarPeca() {
    for (int y = 0; y < TAM; y++) {
        for (int x = 0; x < TAM; x++) {
            if (forma[y][x]) tabuleiro[py + y][px + x] = PAR_DA_PECA(peca_atual);
        }
    }

    int limpas = limparLinhas();
    linhas_feitas += limpas;

    // A mesma jogada vale mais em niveis altos, como no Tetris original.
    pontos += PONTOS_POR_LINHAS[limpas] * nivel;

    if (pontos > recorde) recorde = pontos;

    atualizarNivel();

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

// Painel a direita do tabuleiro: a peca que entra a seguir e os contadores.
// Aqui usa-se mvprintw (posicao absoluta) e nao printw, porque o tabuleiro e
// desenhado linha a linha e deixa o cursor sempre na margem esquerda.
void desenharPainel() {
    const int topo_peca = 3;

    mvprintw(topo_peca - 1, PAINEL_X, "SEGUINTE");

    // A forma esta encostada ao canto da matriz 4x4, por isso centra-se a
    // caixa de rotacao da peca (2x2 no O, 3x3 na maioria, 4x4 no I).
    int lado = PECAS[proxima_peca].lado;
    int desvio = (TAM - lado) / 2;

    for (int y = 0; y < lado; y++) {
        for (int x = 0; x < lado; x++) {
            if (!PECAS[proxima_peca].forma[y][x]) continue;

            move(topo_peca + desvio + y, PAINEL_X + (desvio + x) * 2);
            desenharBloco(PAR_DA_PECA(proxima_peca));
        }
    }

    mvprintw(topo_peca + TAM + 2, PAINEL_X, "Pontos:  %d", pontos);
    mvprintw(topo_peca + TAM + 3, PAINEL_X, "Linhas:  %d", linhas_feitas);
    mvprintw(topo_peca + TAM + 4, PAINEL_X, "Nivel:   %d", nivel);
    mvprintw(topo_peca + TAM + 6, PAINEL_X, "Recorde: %d", recorde);
}

void desenharTela() {
    erase();  // Limpa o ecra

    int sombra_y = linhaDeAterragem();

    printw("=== TIJOLO GAME ===\n");
    for (int y = 0; y < ALTURA; y++) {
        printw("<!");
        for (int x = 0; x < LARGURA; x++) {
            int par = pecaEm(y, x);           // Peca a cair
            if (!par) par = tabuleiro[y][x];  // Peca travada

            // A peca vem primeiro: quando ela ja esta em cima da sombra (perto
            // do fundo) e a peca que se ve, nao a sombra por baixo.
            if (par)                          desenharBloco(par);
            else if (sombraEm(sombra_y, y, x)) desenharSombra();
            else                              printw(" .");  // Fundo
        }
        printw("!>\n");
    }
    printw("  ====================\n");
    printw("  A:Esq  D:Dir  S:Baixo  W:Rodar  Espaco:Cair  Q:Sair");

    desenharPainel();

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
            case ' ':
                // Queda instantanea: a peca vai direita ao sitio onde a sombra
                // esta e trava logo, sem esperar pelo frame da gravidade.
                py = linhaDeAterragem();
                travarPeca();
                contador_frames = 0;

                // Sem isto, as teclas que estiverem na fila atras do espaco
                // (basta te-lo carregado com forca) eram aplicadas a peca
                // seguinte, que acabava de entrar, e ela caia logo tambem.
                flushinp();
                return;
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

    // Guarda-se o valor lido a parte: o 'recorde' vai sendo atualizado durante
    // a partida para o painel o mostrar a subir, e no fim e preciso saber com
    // que numero a partida comecou para dizer se houve ou nao recorde novo.
    int recorde_anterior = lerRecorde();
    recorde = recorde_anterior;

    proxima_peca = sortearPeca();  // enche o painel antes de a primeira peca entrar
    novaPeca();

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

    if (pontos > recorde_anterior) gravarRecorde(pontos);

    printf("\nGAME OVER! Fizeste %d pontos em %d linhas, ate ao nivel %d.\n",
           pontos, linhas_feitas, nivel);

    if (pontos > recorde_anterior) {
        printf("Novo recorde! O anterior era de %d pontos.\n", recorde_anterior);
    } else {
        printf("O recorde continua nos %d pontos.\n", recorde_anterior);
    }

    printf("Obrigado por jogar o Tijolo Game!\n");
    return 0;
}
