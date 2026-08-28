#include <stdio.h>
#include <stdlib.h>
#include <string.h>  
#include <time.h>    
#include <curses.h>  // como o curses.h não esta no meu includePath eu uso o makefile para compilar isto facilmente

#define LARGURA 10
#define ALTURA 20

#define NUM_PECAS 7
#define TAM 4  

#define PAR_DA_PECA(i) ((i) + 1)
#define PAR_SOMBRA     (NUM_PECAS + 1)

#define COR_SOMBRA 8

#define PAINEL_X (2 + LARGURA * 2 + 2 + 3)

#define MIN_LINHAS  (ALTURA + 4)
#define MIN_COLUNAS 56

#define LINHAS_POR_NIVEL   10
#define VELOCIDADE_INICIAL 10
#define VELOCIDADE_MINIMA   2

#define ATRASO_TRAVAGEM 10

#define MAX_ADIAMENTOS 15

#define MODO_A    0
#define MODO_B    1
#define NUM_MODOS 2

#define OBJETIVO_B 25

#define NIVEL_MAX  9
#define ALTURA_MAX 9

#define FICHEIRO_RECORDE "recorde.txt"

#define FIM_SAIR      0
#define FIM_OUTRA_VEZ 1
#define FIM_MENU      2

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


const int PONTOS_POR_LINHAS[] = {0, 40, 100, 300, 1200};

int tabuleiro[ALTURA][LARGURA] = {0};

int peca_atual;       
int proxima_peca;     
int forma[TAM][TAM];  
int px;               
int py;               
int jogo_rodando = 1;
int tem_cor = 0;
int tem_sombra = 0;   
int pontos = 0;
int linhas_feitas = 0;
int nivel = 1;
int velocidade = VELOCIDADE_INICIAL;  
int contador_frames = 0;              

int frames_no_chao = 0;   
int adiamentos = 0;       


int peca_guardada = -1;
int ja_trocou = 0;        

int modo = MODO_A;
int nivel_inicial = 1;   
int altura_inicial = 0;  
int pausado = 0;
int vitoria = 0;        

int recordes[NUM_MODOS] = {0};
int recorde = 0;            
int recorde_anterior = 0;  

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

int lerTecla() {
    nodelay(stdscr, FALSE);
    int c = getch();
    nodelay(stdscr, TRUE);
    return c;
}

const char *nomeModo(int m) {
    return m == MODO_B ? "B-Type" : "A-Type";
}

void lerRecordes() {
    FILE *f = fopen(FICHEIRO_RECORDE, "r");
    if (!f) return;

    for (int m = 0; m < NUM_MODOS; m++) {
        int valor = 0;

        if (fscanf(f, "%d", &valor) != 1 || valor < 0) valor = 0;

        recordes[m] = valor;
    }

    fclose(f);
}

void gravarRecordes() {
    FILE *f = fopen(FICHEIRO_RECORDE, "w");
    if (!f) return;

    for (int m = 0; m < NUM_MODOS; m++) fprintf(f, "%d\n", recordes[m]);

    fclose(f);
}

int podeColocar(const int f[TAM][TAM], int nx, int ny) {
    for (int y = 0; y < TAM; y++) {
        for (int x = 0; x < TAM; x++) {
            if (!f[y][x]) continue; 
            int tx = nx + x;
            int ty = ny + y;

            if (tx < 0 || tx >= LARGURA) return 0;  
            if (ty >= ALTURA) return 0;            
            if (tabuleiro[ty][tx]) return 0;       
        }
    }
    return 1;
}

int pecaEm(int y, int x) {
    int ly = y - py;
    int lx = x - px;

    if (ly < 0 || ly >= TAM || lx < 0 || lx >= TAM) return 0;

    return forma[ly][lx] ? PAR_DA_PECA(peca_atual) : 0;
}

int linhaDeAterragem() {
    int y = py;

    while (y < ALTURA && podeColocar(forma, px, y + 1)) y++;

    return y;
}

int sombraEm(int sombra_y, int y, int x) {
    int ly = y - sombra_y;
    int lx = x - px;

    if (ly < 0 || ly >= TAM || lx < 0 || lx >= TAM) return 0;

    return forma[ly][lx];
}

int saco[NUM_PECAS];
int saco_tiradas = NUM_PECAS;  

void encherSaco() {
    for (int i = 0; i < NUM_PECAS; i++) saco[i] = i;

    for (int i = NUM_PECAS - 1; i > 0; i--) {
        int j = rand() % (i + 1);

        int troca = saco[i];
        saco[i] = saco[j];
        saco[j] = troca;
    }

    saco_tiradas = 0;
}

int sortearPeca() {
    if (saco_tiradas == NUM_PECAS) encherSaco();

    return saco[saco_tiradas++];
}

void entrarPeca(int indice) {
    peca_atual = indice;

    memcpy(forma, PECAS[peca_atual].forma, sizeof forma);

    px = (LARGURA - TAM) / 2;
    py = 0;

    frames_no_chao = 0;
    adiamentos = 0;

    if (!podeColocar(forma, px, py)) {
        jogo_rodando = 0;
        memset(forma, 0, sizeof forma);
    }
}

void novaPeca() {

    int entra = proxima_peca;
    proxima_peca = sortearPeca();

    ja_trocou = 0;  

    entrarPeca(entra);
}

void trocarGuardada() {
    if (ja_trocou) return;

    int guardar = peca_atual;

    if (peca_guardada < 0) {

        novaPeca();
    } else {
        entrarPeca(peca_guardada);
    }

    peca_guardada = guardar;
    ja_trocou = 1;
}

int limparLinhas() {
    int limpas = 0;

    for (int y = ALTURA - 1; y >= 0; y--) {
        int cheia = 1;
        for (int x = 0; x < LARGURA; x++) {
            if (!tabuleiro[y][x]) { cheia = 0; break; }
        }
        if (!cheia) continue;

        for (int l = y; l > 0; l--) {
            memcpy(tabuleiro[l], tabuleiro[l - 1], sizeof tabuleiro[l]);
        }
        memset(tabuleiro[0], 0, sizeof tabuleiro[0]);

        limpas++;
        y++;
    }

    return limpas;
}

void atualizarNivel() {
    nivel = nivel_inicial + linhas_feitas / LINHAS_POR_NIVEL;

    velocidade = VELOCIDADE_INICIAL - (nivel - 1);
    if (velocidade < VELOCIDADE_MINIMA) velocidade = VELOCIDADE_MINIMA;
}

int linhasQueFaltam() {
    int faltam = OBJETIVO_B - linhas_feitas;
    return faltam > 0 ? faltam : 0;
}

void encherTabuleiro(int filas) {
    for (int y = ALTURA - filas; y < ALTURA; y++) {
        int vazias = 0;

        for (int x = 0; x < LARGURA; x++) {
            if (rand() % 2) {

                tabuleiro[y][x] = PAR_DA_PECA(rand() % NUM_PECAS);
            } else {
                tabuleiro[y][x] = 0;
                vazias++;
            }
        }

        if (!vazias) tabuleiro[y][rand() % LARGURA] = 0;
    }
}

void travarPeca() {
    for (int y = 0; y < TAM; y++) {
        for (int x = 0; x < TAM; x++) {
            if (forma[y][x]) tabuleiro[py + y][px + x] = PAR_DA_PECA(peca_atual);
        }
    }

    int limpas = limparLinhas();
    linhas_feitas += limpas;

    pontos += PONTOS_POR_LINHAS[limpas] * nivel;

    if (pontos > recorde) recorde = pontos;

    atualizarNivel();

    if (modo == MODO_B && linhas_feitas >= OBJETIVO_B) {
        vitoria = 1;
        jogo_rodando = 0;
        memset(forma, 0, sizeof forma);
        return;
    }

    novaPeca();
}

int rodarPeca(int sentido) {
    int novo[TAM][TAM] = {0};
    int n = PECAS[peca_atual].lado;

    for (int y = 0; y < n; y++) {
        for (int x = 0; x < n; x++) {
            if (sentido > 0) novo[y][x] = forma[n - 1 - x][y];
            else             novo[y][x] = forma[x][n - 1 - y];
        }
    }

    const int desvios[] = {0, -1, 1, -2, 2};
    for (size_t i = 0; i < sizeof desvios / sizeof desvios[0]; i++) {
        if (podeColocar(novo, px + desvios[i], py)) {
            memcpy(forma, novo, sizeof forma);
            px += desvios[i];
            return 1;
        }
    }

    return 0;
}

int estaAssente() {
    return !podeColocar(forma, px, py + 1);
}

void adiarTravagem() {
    if (!estaAssente()) return;             
    if (adiamentos >= MAX_ADIAMENTOS) return;

    frames_no_chao = 0;
    adiamentos++;
}

void desenharPecaNoPainel(int topo, int indice) {
    if (indice < 0) return;

    int lado = PECAS[indice].lado;
    int desvio = (TAM - lado) / 2;

    for (int y = 0; y < lado; y++) {
        for (int x = 0; x < lado; x++) {
            if (!PECAS[indice].forma[y][x]) continue;

            move(topo + desvio + y, PAINEL_X + (desvio + x) * 2);
            desenharBloco(PAR_DA_PECA(indice));
        }
    }
}

void desenharPainel() {
    const int topo_seguinte = 2;
    const int topo_guardada = topo_seguinte + TAM + 2;

    mvprintw(topo_seguinte - 1, PAINEL_X, "SEGUINTE");
    desenharPecaNoPainel(topo_seguinte, proxima_peca);

    mvprintw(topo_guardada - 1, PAINEL_X, "GUARDADA");
    desenharPecaNoPainel(topo_guardada, peca_guardada);

    int linha = topo_guardada + TAM + 1;

    mvprintw(linha++, PAINEL_X, "Modo:    %s", nomeModo(modo));
    linha++;
    mvprintw(linha++, PAINEL_X, "Pontos:  %d", pontos);
    mvprintw(linha++, PAINEL_X, "Linhas:  %d", linhas_feitas);
    if (modo == MODO_B) mvprintw(linha++, PAINEL_X, "Faltam:  %d", linhasQueFaltam());
    mvprintw(linha++, PAINEL_X, "Nivel:   %d", nivel);
    linha++;
    mvprintw(linha++, PAINEL_X, "Recorde: %d", recorde);
}

void desenharPausa() {
    const char *textos[] = { "PAUSA", "P: voltar", "Q: sair" };

    const int largura = LARGURA * 2;  


    int topo = 1 + ALTURA / 2 - 1;

    for (size_t i = 0; i < sizeof textos / sizeof textos[0]; i++) {
        char faixa[LARGURA * 2 + 1];
        memset(faixa, ' ', largura);
        faixa[largura] = '\0';

        int tamanho = (int) strlen(textos[i]);
        memcpy(faixa + (largura - tamanho) / 2, textos[i], tamanho);

        mvprintw(topo + (int) i, 2, "%s", faixa);
    }
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

            if (par)                          desenharBloco(par);
            else if (sombraEm(sombra_y, y, x)) desenharSombra();
            else                              printw(" .");  // Fundo
        }
        printw("!>\n");
    }
    printw("  ====================\n");
    printw("  Setas:Mover  Z/X:Rodar  C:Guardar\n");
    printw("  Espaco:Cair  P:Pausa  Q:Sair");

    desenharPainel();

    if (pausado) desenharPausa();

    refresh();
}

void processarEntrada() {
    int c;

    while ((c = getch()) != ERR) {

        if (pausado) {
            if (c == 'p' || c == 'P') {
                pausado = 0;
                flushinp();
                return;
            }
            if (c == 'q' || c == 'Q') {
                jogo_rodando = 0;
                return;
            }
            continue;
        }

        switch (c) {

            case KEY_LEFT:
                if (podeColocar(forma, px - 1, py)) { px--; adiarTravagem(); }
                break;
            case KEY_RIGHT:
                if (podeColocar(forma, px + 1, py)) { px++; adiarTravagem(); }
                break;
            case KEY_DOWN:
                if (podeColocar(forma, px, py + 1)) {
                    py++;
                    contador_frames = 0;
                }
                break;
            case 'z': case 'Z':
                if (rodarPeca(-1)) adiarTravagem();
                break;
            case 'x': case 'X':
                if (rodarPeca(1)) adiarTravagem();
                break;
            case 'c': case 'C':
                trocarGuardada();
                break;
            case ' ':
                py = linhaDeAterragem();
                travarPeca();
                contador_frames = 0;

                flushinp();
                return;
            case 'p': case 'P':
                pausado = 1;
                flushinp();
                return;
            case 'q': case 'Q':
                jogo_rodando = 0;
                return;
        }
    }
}


void atualizarLogica() {
    if (!estaAssente()) {
        frames_no_chao = 0;

        contador_frames++;
        if (contador_frames >= velocidade) {
            py++;
            contador_frames = 0;
        }
        return;
    }

    contador_frames = 0;
    frames_no_chao++;

    if (frames_no_chao >= ATRASO_TRAVAGEM) travarPeca();
}

//menus

int escolherNivel() {
    for (;;) {
        erase();

        int y = 2;
        mvprintw(y++, 2, "=== B-TYPE ===");
        y++;
        mvprintw(y++, 2, "Limpar %d linhas num tabuleiro ja com blocos.", OBJETIVO_B);
        y++;
        mvprintw(y++, 2, "Nivel de velocidade, de 1 a %d:", NIVEL_MAX);
        y++;
        mvprintw(y++, 4, "1  mais devagar");
        mvprintw(y++, 4, "%d  mais depressa", NIVEL_MAX);
        y++;
        mvprintw(y++, 4, "Q  voltar ao menu");

        refresh();

        int c = lerTecla();

        if (c == 'q' || c == 'Q') return 0;
        if (c >= '1' && c <= '0' + NIVEL_MAX) return c - '0';
    }
}

int escolherAltura(int nivel_escolhido) {
    for (;;) {
        erase();

        int y = 2;
        mvprintw(y++, 2, "=== B-TYPE ===");
        y++;
        mvprintw(y++, 2, "Nivel de velocidade: %d", nivel_escolhido);
        y++;
        mvprintw(y++, 2, "Altura da pilha inicial, de 0 a %d:", ALTURA_MAX);
        y++;
        mvprintw(y++, 4, "0  tabuleiro vazio");
        mvprintw(y++, 4, "%d  %d filas de blocos", ALTURA_MAX, ALTURA_MAX);
        y++;
        mvprintw(y++, 4, "Q  voltar atras");

        refresh();

        int c = lerTecla();

        if (c == 'q' || c == 'Q') return -1;
        if (c >= '0' && c <= '0' + ALTURA_MAX) return c - '0';
    }
}

int menuModos() {
    for (;;) {
        erase();

        int y = 2;
        mvprintw(y++, 2, "=== TIJOLO GAME ===");
        y++;
        mvprintw(y++, 2, "Escolhe o modo de jogo:");
        y++;
        mvprintw(y++, 4, "1  A-Type  -  sem fim, cada vez mais rapido");
        mvprintw(y++, 4, "2  B-Type  -  limpar %d linhas", OBJETIVO_B);
        y++;
        mvprintw(y++, 4, "Q  sair");
        y++;
        mvprintw(y++, 2, "Recordes:  A-Type %d   B-Type %d",
                 recordes[MODO_A], recordes[MODO_B]);

        refresh();

        int c = lerTecla();

        if (c == 'q' || c == 'Q') return 0;

        if (c == '1') {
            modo = MODO_A;
            nivel_inicial = 1;
            return 1;
        }

        if (c == '2') {
            int n = 0;
            int a = -1;

            while (a < 0) {
                n = escolherNivel();
                if (!n) break;

                a = escolherAltura(n);
            }

            if (!n) continue;

            modo = MODO_B;
            nivel_inicial = n;
            altura_inicial = a;
            return 1;
        }
    }
}

int ecraFinal() {
    erase();

    int y = 2;

    if (vitoria) mvprintw(y++, 2, "=== CONSEGUISTE! %d LINHAS ===", OBJETIVO_B);
    else         mvprintw(y++, 2, "=== GAME OVER ===");

    y++;
    mvprintw(y++, 2, "Modo:    %s", nomeModo(modo));
    if (modo == MODO_B) {
        mvprintw(y++, 2, "Inicio:  nivel %d, altura %d", nivel_inicial, altura_inicial);
    }
    mvprintw(y++, 2, "Pontos:  %d", pontos);
    mvprintw(y++, 2, "Linhas:  %d", linhas_feitas);
    mvprintw(y++, 2, "Nivel:   %d", nivel);

    y++;
    if (pontos > recorde_anterior) {
        mvprintw(y++, 2, "Novo recorde! O anterior era de %d pontos.", recorde_anterior);
    } else {
        mvprintw(y++, 2, "O recorde do %s continua nos %d pontos.",
                 nomeModo(modo), recorde_anterior);
    }

    y++;
    mvprintw(y++, 4, "R  jogar outra vez no %s", nomeModo(modo));
    mvprintw(y++, 4, "M  voltar ao menu dos modos");
    mvprintw(y++, 4, "Q  sair");

    refresh();

    for (;;) {
        int c = lerTecla();

        if (c == 'r' || c == 'R') return FIM_OUTRA_VEZ;
        if (c == 'm' || c == 'M') return FIM_MENU;
        if (c == 'q' || c == 'Q') return FIM_SAIR;
    }
}

void reiniciarJogo() {
    memset(tabuleiro, 0, sizeof tabuleiro);

    pontos = 0;
    linhas_feitas = 0;
    contador_frames = 0;
    frames_no_chao = 0;
    adiamentos = 0;
    peca_guardada = -1;
    ja_trocou = 0;
    pausado = 0;
    vitoria = 0;
    jogo_rodando = 1;

    atualizarNivel();  

    recorde_anterior = recordes[modo];
    recorde = recorde_anterior;

    if (modo == MODO_B) encherTabuleiro(altura_inicial);

    encherSaco();

    proxima_peca = sortearPeca();  
    novaPeca();
}

void cicloDeJogo() {
    while (jogo_rodando) {
        processarEntrada();

        if (!pausado) atualizarLogica();

        desenharTela();
        napms(50);
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

    srand((unsigned) time(NULL));

    lerRecordes();

    int no_menu = 1;

    while (1) {
        if (no_menu && !menuModos()) break;

        reiniciarJogo();
        cicloDeJogo();

        if (pontos > recorde_anterior) {
            recordes[modo] = pontos;
            gravarRecordes();
        }

        int escolha = ecraFinal();
        if (escolha == FIM_SAIR) break;

        no_menu = (escolha == FIM_MENU);
    }

    endwin();

    printf("Obrigado por jogar o Tijolo Game!\n");
    return 0;
}
