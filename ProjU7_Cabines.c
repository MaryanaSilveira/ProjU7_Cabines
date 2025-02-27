//Projeto Final: “Biblioreserva”: Interface de visualização e reserva de cabines disponíveis em uma biblioteca.
//Embarcatech: Residência Tecnológica em Sistemas Embarcados
//Aluna: Maryana Souza Silveira

#include <stdio.h> //Biblioteca padrão para entrada e saída
#include "pico/stdlib.h" // Biblioteca padrão da Raspberry Pi Pico
#include "hardware/i2c.h" //Biblioteca que fornece funções para comunicação I2C
#include "hardware/adc.h" //Biblioteca para trabalhar com o ADC
#include "auxiliares/ssd1306.h" //Biblioteca auxiliar para o display
#include "auxiliares/font.h" //Biblioteca auxiliar para fontes no display
#include "hardware/pio.h" // Biblioteca para controle de PIO
#include "ProjU7_Cabines.pio.h" // Código do programa para controle de LEDs WS2812

//Definições de constantes a serem utilizadas no código
#define buttonA 5 // Pino do botão a
#define buttonB 6 // Pino do botão b
#define buttonjoy 22 // Pino do botão do joystick
#define eixo_x_joystick 26  // GPIO para eixo X
#define eixo_y_joystick 27  // GPIO para eixo Y
#define I2C_PORT i2c1 // Define o barramento I2C a ser utilizado (i2c1)
#define I2C_SDA 14 // Define o pino SDA para I2C
#define I2C_SCL 15 // Define o pino SCL para I2C
#define endereco 0x3C // Define o endereço do dispositivo I2c
#define matriz_leds 7 // Pino de controle dos LEDs WS2812

//Definições de variáveis a serem utilizadas no código
// Variável global para armazenar a cor (Entre 0 e 255 para intensidade)
uint8_t r = 60; // Intensidade do vermelho
uint8_t g = 60; // Intensidade do verde
uint8_t b = 60; // Intensidade do azul
//Variáveis para controle de linha e coluna do cursor da interface 1
int row = 24;
int col = 28;
//Variáveis para linha e coluna da cabine a ser escolhida
int linha_cabine = 36;
int coluna_cabine = 24;

int opcao = 32; //Variável para controle da opção a ser escolhida na interface 3
bool reinicia = 0; //Variável para controle do programa (reiniciar)
bool reserva = 0; //Variável para verificar se o usuário selecionou a opção de reserva de cabine
bool acabou = 0; //Variável para indicar que o usuário reservou a cabine

char password[4] = {'1', '2', '3', '4'}; // Senha para acesso ao programa
char input_password[4] = {'0', '0', '0', '0'}; // Vetor para armazenar a senha digitada
int pass_index = 0; // Índice da senha
int password_ok = 0; // Variável para verificar se a senha está correta

//Variáveis para comparação do valor do joystick (centro)
uint16_t valor_x=1912; 
uint16_t valor_y=1964;

ssd1306_t ssd; // Inicializa a estrutura do display
absolute_time_t last_time = {0};  // Último tempo de pressão do botão

int cabines[25] = { //Status inicial das cabines
  1, 2, 2, 1, 2,
  2, 3, 3, 2, 2,
  1, 2, 2, 1, 2,
  2, 1, 2, 2, 2,
  2, 2, 2, 2, 1,
};

//Chamada das funções a serem utilizadas 
//Função de configurações iniciais
void initial_configs();

//Função da interrupção
void gpio_irq_handler(uint gpio, uint32_t events);

//Funções para o display
void mensagem_display(int comando); // Imprime mensagens no display de acordo com o comando recebido
void navega_interface(int interface); // Navega entre as opções do display através do joystick
void ssd1306_square(ssd1306_t *ssd, uint8_t x, uint8_t y); // Desenha um quadrado no display

//Função para liberar o acesso ao programa
void acess_granted ();

//Funções para visualização ou reserva de cabines
void print_cabines(bool controle); // Mostra o status atual das cabines
void reserva_cabine(int *cabines); // Reserva a cabine desejada

//Função para associar a grade de cabines na tela com a matriz de leds 5x5
int adereça_index(int linha_cabine, int coluna_cabine);
//Funções para a matriz de leds 5x5
static inline void put_pixel(uint32_t pixel_grb);
static inline uint32_t urgb_u32(uint8_t r, uint8_t g, uint8_t b);

int main()
{
    initial_configs(); // Inicializa as configurações iniciais

    gpio_set_irq_enabled_with_callback(buttonjoy, GPIO_IRQ_EDGE_FALL, true, &gpio_irq_handler); // Habilita a interrupção do joystick

    while(true){
        mensagem_display(1); // Mostra a interface da senha

        navega_interface(1); // Chama a função que auxilia o usuário a se mover entre as opções de senhas
        if (password_ok == 1) { // Verifica se a senha está correta
            mensagem_display(2); // Exibe uma mensagem informando que a senha está correta
            acess_granted(); // Libera o acesso e segue o programa

            if (reinicia == 1) { // Verifica se o usuário terminou a sessão
                password_ok = 0; // Reinicia a variável de senha correta para a volta das configurações iniciais
                for (int i = 0; i < 4; i++) { // Retorna o vetor de controle de senha para o valor 0000
                    input_password[i] =  '0'; 
                }
                reinicia = 0; // Reinicia a variável de reinício
            }

        } else if (password_ok == 2) { // Senha incorreta
            mensagem_display(3); // Exibe uma mensagem informando que a senha está incorreta
            for (int i = 0; i < 4; i++) { // Retorna o vetor de controle de senha para o valor 0000
              input_password[i] =  '0';      
              }
            password_ok = 0; // Reinicia a variável de senha correta
        }
    }
}

void initial_configs(){

  gpio_init(buttonA); // Inicializa o pino do botão A
  gpio_set_dir(buttonA, GPIO_IN); // Configura o pino como entrada
  gpio_pull_up(buttonA); // Habilita o pull-up do botão A

  gpio_init(buttonB); // Inicializa o pino do botão B
  gpio_set_dir(buttonB, GPIO_IN); // Configura o pino como entrada
  gpio_pull_up(buttonB); // Habilita o pull-up do botão B

  gpio_init(buttonjoy); // Inicializa o pino do botão do joystick
  gpio_set_dir(buttonjoy, GPIO_IN); // Configura o pino como entrada
  gpio_pull_up(buttonjoy); // Habilita o pull-up do botão do joystick

  i2c_init(I2C_PORT, 400 * 1000); // Inicializa o barramento I2C a 400kHz

  gpio_set_function(I2C_SDA, GPIO_FUNC_I2C); // Configura a função do pino SDA
  gpio_set_function(I2C_SCL, GPIO_FUNC_I2C); // Configura a função do pino SCL
  gpio_pull_up(I2C_SDA); // Habilita o pull-up do pino SDA
  gpio_pull_up(I2C_SCL); // Habilita o pull-up do pino SCL
  ssd1306_init(&ssd, 128, 64, false, endereco, I2C_PORT); // Inicializa o display
  ssd1306_config(&ssd); // Configura o display
  ssd1306_send_data(&ssd); // Envia os dados para o display

  // Limpa o display. O display inicia com todos os pixels apagados.
  ssd1306_fill(&ssd, false);
  ssd1306_send_data(&ssd);

  adc_init(); // Inicializa o ADC
  adc_gpio_init(eixo_x_joystick); // Inicializa o pino do eixo X do joystick
  adc_gpio_init(eixo_y_joystick); // Inicializa o pino do eixo Y do joystick

  //Inicializa o PIO
  PIO pio = pio0; 
  int sm = 0; 
  uint offset = pio_add_program(pio, &ws2812_program);

  // Inicializa o programa para controle dos LEDs WS2812
  ws2812_program_init(pio, sm, offset, matriz_leds, 800000, false); // Inicializa o programa para controle dos LEDs WS2812
  
}

void gpio_irq_handler(uint gpio, uint32_t events) { // Função de interrupção
  absolute_time_t time = get_absolute_time(); // Obtém o tempo atual
  if (time - last_time > 500000) { // Verifica se o tempo de pressão do botão é maior que 500ms
    last_time = time; // Atualiza o tempo de pressão do botão

    if (gpio == buttonjoy) { // Verifica se o botão do joystick foi pressionado
      char pass; // Variável para relacionar a opção escolhida pelo usuário com o número da senha no vetor
      switch (row) { // Verifica a linha do caracter escolhido
          case 24: 
            if (col == 28){ // Verifica a coluna do caracter escolhido
                pass = '1';
            }
            else{
                pass = '6';
            }
        break;
        case 40: 
            if (col == 28){
                pass = '2';
            }
            else{
                pass = '7';
            }
        break;
        case 56: 
            if (col == 28){
                pass = '3';
            }
            else{
                pass = '8';
            }
        break;
        case 72: 
            if (col == 28){
                pass = '4';
            }
            else{
                pass = '9';
            }
        break;
        case 88: 
            if (col == 28){
                pass = '5';
            }
            else{
                pass = '0';
            }
        default:
        break;
    }
      input_password[pass_index] = pass; // Armazena o valor do botão pressionado na senha
      pass_index++; // Incrementa o índice da senha
      if (pass_index == 4) { // Verifica se a senha completa foi digitada
        pass_index = 0; // Reinicia o índice da senha
        for (int i = 0; i < 4; i++) { // Verifica se a senha está correta
          if (input_password[i] != password[i]) { // Compara com o vetor que armazena a senha correta
            password_ok = 2; // Senha incorreta
          } else {
            password_ok = 1; // Senha correta
          }
        }
      }
    }

    if (gpio == buttonA){ // Verifica se o botão do joystick foi pressionado
      switch (opcao){ // Verifica qual opção o usuário escolheu
        case 32: //Ver o status das cabines
          print_cabines(1);
          break;
          case 40: //Reservar uma cabine
            reserva = 1; //Modifica a variável de reserva para a função de reserva ser chamada no loop principal
          break;
          case 48: // Reiniciar
            print_cabines(0); //Apaga os leds 5x5
            reinicia = 1; // Modifica a variável de reiniciar
          break;
      }
    }

    if (gpio == buttonB){ //Verifica se o botão B foi pressionado
      //Atribui a linha e coluna da opção escolhida a um índice na matriz 5x5
      int indice_cabine = adereça_index(linha_cabine, coluna_cabine);

      if (cabines[indice_cabine] == 2){ //Só reserva se a cabine estiver disponível (cor verde)
        cabines[indice_cabine] = 3;
        print_cabines(1);
        acabou = 1; // Finaliza a parte de escolher cabine
      }
      if (acabou ==1 ){
        print_cabines(0);
      }
    }
  }
}

void mensagem_display(int comando){ // Função para expor mensagens no display
  switch(comando){ //Expõe a mensagem de acordo com o comando recebbido
    case 1:{
      ssd1306_rect(&ssd, 3, 3, 122, 60, true, false); // Desenha um retângulo
      ssd1306_draw_string(&ssd, "Digite sua", 24, 6); // Desenha uma string
      ssd1306_draw_string(&ssd, "senha", 40, 16); // Desenha uma string
      ssd1306_draw_string(&ssd, "1 2 3 4 5", 24, 28); // Desenha uma string 
      ssd1306_draw_string(&ssd, "6 7 8 9 0", 24, 40); // Desenha uma string 
      ssd1306_draw_string(&ssd, input_password, 48, 52); // Desenha uma string 
      ssd1306_send_data(&ssd); // Atualiza o display
    }
    break;
    case 2:{
      ssd1306_fill(&ssd, false); // Limpa o display
      ssd1306_draw_string(&ssd, "Senha correta", 8, 24); // Desenha uma string
      ssd1306_send_data(&ssd); // Atualiza o display
      sleep_ms(1000); // Aguarda 1s
    }
    break;
    case 3:{
      ssd1306_fill(&ssd, false); // Limpa o display
      ssd1306_draw_string(&ssd, "Senha", 8, 24); // Desenha uma string
      ssd1306_draw_string(&ssd, "incorreta", 8, 32); // Desenha uma string
      ssd1306_send_data(&ssd); // Atualiza o display
      sleep_ms(1000); // Aguarda 1s
    }
    break;
    case 4:{
      ssd1306_fill(&ssd, false); // Limpa o display
      ssd1306_draw_string(&ssd, "Acesso", 8, 24); // Desenha uma string
      ssd1306_draw_string(&ssd, "concebido",8,32); // Desenha uma string
      ssd1306_send_data(&ssd); // Atualiza o display
      sleep_ms(1000); // Aguarda 2s
    }
    break;
    case 5:{
      ssd1306_fill(&ssd, false); // Limpa o display
      ssd1306_rect(&ssd, 0, 0, 122, 60, true, false); // Desenha um retângulo
      ssd1306_draw_string(&ssd, "Clique no",24,8); // Desenha uma string
      ssd1306_draw_string(&ssd, "botao A quando",8,16); // Desenha uma string
      ssd1306_draw_string(&ssd, "escolher ",32,24); // Desenha uma string
      ssd1306_draw_string(&ssd, "1 ver status",8,32); // Desenha uma string
      ssd1306_draw_string(&ssd, "2 reservar",8,40); // Desenha uma string
      ssd1306_draw_string(&ssd, "3 reiniciar",8,48); // Desenha uma string
      ssd1306_send_data(&ssd); // Atualiza o display
    }
    break;
    case 6:{
      ssd1306_fill(&ssd, false); // Limpa o display
      ssd1306_draw_string(&ssd, "Pressione B",24,8); // Desenha uma string
      ssd1306_draw_string(&ssd, "para reservar",8,16); // Desenha uma string
      for (int i = 24; i<64; i+=8){
        for (int j = 36; j<76; j+=8){
          ssd1306_rect(&ssd, i, j, 8, 8, true, false); // Desenha um retângulo
        }
      }
      ssd1306_send_data(&ssd); // Atualiza o display
    }
    break;
    case 7:{
      ssd1306_fill(&ssd, false); // Limpa o display
      ssd1306_draw_string(&ssd, "Esta cabine ",8,8); // Desenha uma string
      ssd1306_draw_string(&ssd, "ja tem reserva",8,16); // Desenha uma string
      ssd1306_send_data(&ssd); // Atualiza o display
      sleep_ms(1000);
    }
  }
}

void navega_interface(int interface){ //Função para mover o cursor no display para cada interface do programa
  switch (interface){ 
    case 1:{ //Interface inicial: digitando a senha
      uint16_t adc_value_y;  // Variável para armazenar o valor do ADC do eixo Y
      adc_select_input(1); // Seleciona o ADC para eixo Y
      adc_value_y = adc_read(); // Lê o valor do ADC
      if (adc_value_y > (valor_x + 250)){
        if (row < 88){
          row += 16;
        } else {
          row = 24;
          if (col == 28){
            col = 40;
          } else {
            col = 28;
          }
        }
      }
      if (adc_value_y < (valor_x - 250)){
       if (row > 24){
          row -= 16;
        } else {
          row = 88;
          if (col == 28){
            col = 40;
          } else {
            col = 28;
          }
        }
  
      }
      ssd1306_fill(&ssd, false); // Limpa o display
      mensagem_display(1); //Mostra a tela inicial com os números
      ssd1306_square(&ssd, row, col); // Desenha o quadrado que acompanha o joystick
      ssd1306_send_data(&ssd); // Atualiza o display
  
      sleep_ms(100); // Aguarda 100ms

    }
    break;

    case 2:{ //Segunda interface: escolhendo a opção
      uint16_t read_x;  // Variável para armazenar o valor do ADC do eixo x
      adc_select_input(0); // Seleciona o ADC para eixo x
      read_x = adc_read(); // Lê o valor do ADC
      if (read_x > (valor_y + 250)){
        if (opcao > 32){
          opcao -= 8;
        } else {
          opcao = 48;
        }
      }
      if (read_x < (valor_x - 250)){
        if (opcao < 48){
          opcao += 8;
        } else {
          opcao = 32;
        } 
      }
      ssd1306_fill(&ssd, false); // Limpa o display
      mensagem_display(5); //Mostra as opções da segunda interface
      ssd1306_square(&ssd, 8, opcao); // Desenha o quadrado que acompanha o joystick (calibrado para que o movimento em y acompanhe o joystick)
      ssd1306_send_data(&ssd); // Atualiza o display

    }
    break;

    case 3:{//Terceira interface: Reservando a cabine
      uint16_t adc_value_x; // Variável para armazenar o valor do ADC do eixo X
      uint16_t adc_value_y;  // Variável para armazenar o valor do ADC do eixo Y
      adc_select_input(1); // Seleciona o ADC para eixo Y
      adc_value_y = adc_read(); // Lê o valor do ADC

      //Move o cursor na horiontal
      if (adc_value_y > (valor_x + 250)){
        if (linha_cabine < 68){
          linha_cabine += 8;
        } else {
          linha_cabine = 36;
          if (coluna_cabine < 56){
            coluna_cabine +=8;
          } else {
            coluna_cabine = 24;
            linha_cabine = 36;
          }
        }
      }

      if (adc_value_y < (valor_x - 250)){
        if (linha_cabine > 36){
          linha_cabine -= 8;
        } else {
          linha_cabine = 68;
          if (coluna_cabine > 24){
            coluna_cabine -=8;
          } else {
            coluna_cabine = 56;
            linha_cabine = 36;
          }
        }
      }
  
      adc_select_input(0); // Seleciona o ADC para eixo X
      adc_value_x = adc_read(); // Lê o valor do ADC

      //Move o cursor na vertical
      if (adc_value_x > (valor_y + 250)){
        if (coluna_cabine > 24){
          coluna_cabine -= 8;
        } else {
          coluna_cabine = 56;
          if (linha_cabine > 36){
            linha_cabine -= 8;
          } else {
            linha_cabine = 68;
          }
        }
      }

      if (adc_value_x < (valor_y - 250)){
        if (coluna_cabine < 56 ){
          coluna_cabine += 8;
        } else {
          coluna_cabine = 24;
          if (linha_cabine < 68){
            linha_cabine += 8;
          } else {
            linha_cabine = 36;
          }
        }
      }
      
      ssd1306_fill(&ssd, false); // Limpa o display
      mensagem_display(6); //Mostra as opções da interface 3
      ssd1306_square(&ssd, linha_cabine, coluna_cabine); // Desenha o quadrado que acompanha o joystick
      ssd1306_send_data(&ssd); // Atualiza o display
  
      sleep_ms(100); // Aguarda 100ms

    }
    break;
  }
}

void ssd1306_square(ssd1306_t *ssd, uint8_t x, uint8_t y){ // Função para desenhar um quadrado no display
  for (uint8_t i = 0; i < 8; ++i){ // Desenha um quadrado de 8x8 pixels
    static uint8_t square[] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff}; // Define o quadrado
    uint8_t line = square[i]; // Obtém a linha do quadrado
    for (uint8_t j = 0; j < 8; ++j){ // Desenha a coluna do quadrado
      ssd1306_pixel(ssd, x + i, y + j, line & (1 << j)); // Desenha o pixel
    }
  }
}

void acess_granted(){
  mensagem_display(4); //Exibe mensagem de acesso concedido
  mensagem_display(5); //Exibe as opções da segunda interfaxe
  gpio_set_irq_enabled_with_callback(buttonA, GPIO_IRQ_EDGE_FALL, true, &gpio_irq_handler); // Habilita a interrupção do botão A
  
  while (true) { 
    navega_interface(2); //Move o cursor entre as opções da segunda interface

    if (reserva == 1){ //Verifica se a opção de reserva foi escolhida
      reserva_cabine(cabines); //chama a função de reserva de cabines
      reserva = 0; //reinicia a variável reserva
    }
    if (reinicia == 1){ //Verifica se a opção de reiniciar foi escolhida
      break; // sai do loop
    }
    if (acabou == 1){ //Verifica se o usuário já escolheu a cabine
      acabou = 0;
    }
    sleep_ms(100); // Aguarda 100ms
  }
}

void print_cabines(bool controle){ //Mostra os status das cabines na matriz 5x5
  if (controle){ //Caso a variável seja positiva, acende os leds
    uint32_t red = urgb_u32(r, 0, 0);
    uint32_t green = urgb_u32(0, g, 0);
    uint32_t blue = urgb_u32(0, 0, b);

    for (int i = 0; i < 25; i++) { //Acende de acordo com a cor na matriz de leds, 1 - vermelho(ocupado), 2-verde(liberado), 3 - azul(reservado)
        if (cabines[i] == 1) {
            put_pixel(red);
        } else if (cabines[i] == 2) {
            put_pixel(green);
        } else if (cabines[i] == 3) {
            put_pixel(blue);
        } else {
           put_pixel(urgb_u32(0, 0, 0));
        }
    }
  } else {
    for (int i = 0; i < 25; i++) { //Apaga os leds 
      put_pixel(urgb_u32(0, 0, 0));
    }
  }
}

void reserva_cabine(int *cabines){ //Função chamada para reservar uma cabine
  gpio_set_irq_enabled_with_callback(buttonB, GPIO_IRQ_EDGE_FALL, true, &gpio_irq_handler); // Habilita a interrupção do botão B

  mensagem_display(6); //Mostra a interface de escolha da cabine
  while (true){
    navega_interface(3); //Navega o cursor pelas opções
    if (reinicia == 1){ //Caso a opção de reiniciar seja escolhida
      break;
    }
    if (acabou == 1){ //Caso o usuário já tenha escolhido a cabine
      acabou = 0;
      break;
    }
  }
} 

//Função para associar a grade de cabines na tela com a matriz de leds 5x5
int adereça_index(int linha, int coluna){
  int indice_cabine = 0; //Variável a ser retornada indicando o led a ser aceso com a luz azul
  switch(linha_cabine){ //Verifica a linha da cabine
    case 36:
      switch(coluna_cabine){ //Verfica a coluna da cabine
        case 24:
        indice_cabine = 24; //Atribui o índice correpondente a matriz de leds 5x5
          break;
        case 32:
        indice_cabine = 15;
          break;
        case 40:
        indice_cabine = 14;
          break;
        case 48:
        indice_cabine = 5;
          break;
        case 56:
        indice_cabine = 4;
          break;
      }
    break;
    case 44:
      switch(coluna_cabine){
        case 24:
        indice_cabine = 23;
          break;
        case 32:
        indice_cabine = 16;
          break;
        case 40:
        indice_cabine = 13;
          break;
        case 48:
        indice_cabine = 6;
          break;
       case 56:
       indice_cabine = 3;
          break;
      }
    break;
    case 52:
      switch(coluna_cabine){
        case 24:
        indice_cabine = 22;
          break;
        case 32:
        indice_cabine = 17;
          break;
        case 40:
        indice_cabine = 12;
          break;
        case 48:
        indice_cabine = 7;
          break;
        case 56:
        indice_cabine = 2;
          break;
      }
    break;
    case 60:
      switch(coluna_cabine){
        case 24:
        indice_cabine = 21;
          break;
        case 32:
        indice_cabine = 18;
          break;
        case 40:
        indice_cabine = 11;
          break;
        case 48:
        indice_cabine = 8;
          break;
        case 56:
        indice_cabine = 1;
          break;
      }
    break;
    case 68:
      switch(coluna_cabine){
        case 24:
        indice_cabine = 20;
          break;
        case 32:
        indice_cabine = 19;
          break;
        case 40:
        indice_cabine = 10;
          break;
        case 48:
        indice_cabine = 9;
          break;
        case 56:
        indice_cabine = 0;
          break;
      }
    break;
}
  return indice_cabine; //Devolve o valor do índice
}

static inline void put_pixel(uint32_t pixel_grb) // Função para atualizar um LED
{
    pio_sm_put_blocking(pio0, 0, pixel_grb << 8u); // Atualiza o LED com a cor fornecida
}

static inline uint32_t urgb_u32(uint8_t r, uint8_t g, uint8_t b) // Função para converter RGB para uint32_t
{
    return ((uint32_t)(r) << 8) | ((uint32_t)(g) << 16) | (uint32_t)(b); // Retorna a cor em formato uint32_t 
}
