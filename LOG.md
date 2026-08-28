# Registo de Sessões - Tijolo Game

## Sessão 1 - 19/08/2026

**Funcionalidades implementadas hoje:**
- Criação da estrutura base do projeto e do loop principal do jogo.
- Renderização do tabuleiro do jogo (10x20) no terminal.
- Mecânica básica de física: bloco de tamanho 1x1 a cair do topo do ecra.
- Deteção de colisão simples com o chão e com blocos fixos na base.
- Leitura de comandos do teclado em tempo real (A para esquerda, D para direita, S para acelerar a queda, Q para sair).

**Maior dificuldade encontrada e como resolvi (ou não resolvi...):**
- A leitura contínua das teclas sem pausar o jogo e o problema da tela a piscar (*flicker*) durante as atualizações. Resolvido ajustando a posição do cursor do terminal no início de cada renderização em vez de limpar a tela inteira.

**Próximo passo planeado:**
- Substituir o bloco individual de 1x1 pelas 7 formas clássicas do Tetris (Tetrominós) através de matrizes 4x4 e implementar a rotação das peças.
- Adicionar opção de puder mudar o estilo de commandos.

## Sessão 2 - 20/08/2026

**Funcionalidades implementadas hoje:**
- Migração de todo o projeto para a biblioteca ncurses, eliminando as dependências de `conio.h` e `windows.h`. O código passou a ser C portável (compila em Linux apenas trocando a flag de ligação).
- Substituição das rotinas manuais da consola do Windows pelos equivalentes do curses: `esconderCursor()` (que usava `GetStdHandle`/`CONSOLE_CURSOR_INFO`) passou a `curs_set(0)`, `moverCursorParaTopo()` (`SetConsoleCursorPosition`) deixou de ser necessário, `system("cls")` passou a `erase()` e o `Sleep()` passou a `napms()`.
- Reescrita da leitura do teclado: o par `_kbhit()`/`_getch()` deu lugar a `getch()` em modo não-bloqueante (`nodelay`), que devolve `ERR` quando não há tecla premida no frame.
- Adição das setas do teclado como alternativa ao A/D/S, através do `keypad(stdscr, TRUE)`. Fica assim tratada parte do "estilo de comandos" que estava planeado.
- Verificação do tamanho do terminal no arranque, para o tabuleiro não sair cortado sem aviso quando a janela é pequena.
- Criação de um `Makefile` (alvos `run` e `clean`) que usa o `pkg-config` para localizar o ncurses automaticamente, e de um `.gitignore` para o executável.
- Ajuste do `Makefile` para funcionar tanto no PowerShell como no Git Bash: as chamadas ao `pkg-config` e o comando `rm` só existiam no bash, pelo que foram substituídos por `$(wildcard ...)` e pelo `rm.exe` do MSYS2 chamado por caminho absoluto.
- Introdução de cor: a peça que cai passou a azul e as peças já travadas a branco, usando o sistema de pares de cor do ncurses (`start_color()` e `init_pair()`).
- Substituição do `[]` por quadrados sólidos. Em vez de procurar um caractere com forma de quadrado, desenham-se **dois espaços com cor de fundo** — o resultado é um bloco cheio e não depende da fonte nem da codificação do terminal.
- Criação de uma função `desenharBloco()` que centraliza o desenho de uma célula ocupada e que volta ao `[]` em texto caso o terminal não suporte cores. Sem essa salvaguarda, um terminal monocromático mostraria o tabuleiro vazio, porque os blocos seriam espaços invisíveis.
- A lógica de jogo — colisões, velocidade de queda, layout do tabuleiro — ficou intacta.

**Maior dificuldade encontrada e como resolvi (ou não resolvi...):**
- Pôr o ncurses a compilar no Windows. No MSYS2/UCRT64 o `curses.h` não está na raiz dos includes mas sim em `include/ncursesw/`, pelo que um `#include <curses.h>` simples falha. Resolvido passando o caminho ao compilador com `-I`. No Makefile, para não ficar dependente do caminho da minha máquina, procuro as localizações conhecidas com a função `$(wildcard ...)` do próprio make — cheguei a usar o `pkg-config`, mas esse dependia da shell e falhava no PowerShell.
- Ficou por resolver a questão da distribuição: o executável depende das DLLs do UCRT64 e da base de dados terminfo, ou seja, não corre noutro computador sem o MSYS2 instalado. A alternativa é o PDCurses, que tem a mesma API mas fala diretamente com a consola do Windows e gera um executável autónomo. Como o código só usa chamadas curses padrão, essa troca é só mudar a flag de ligação se vier a ser preciso.
- Um bug silencioso nas cores, que foi o mais difícil de encontrar por não dar erro nenhum. Na primeira tentativa usei `init_pair(COR_PECA, COLOR_BLUE, -1)`, em que o `-1` significa "manter o fundo atual do terminal". Acontece que esse `-1` depende da função `use_default_colors()`, que neste build do ncurses devolve `ERR` — testei com cinco valores de `TERM` diferentes e falhou em todos. Como o `init_pair()` falhava, o par de cor nunca chegava a ser criado, e o `attron()` simplesmente não fazia nada: a peça saía sem cor e sem qualquer mensagem de erro. Resolvido definindo o fundo explicitamente em vez de contar com o `-1`. Ficou a lição de que no ncurses convém verificar o valor de retorno destas funções.
- O problema do *flicker* da Sessão 1 deixou de existir por si só: o ncurses mantém um ecrã virtual em memória e no `refresh()` envia ao terminal apenas as células que mudaram, pelo que o truque manual de reposicionar o cursor deixou de ser necessário.

**Próximo passo planeado:**
- Substituir o bloco individual de 1x1 pelas 7 formas clássicas do Tetris (Tetrominós) através de matrizes 4x4 e implementar a rotação das peças.
- Dar uma cor própria a cada um dos 7 tetrominós, reaproveitando o mecanismo de pares de cor já montado (basta um `init_pair()` por peça).
- Deteção de linhas completas e sistema de pontuação.

## Sessão 3 - 21/08/2026

**Funcionalidades implementadas hoje:**
- Substituição do bloco de 1x1 pelos 7 tetrominós clássicos, guardados em matrizes 4x4 numa tabela `PECAS`.
- Rotação das peças com o `W` ou a seta para cima. Cada peça roda dentro de uma caixa do seu próprio tamanho — 2x2 no O, 3x3 na maioria, 4x4 no I — e não dentro da matriz 4x4 inteira.
- *Wall kick*: encostada a uma parede, a peça tenta afastar-se um ou dois quadrados para o lado antes de a rotação ser recusada. Sem isto o I nunca rodava junto às bordas.
- Toda a deteção de colisões passou para uma única função, `podeColocar()`, usada pelo movimento lateral, pela gravidade e pela rotação. Antes a mesma verificação estava escrita em vários sítios.
- Uma cor por peça, com o tom definido em RGB através do `init_color()` e a cor básica do ncurses como alternativa para terminais mais limitados.
- Deteção de linhas completas e pontuação clássica do Tetris: 40, 100, 300 e 1200 pontos conforme se façam 1, 2, 3 ou 4 linhas de uma vez. Pontos e linhas passaram a aparecer no cabeçalho e na mensagem final.
- Correções ao ciclo principal: a fila do teclado passou a ser esvaziada a cada frame (lia-se uma tecla só, e as restantes acumulavam-se, fazendo a peça mexer-se sozinha depois de largar a tecla); a ordem passou a ser ler → gravidade → desenhar, para o movimento aparecer no próprio frame em vez do seguinte; e o `S` reinicia o contador da gravidade, senão a peça descia duas casas quase ao mesmo tempo.
- O `Makefile` deixou de estar no `.gitignore` e passa a ir para o repositório, como já era anunciado na mensagem de commit da sessão anterior. Corrigidos também dois blocos de código por fechar no guia de compilação, que estavam a esconder a secção do Linux.

**Maior dificuldade encontrada e como resolvi (ou não resolvi...):**
- A rotação. Rodar a matriz 4x4 inteira faz as peças de 3 quadrados de largura saltarem para o canto da matriz a cada rotação, em vez de rodarem no sítio. Resolvido guardando em cada peça o lado da caixa em que ela roda e aplicando a fórmula `novo[y][x] = antigo[lado-1-x][y]` só a essa caixa.
- As cores no PowerShell, que ficou por resolver. As peças saíam com tons escuros e o T era praticamente invisível. A causa não estava no código: o Windows PowerShell traz uma paleta de consola própria que substitui dois lugares da tabela de 16 cores — o índice 5 (magenta, a cor do T) passa a `#012456`, que é exatamente o azul do fundo da janela, e o índice 6 (amarelo, a cor do O) passa a quase branco. Confirmei isto lendo a paleta no registo do Windows, em `HKCU:\Console`. A saída seria pedir um terminal de 256 cores, para as peças usarem os índices 8 a 15, que o PowerShell não altera — mas a consola do Windows não digere bem as sequências desse terminal e o ecrã sai com lixo. Ficou decidido testar o jogo no `cmd.exe`, onde a paleta é a de fábrica e as cores saem certas.
- Um pormenor pequeno na limpeza de linhas que dava um bug difícil de ver: quando uma linha cheia é apagada, a linha que desce para o lugar dela tem de ser testada outra vez. Sem isso, duas linhas cheias seguidas só contavam como uma.

**Próximo passo planeado:**
- Níveis de velocidade: acelerar a queda à medida que as linhas vão sendo feitas. Neste momento a velocidade é fixa e o jogo nunca fica mais difícil.
- Mostrar a peça seguinte ao lado do tabuleiro.
- Guardar a melhor pontuação entre partidas.

## Sessão 4 - 24/08/2026

**Funcionalidades implementadas hoje:**
- Níveis de velocidade. A velocidade deixou de ser fixa: sobe-se um nível a cada 10 linhas feitas e a peça passa a cair um *frame* mais depressa por cada nível. Como o ciclo principal corre a 50 ms por frame, o nível 1 dá meio segundo por casa e o nível 9 dá um décimo. Há um limite mínimo de 2 frames, senão a partir de certa altura a peça descia mais depressa do que dá para reagir.
- A pontuação passou a ser multiplicada pelo nível, como no Tetris original — as mesmas 4 linhas de uma vez valem 1200 pontos no nível 1 e 6000 no nível 5.
- Painel lateral à direita do tabuleiro, com a peça seguinte e os contadores (pontos, linhas, nível e recorde). Os contadores saíram do cabeçalho, que estava a ficar comprido demais para a largura mínima do terminal.
- Peça seguinte: a peça a entrar deixou de ser sorteada no momento em que a anterior trava. Agora existe uma `proxima_peca` que é anunciada no painel; quando a peça atual trava, é essa que entra e sorteia-se outra para o lugar dela.
- Melhor pontuação guardada entre partidas, num ficheiro de texto `recorde.txt` ao lado do executável. É lido no arranque, mostrado no painel a subir durante a partida e gravado no fim se tiver sido batido. A mensagem final diz se houve recorde novo ou qual continua a ser o recorde a bater.
- O `recorde.txt` foi acrescentado ao `.gitignore`: é um ficheiro gerado e próprio de cada máquina, não faz sentido ir para o repositório.
- Sombra da peça (*ghost*): a cinzento, no fundo do tabuleiro, a mostrar onde a peça vai aterrar se ninguém lhe tocar. É desenhada com o índice 8 da paleta, o "preto intenso", que no cmd.exe sai cinzento e não é usado por nenhuma das peças (essas ficam nos índices 9 a 15). Num terminal de 8 cores, onde esse cinzento não existe, a sombra passa a ser desenhada com `::` em vez de um bloco cheio.
- Queda instantânea (*hard drop*) na barra de espaços: a peça vai direita ao sítio onde a sombra está e trava logo, sem esperar pelo frame da gravidade.

**Maior dificuldade encontrada e como resolvi (ou não resolvi...):**
- O desenho do painel. O tabuleiro é desenhado linha a linha com `printw` e um `\n` no fim, o que deixa o cursor sempre na margem esquerda da linha seguinte — não há forma de escrever à direita do tabuleiro desse modo. Resolvido desenhando o painel à parte com `mvprintw`, que recebe a linha e a coluna em absoluto, e chamando-o depois do tabuleiro estar desenhado. A coluna de início não ficou escrita à mão: é calculada a partir da `LARGURA` (duas paredes mais dez células de dois caracteres cada, mais três de folga), para o painel não ficar por cima do tabuleiro se um dia mudar o tamanho do tabuleiro.
- Verificar o aspeto do painel sem conseguir ver o ecrã. Como não dá para inspecionar o que o ncurses põe no terminal a não ser olhando para ele, escrevi um programa pequeno à parte que repete as mesmas contas de posição, mas escreve numa matriz de caracteres em vez de chamar o curses, e que avisa se dois textos calharem na mesma célula ou se algo sair fora das 56x24 mínimas. Confirmou-se que o painel não colide com o tabuleiro e que a peça seguinte fica centrada nas sete peças.
- Centrar a peça no painel. As formas estão encostadas ao canto da matriz 4x4, por isso desenhá-las tal e qual fazia o O aparecer colado ao canto superior esquerdo enquanto o I ocupava a largura toda. Resolvido reutilizando o campo `lado` que já existia para a rotação: desenha-se só a caixa de rotação da peça e desloca-se `(4 - lado) / 2` para o meio.
- A sombra e a queda instantânea acabaram por ser a mesma pergunta feita duas vezes: "até onde é que esta peça desce?". Ficou uma função só, a `linhaDeAterragem()`, usada pelo desenho da sombra e pelo espaço — a sombra mostra exatamente o sítio onde o espaço vai pôr a peça, e não há duas contas para andarem a divergir uma da outra.
- Um ciclo infinito que só acontecia depois do fim do jogo. A `linhaDeAterragem()` desce enquanto a peça couber, e quando o jogo acaba a peça é posta a zeros — uma peça sem quadrados nenhuns cabe em todo o lado, portanto o ciclo nunca mais parava e o jogo ficava pendurado no último frame. Resolvido pondo o fundo do tabuleiro como limite do ciclo, e não só a colisão.
- O espaço a largar duas peças de uma vez. Como a fila do teclado é toda esvaziada em cada frame (correção da Sessão 3), carregar no espaço com um bocadinho de força mete lá dois ou três, e os que sobram eram aplicados à peça seguinte, que acabava de entrar — largavam-se duas ou três peças num piscar de olhos. Resolvido com o `flushinp()`, que deita fora o resto da fila logo a seguir a uma queda instantânea.
- Verificar isto tudo sem conseguir ver o ecrã, outra vez. Desta vez, em vez do programa à parte que só repetia as contas, compilei o próprio `TijoloGame.c` contra um `curses.h` de mentira, que em vez de falar com o terminal escreve numa matriz de caracteres e devolve as teclas de uma lista preparada por mim. Assim consegui ver o tabuleiro impresso em texto e testar a sombra encostada ao chão, a sombra em cima de uma torre, o espaço a completar uma linha, os cinco espaços seguidos e o fim de jogo — sem mexer numa linha do jogo, que continua a incluir o `<curses.h>` normal.
- Um pormenor na ordem das coisas ao gravar o recorde. Ao início atualizava o `recorde` durante a partida (para o painel o mostrar a subir) e no fim comparava os pontos com esse mesmo `recorde` — que já tinha sido atualizado, portanto a comparação dava sempre falso e a mensagem de recorde novo nunca aparecia. Resolvido guardando à parte o valor lido do ficheiro no arranque, num `recorde_anterior` que não é tocado durante o jogo.
- Ficou por resolver a leitura de um `recorde.txt` alterado à mão: se o ficheiro tiver texto em vez de um número, ou um número negativo, o jogo assume zero em vez de dar erro. É de propósito — um recorde perdido não é razão para o jogo não arrancar — mas também quer dizer que não há nada a impedir alguém de escrever lá o número que quiser.

**Próximo passo planeado:**
- Pausa no jogo com a tecla P.
- Ecrã de fim de jogo dentro do próprio ncurses, com opção de jogar outra vez sem ter de voltar a correr o programa.
- Modo B-Type do Tetris Original (O Tetris B-Type é um modo de jogo clássico presente nas primeiras versões da Nintendo, como o Tetris para a NES e o Tetris para a Game Boy. O objetivo é eliminar exatamente 25 linhas num tabuleiro que começa com blocos aleatórios pré-carregados, vai pedir ao utilizador um nivel de dificuldade) 

## Sessão 5 - 28/08/2026

**Funcionalidades implementadas hoje:**
- Pausa com a tecla `P`. O jogo congela e o tabuleiro fica à vista por baixo do aviso. O contador da gravidade também para, senão a peça descia logo ao voltar.
- Menu de modos e ecrã de fim de jogo, os dois dentro do ncurses. Antes o resultado era escrito com `printf` depois do `endwin()` e era preciso correr o programa outra vez para jogar mais uma partida; agora dá para repetir ou voltar ao menu ali mesmo.
- O `main()` passou a ser o ciclo menu → partida → ecrã final → menu. Como se joga várias vezes seguidas, o estado deixou de poder ser posto só na declaração das variáveis e nasceu a `reiniciarJogo()`.
- Modo B-Type, o clássico da NES: limpar 25 linhas num tabuleiro que já começa com blocos. Pedem-se duas coisas separadas antes de começar, como no original — o nível de velocidade e a altura da pilha inicial, os dois de 0 a 9. Por serem separadas, dá para pedir uma pilha alta a jogar devagar.
- É o primeiro modo em que o jogo também se pode ganhar, e não só perder. O painel ganhou um contador "Faltam" e o ecrã final distingue os dois casos.
- Um recorde por modo, em vez de um só: no B-Type joga-se contra um objetivo fixo, portanto as pontuações não são da mesma ordem. O `recorde.txt` passou a guardar dois números.
- Sorteio das peças por sacos de sete, como no Tetris moderno, em vez do `rand() % 7`. As sete aparecem uma vez cada antes de qualquer uma repetir — acabaram as esperas longas pelo I.
- Atraso na travagem (*lock delay*). Uma peça assente espera meio segundo antes de travar, e cada mexida durante essa espera põe a contagem a zero. Antes travava na jogada seguinte da gravidade, o que nos níveis altos obrigava a ter a peça no sítio certo antes de ela aterrar.
- Peça guardada (*hold*), com a tecla `C`, e uma caixa "GUARDADA" no painel. Só uma troca por peça.
- Comandos só nas setas, e rotação nos dois sentidos: `Z` para a esquerda, `X` para a direita. O `A`, `D`, `S` e `W` saíram. A conta da rotação é a mesma lida ao contrário, não foi preciso escrever nenhuma tabela de posições.

**Maior dificuldade encontrada e como resolvi (ou não resolvi...):**
- O sistema de saco de peças pois genuinamente não tinha nenhuma ideia da sua logica então fui ver projetos como referencia e perguntar a IA como isso funciona

**Próximo passo planeado:**
- Ainda mais polimento se possivel. isto cada vez fica mais curto quanto mais eu fico perto do fim do meu projeto
