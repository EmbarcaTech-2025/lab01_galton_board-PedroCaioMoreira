/*
 * galton_board.c
 * Projeto Galton Board - 
 * Placa: BitdogLab (RP2040 + SSD1306 integrado)
 * VSCode para programação
 *
 * Aplicação:
 * Esse projeto simula um tabuleiro de Galton, que demonstra empiricamente como a distribuição normal (curva de Gauss) emerge
 * de eventos binários com probabilidades iguais. As "bolas" descem e colidem com pinos, desviando aleatoriamente para a esquerda
 * ou direita, acumulando-se em bins inferiores. O resultado visual forma a curva característica da distribuição normal.
 */

 #include <stdio.h>
 #include <stdlib.h>
 #include <string.h>
 #include <math.h>
 #include "pico/stdlib.h"
 #include "pico/time.h"
 #include "inc/ssd1306.h"
 #include "hardware/i2c.h"
 
 #define I2C_SDA       14
 #define I2C_SCL       15
 #define BUTTON_A_PIN   5
 #define BUTTON_B_PIN   6
 
 #define BASE_PINS     15
 #define ROWS          BASE_PINS
 #define TOTAL_BALLS   100
 #define RELEASE_DELAY 10000
 #define NUM_BINS       7
 
 static int display_width;
 static int display_height;
 static float step_x;
 static float step_y;
 
 // Representa uma bola no experimento
 // Contém posição (x,y), se está ativa, se já foi contada e a última linha que ela passou
 typedef struct {
     float x, y;         // posição atual da bola
     bool active;        // indica se a bola ainda está se movendo
     bool counted;       // indica se já foi registrada no bin
     int  last_row;      // última linha de pinos que ela interagiu
 } Ball;
 
 // Representa um pino da estrutura triangular
 // Contém coordenadas inteiras (x, y)
 typedef struct {
     int x, y;           // posição do pino na tela
 } Pin;
 
 // Matriz com todos os pinos do triângulo
 // Cada linha r tem pins[r][c] até pins_per_row[r]
 static Pin pins[ROWS][BASE_PINS];
 
 // Número de pinos por linha
 static int  pins_per_row[ROWS];
 
 // Vetor de bolas que serão lançadas
 static Ball balls[TOTAL_BALLS];
 
 // Contador de bolas acumuladas em cada bin inferior
 static int  bins[NUM_BINS];
 
 // Posição horizontal dos bins (colunas de acúmulo)
 static int  bin_x[NUM_BINS];
 
 // Função auxiliar que retorna 0 ou 1 aleatoriamente
 // Usada para simular 50% de chance de ir para a esquerda ou direita
 static inline uint8_t random_dir(void) {
     return (uint8_t)(rand() & 1u);
 }
 
 // Inicializa os pinos da estrutura triangular
 void init_pins(void) {
     display_width  = ssd1306_width;
     display_height = ssd1306_n_pages * 8;
     int half_h = display_height / 2;
     step_x = (display_width * 0.5f) / (float)(BASE_PINS - 1);
     step_y = (half_h - 1) / (float)(ROWS - 1);
 
     for (int r = 0; r < ROWS; r++) {
         int count = r + 1;
         pins_per_row[r] = count;
         float shift_x = ((display_width - 1) - (count - 1) * step_x) * 0.5f;
         for (int c = 0; c < count; c++) {
             pins[r][c].x = (int)(shift_x + c * step_x + 0.5f);
             pins[r][c].y = (int)(r * step_y + 0.5f);
         }
     }
 
     // Define a posição horizontal de cada bin baseado na base do triângulo
     int start = pins[ROWS - 1][0].x;
     int end   = pins[ROWS - 1][pins_per_row[ROWS - 1] - 1].x;
     float bin_step = (float)(end - start) / (NUM_BINS - 1);
     for (int i = 0; i < NUM_BINS; i++) {
         bin_x[i] = (int)(start + i * bin_step + 0.5f);
     }
 }
 
 // Inicializa uma bola no topo do triângulo
 void init_ball(Ball *b) {
     b->x = pins[0][0].x;
     b->y = 0.0f;
     b->active   = true;
     b->counted  = false;
     b->last_row = -1;
 }
 
 // Atualiza a posição da bola e sua movimentação aleatória
 void update_ball(Ball *b) {
     if (!b->active) return;
     b->y += 1.5f;
     int row = (int)floorf((b->y + step_y * 0.5f) / step_y);
     if (row != b->last_row && row >= 0 && row < ROWS) {
         // 50% de chance para esquerda ou direita
         b->x += (random_dir() ? step_x : -step_x);
         b->last_row = row;
     }
     if (b->y >= display_height) b->active = false;
 }
 
 // Reinicia a simulação (zera bolas e bins)
 void reset_simulation(void) {
     for (int i = 0; i < TOTAL_BALLS; i++) balls[i].active = balls[i].counted = false;
     for (int i = 0; i < NUM_BINS; i++) bins[i] = 0;
 }
 
 int main() {
     stdio_init_all();
     i2c_init(i2c1, ssd1306_i2c_clock * 1000);
     gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
     gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
     gpio_pull_up(I2C_SDA);
     gpio_pull_up(I2C_SCL);
     ssd1306_init();
     struct render_area frame = {0, ssd1306_width - 1, 0, ssd1306_n_pages - 1};
     calculate_render_area_buffer_length(&frame);
 
     srand((uint32_t)time_us_64());
     init_pins();
     reset_simulation();
 
     gpio_init(BUTTON_A_PIN);
     gpio_set_dir(BUTTON_A_PIN, GPIO_IN);
     gpio_pull_up(BUTTON_A_PIN);
     gpio_init(BUTTON_B_PIN);
     gpio_set_dir(BUTTON_B_PIN, GPIO_IN);
     gpio_pull_up(BUTTON_B_PIN);
 
     bool running = false;
     int  ball_count = 0;
     absolute_time_t last_rel = get_absolute_time();
     bool last_btn_a = false;
     bool last_btn_b = false;
 
     static uint8_t buf[ssd1306_buffer_length];
 
     while (true) {
         bool btn_a = (gpio_get(BUTTON_A_PIN) == 0);
         bool btn_b = (gpio_get(BUTTON_B_PIN) == 0);
 
         if (btn_a && !last_btn_a && !running) {
             running = true;
             ball_count = 0;
             reset_simulation();
             last_rel = get_absolute_time();
         }
         if (btn_b && !last_btn_b) {
             running = false;
             reset_simulation();
             memset(buf, 0, sizeof(buf));
             render_on_display(buf, &frame);
         }
         last_btn_a = btn_a;
         last_btn_b = btn_b;
 
         if (running) {
             if (ball_count < TOTAL_BALLS &&
                 absolute_time_diff_us(last_rel, get_absolute_time()) >= RELEASE_DELAY) {
                 init_ball(&balls[ball_count]);
                 ball_count++;
                 last_rel = get_absolute_time();
             }
             memset(buf, 0, sizeof(buf));
             char cnt[5]; snprintf(cnt, sizeof(cnt), "%3d", ball_count);
             ssd1306_draw_string(buf, 0, 0, cnt);
 
             for (int r = 0; r < ROWS; r++)
                 for (int i = 0; i < pins_per_row[r]; i++)
                     ssd1306_set_pixel(buf, pins[r][i].x, pins[r][i].y, true);
 
             for (int i = 0; i < ball_count; i++) {
                 Ball *b = &balls[i];
                 if (b->active) {
                     update_ball(b);
                     ssd1306_set_pixel(buf, (int)roundf(b->x), (int)roundf(b->y), true);
                 } else if (!b->counted) {
                     int min_dist = 10000, idx = 0;
                     for (int j = 0; j < NUM_BINS; j++) {
                         int dist = abs((int)b->x - bin_x[j]);
                         if (dist < min_dist) { min_dist = dist; idx = j; }
                     }
                     bins[idx]++;
                     b->counted = true;
                 }
             }
             for (int j = 0; j < NUM_BINS; j++) {
                 for (int h = 0; h < bins[j] && h < display_height / 2; h++) {
                     int py = display_height - 1 - h;
                     ssd1306_set_pixel(buf, bin_x[j], py, true);
                 }
             }
             render_on_display(buf, &frame);
             if (ball_count >= TOTAL_BALLS) {
                 bool any = false;
                 for (int i = 0; i < TOTAL_BALLS; i++) if (balls[i].active) { any = true; break; }
                 if (!any) running = false;
             }
         } else {
             sleep_ms(20);
         }
     }
     return 0;
 }
 
