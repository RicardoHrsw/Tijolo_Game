## Como compilar

Precisa de um compilador C e da biblioteca ncurses (versão *wide*, `ncursesw`).

### Windows (MSYS2 / UCRT64)

```
gcc -std=c11 -Wall -Wextra TijoloGame.c -o TijoloGame.exe -IC:/msys64/ucrt64/include/ncursesw -lncursesw
```

O `-I` é necessário porque neste pacote o `curses.h` não está na raiz dos
includes, mas sim em `include/ncursesw/`. Se a instalação do MSYS2 estiver
noutro sítio, é esse caminho que muda.

O comando acima funciona tal e qual no PowerShell, no `cmd.exe` e no Git Bash.

Para instalar o ncurses, caso falte:

```
pacman -S mingw-w64-ucrt-x86_64-ncurses
```

### Linux

Aqui o `curses.h` está no sítio normal, por isso não é preciso o `-I`:

```
gcc -std=c11 -Wall -Wextra TijoloGame.c -o TijoloGame -lncursesw
```

### Com o Makefile

O repositório traz um `Makefile` que trata do `-I` sozinho, tanto no
PowerShell como no `cmd.exe` e no Git Bash:

```
mingw32-make          compila
mingw32-make run      compila e joga
mingw32-make clean    apaga o executável
```

## Cores das peças

**No Windows, jogar no `cmd.exe`.**

Cada peça tem o seu tom próprio, definido em RGB na tabela `PECAS`. Em
terminais que anunciem 256 cores — Git Bash, Windows Terminal, Linux — as
peças usam os índices de cor 8 a 15 e recebem esse tom exacto.

No Windows PowerShell duas peças saem com a cor errada. A consola do
PowerShell traz uma paleta própria que substitui dois lugares da tabela de 16
cores:

| Índice | Cor normal | No PowerShell | Peça afectada |
|--------|-----------|---------------|---------------|
| 5      | magenta   | `#012456`     | T — fica igual ao fundo da janela |
| 6      | amarelo   | `#EEEDF0`     | O — fica quase branco |

Forçar `TERM=xterm-256color` levaria as peças para os índices 8 a 15, que o
PowerShell não altera, mas a consola do Windows não digere bem as sequências
desse terminal e o ecrã sai com lixo. Por isso o jogo respeita o `TERM` que
encontrar e não mexe nele.

No `cmd.exe` a paleta é a de fábrica e as cores saem certas.