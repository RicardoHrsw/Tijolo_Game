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

### Linux

Aqui o `curses.h` está no sítio normal, por isso não é preciso o `-I`:

```
gcc -std=c11 -Wall -Wextra TijoloGame.c -o TijoloGame -lncursesw