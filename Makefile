#Confeso que esta parte eu usei IA para escrever pois muitos destes conceitos eu pessoalmente não soube

# Tijolo Game - compilacao com ncurses
#
# Uso (funciona no PowerShell, no cmd.exe e no Git Bash):
#   mingw32-make          compila
#   mingw32-make run      compila e joga
#   mingw32-make clean    apaga o executavel

CC     := gcc
ALVO   := TijoloGame.exe
FONTES := TijoloGame.c

# --- Localizacao do ncurses -------------------------------------------------
# No MSYS2 o curses.h nao esta na raiz dos includes, esta em include/ncursesw/,
# por isso e preciso indicar o caminho com -I. Procura-se nas localizacoes
# conhecidas; em Linux nao encontra nenhuma e fica vazio, que e o correto
# porque la o curses.h esta no sitio normal.
#
# Usa-se $(wildcard ...) em vez do pkg-config de proposito: e uma funcao
# interna do make e nao depende da shell, ao contrario do $(shell ...).
NCURSES_INC := $(firstword $(wildcard C:/msys64/ucrt64/include/ncursesw) \
                          $(wildcard C:/msys64/mingw64/include/ncursesw))

ifneq ($(NCURSES_INC),)
  CURSES_CFLAGS := -I$(NCURSES_INC)
endif
CURSES_LIBS := -lncursesw

CFLAGS := -std=c11 -Wall -Wextra $(CURSES_CFLAGS)
LDLIBS := $(CURSES_LIBS)

# --- Apagar ficheiros -------------------------------------------------------
# O 'rm' nao existe no PowerShell nem no cmd.exe. Em Windows chama-se o rm.exe
# do MSYS2 pelo caminho absoluto, que funciona a partir de qualquer shell.
ifeq ($(OS),Windows_NT)
  RM_CMD := C:/msys64/usr/bin/rm.exe -f
else
  RM_CMD := rm -f
endif

$(ALVO): $(FONTES)
	$(CC) $(CFLAGS) $(FONTES) -o $(ALVO) $(LDLIBS)

# --- Correr o jogo ----------------------------------------------------------
# Nao se mexe aqui na variavel TERM. Chegou a experimentar-se fixar um
# TERM=xterm-256color, para as pecas usarem os indices de cor 8..15 em vez dos
# 0..7, mas a consola do Windows nao digere bem as sequencias desse terminal e
# o ecra sai com lixo. O jogo corre melhor no cmd.exe, onde a paleta da
# consola e a de fabrica.
run: $(ALVO)
	"$(CURDIR)/$(ALVO)"

clean:
	-$(RM_CMD) $(ALVO)

.PHONY: run clean
