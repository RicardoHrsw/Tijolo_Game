# Guia de compilação

## O que é preciso

- **MSYS2** instalado em `C:\msys64` (é o caminho que o `Makefile` procura).
- Dentro do MSYS2, o compilador, o ncurses (versão *wide*, `ncursesw`) e o make:

```
pacman -S mingw-w64-ucrt-x86_64-gcc
pacman -S mingw-w64-ucrt-x86_64-ncurses
pacman -S mingw-w64-ucrt-x86_64-make
```

## Comandos

Na pasta do projeto:

```
mingw32-make          compila
mingw32-make run      compila e joga
mingw32-make clean    apaga o executável
```

Funcionam tal e qual no PowerShell, no `cmd.exe` e no Git Bash. O `Makefile`
trata sozinho de encontrar o ncurses.

Em Linux é o mesmo, mas com `make` em vez de `mingw32-make`.

## Onde jogar

**No Windows, jogar no `cmd.exe`** — é onde as cores das peças saem certas. No
PowerShell duas peças aparecem com a cor errada (o T fica igual ao fundo da
janela e o O fica quase branco), porque a consola do PowerShell traz uma
paleta própria que substitui dois lugares da tabela de cores.

A janela tem de ter pelo menos **56 colunas por 24 linhas**; abaixo disso o
jogo avisa e não arranca.
