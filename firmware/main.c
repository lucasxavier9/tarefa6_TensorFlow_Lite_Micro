#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include <uart.h>
#include <console.h>
#include <generated/csr.h>
#include <irq.h>

#include "inference.h" // Inclui definições do modelo TensorFlow Lite Micro


static char *readstr(void)
{
    char c[2];
    static char s[64];
    static int ptr = 0;

    if(readchar_nonblock()) {
        c[0] = readchar();
        c[1] = 0;
        switch(c[0]) {
            case 0x7f:
            case 0x08:
                if(ptr > 0) {
                    ptr--;
                    putsnonl("\x08 \x08");
                }
                break;
            case 0x07:
                break;
            case '\r':
            case '\n':
                s[ptr] = 0x00;
                putsnonl("\n");
                ptr = 0;
                return s;
            default:
                if(ptr >= (sizeof(s) - 1))
                    break;
                putsnonl(c);
                s[ptr] = c[0];
                ptr++;
                break;
        }
    }
    return NULL;
}

static char *get_token(char **str)
{
    char *c, *d;

    c = (char *)strchr(*str, ' ');
    if(c == NULL) {
        d = *str;
        *str = *str+strlen(*str);
        return d;
    }
    *c = 0;
    d = *str;
    *str = c+1;
    return d;
}

static void prompt(void)
{
    printf("RUNTIME>");
}

static void help(void)
{
    puts("=========================================================");
    puts("|              Comandos disponíveis:                     |");
    puts("|  help                            - Mostra commands     |");
    puts("|  reboot                          - Reboot CPU          |");
    puts("|  led_TF                          - inciar o projeto    |");
    puts("|  no loop LED_TF - pressione CTRL+P para encerrar       |");
    puts("=========================================================");
}

static void reboot(void)
{
    ctrl_reset_write(1);
}

static void led_TF(void)
{
    printf("Inicializando modelo TensorFlow Lite Micro...\n");
    
    // Inicializa o modelo
    inference_init();
    
    // Teste inicial dos LEDs externos
    // Mostra uma sequência simples para verificar funcionamento
    printf("\n=== TESTE DE LEDs EXTERNOS  ===\n");
     
    // Teste 1: Todos piscando
    printf("Teste: Todos os LEDs piscando...\n");
    for(int blink = 0; blink < 5; blink++) {
        leds_out_write(0xFF);
        printf("  Todos LIGADOS (0xFF)\n");
        for(volatile int d = 0; d < 400000; d++);
        leds_out_write(0x00);
        printf("  Todos DESLIGADOS (0x00)\n");
        for(volatile int d = 0; d < 400000; d++);
    }
    
    printf("=== TESTE DE LEDs COMPLETO ===\n\n");
    
    printf("Executando inferencias continuas (pressione Ctrl+P para parar)...\n");
    printf("Modelo: hello_world - simulando função seno\n\n");
    
    // Variáveis para entrada e controle
    float x = 0.0f;
    float x_increment = 0.1f;  // Incremento a cada iteração
    const float pi = 3.14159265f;
    
    int iteration = 0;
    
    // Loop principal de inferência
    while(1) {

        // Executa inferência
        float y_pred = inference_run(x);
        
        // Converte saída do modelo para padrão de LEDs (0-255)
        unsigned char led_pattern = inference_output_to_led_pattern(y_pred);
        
        // Define padrão de LEDs baseado na saída
        // Aqui usamos uma barra simples: mais LEDs acesos para valores maiores
        unsigned char led_output = 0;
        int num_leds_on = (led_pattern * 9) / 256;  // 0-8 LEDs
        
        // Limita o número máximo de LEDs a 8
        if (num_leds_on > 8) num_leds_on = 8;
        
        // Configura os LEDs conforme o número calculado
        // Bits 0-7 correspondem aos 8 LEDs
        for(int i = 0; i < num_leds_on; i++) {
            led_output |= (1 << i);
        }
        
        leds_out_write(led_output);
        
        // Imprime informações a cada 10 iterações
        if (iteration % 10 == 0) {
            // Formata a saída para melhor legibilidade
            int x_int = (int)(x * 1000);  // x em miliradians
            int y_pred_int = (int)(y_pred * 10000);  // y_pred em 4 casas decimais
            
            printf("Iterações:%4d | valor_x = %d.%03d | y_previsto = %s%d.%04d | LEDs =0x%02X (%d/8)\n",
                   iteration, 
                   x_int / 1000, x_int % 1000,
                   (y_pred >= 0) ? "+" : "-",
                   (y_pred_int < 0 ? -y_pred_int : y_pred_int) / 10000,
                   (y_pred_int < 0 ? -y_pred_int : y_pred_int) % 10000,
                   led_output, num_leds_on);
        }
        
        // Atualiza x para a próxima iteração
        x += x_increment;
        if (x >= 2.0f * pi) {
            x = 0.0f;
            printf("\n--- Ciclo completo (0 a 2*PI) ---\n\n");
        }
        
        iteration++;
        
        // Pequena pausa para visualização
        for(volatile int i = 0; i < 250000; i++);
        
        // Verifica se Ctrl+P foi pressionado para encerrar
        if(readchar_nonblock()) {
            char ch = readchar();
            if(ch == 0x10) {  // Ctrl+C
                printf("\n\nCTRL+P pressionado.");
                printf("\nDesligando todos os leds..");
                leds_out_write(0x00);   // apaga LEDs
                printf("\nEncerrando...\n\n");
                break;
            }
        }
    }
    
    printf("SUCESS: Modelo finalizado!\n");
}

static void console_service(void) {
    char *str;
    char *token;

    str = readstr();
    if(str == NULL) return;
    token = get_token(&str);
    if(strcmp(token, "help") == 0)
        help();
    else if(strcmp(token, "reboot") == 0)
        reboot();
    else if(strcmp(token, "led_TF") == 0)
        led_TF();
    else
        printf("Comando desconhecido: %s\n", token);
    prompt();
}

int main(void) {
#ifdef CONFIG_CPU_HAS_INTERRUPT
    irq_setmask(0);
    irq_setie(1);
#endif
    uart_init();

    printf("Hellorld!\n");
    help();
    prompt();

    while(1) {
        console_service();
    }

    return 0;
}
