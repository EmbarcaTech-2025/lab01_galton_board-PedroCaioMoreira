
# 🎲 Galton Board - Simulador de Distribuição Normal com RP2040 + SSD1306

Este projeto simula uma **Galton Board** (ou máquina de Galton), usando uma placa BitdogLab (RP2040 com display OLED SSD1306 integrado). O objetivo é visualizar a formação da distribuição normal a partir de múltiplas quedas de bolas sobre uma matriz de pinos.

## 📚 Objetivo

Demonstrar de forma visual o princípio da **distribuição normal (gaussiana)**, utilizando física simples de colisões com pinos e sorteios aleatórios para definir o caminho de cada bola até sua coluna final (bin).

---

## ⚙️ Hardware Utilizado

- Placa **BitdogLab** com:
  - Microcontrolador **RP2040**
  - Display OLED **SSD1306** (128x64)
  - Dois botões físicos (GPIO 5 - Botão A, GPIO 6 - Botão B)
- Conexão via **I2C (GPIO 14 e 15)**

---

## 🧠 Estruturas de Dados

### `typedef struct Ball`
Representa uma bola que está "caindo" na simulação.

```c
typedef struct {
    float x, y;        // Posição atual da bola
    bool active;       // Se está em movimento
    bool counted;      // Se já foi registrada no bin inferior
    int  last_row;     // Última linha de pinos que a bola cruzou
} Ball;
```

---

### `typedef struct Pin`
Define um pino fixo do triângulo.

```c
typedef struct {
    int x, y;  // Posição do pino na tela
} Pin;
```

---

### `Pin pins[ROWS][BASE_PINS]`
Matriz de pinos do triângulo. Cada linha contém uma quantidade crescente de pinos, até formar a base com 15 pinos.

---

### `int pins_per_row[ROWS]`
Guarda quantos pinos existem por linha, necessário para percorrer a matriz com eficiência.

---

### `Ball balls[TOTAL_BALLS]`
Vetor de bolas que são lançadas. Até 100 bolas podem ser simuladas por vez.

---

### `int bins[NUM_BINS]` e `int bin_x[NUM_BINS]`
Contadores e posições horizontais dos bins (colunas de acúmulo) na base.

---

## 🔄 Fluxo de Execução do Código

### 1. **Inicialização do Display e GPIOs**

- Configurações I2C, inicialização do display SSD1306 e botões.
- `ssd1306_init()`, `gpio_init()`, etc.

---

### 2. **Configuração Inicial**

- `init_pins()`: Calcula e posiciona os pinos do triângulo.
- `reset_simulation()`: Zera todas as bolas e bins.

---

### 3. **Interação com Usuário**

- **Botão A (GPIO 5)**: Inicia a simulação.
- **Botão B (GPIO 6)**: Interrompe e reinicia tudo.

---

### 4. **Simulação de Bolas**

- A cada `RELEASE_DELAY` microssegundos, uma nova bola é lançada.
- `init_ball(&balls[i])`: Posiciona no topo do triângulo.
- `update_ball()`: Simula queda com deslocamento aleatório:
  
  ```c
  b->x += (random_dir() ? step_x : -step_x); // 50% esquerda ou direita
  ```

---

### 5. **Contagem e Visualização**

- Quando a bola para de se mover, é registrada no bin mais próximo com base na posição `x`.
- Os bins acumulam as bolas para mostrar a curva normal ao final.

---

### 6. **Renderização no Display**

- Toda a lógica de desenho acontece no `buf[]`, e é renderizada via:

  ```c
  render_on_display(buf, &frame);
  ```

---

## 🔍 Funções Principais

| Função             | Descrição |
|--------------------|-----------|
| `random_dir()`     | Retorna aleatoriamente 0 ou 1 (simula bifurcação esquerda/direita). |
| `init_pins()`      | Calcula posição dos pinos do triângulo com base na tela. |
| `init_ball()`      | Inicia uma bola no topo do triângulo. |
| `update_ball()`    | Atualiza posição da bola e trata a lógica de bifurcação. |
| `reset_simulation()` | Zera bolas e bins para nova simulação. |

---

## 📈 Resultado Esperado

Ao pressionar o botão **A**, 100 bolas são lançadas. Ao passarem pelos 15 níveis de pinos, as bolas se acumulam em 7 colunas na parte inferior da tela. Devido à aleatoriedade binária, forma-se uma **distribuição normal** (curva em forma de sino).

---

## 🧠 Aprendizado

Este projeto reforça conceitos de:

- Probabilidade e Estatística
- Física e simulação gráfica
- Programação embarcada com C
- Manipulação de buffers e displays gráficos

---

## ✍️ Autor

**Pedro Caio Moreira Costa** – Estudante de Engenharia Eletrônica  
Universidade Federal de Sergipe  
Programa de Capacitação EmbarcaTech - 2025 - 
