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
