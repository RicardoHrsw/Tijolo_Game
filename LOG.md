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
