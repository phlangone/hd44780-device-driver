/**
 * main.c
 * Programa de controle para o driver hd44780_driver (I2C)
 *
 * Autor: Paulo Henrique Langone
 *
 * Descrição:
 *  - Controla o LCD via sysfs (linha, coluna e limpeza)
 *  - Envia texto para o LCD via /dev/hd44780_driver
 *
 * Compilação:
 *  gcc main.c -o main
 *
 * Execução:
 *  sudo ./lcd_control
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

#define SYSFS_BASE "/sys/module/hd44780_driver/parameters"
#define DEV_PATH   "/dev/hd44780_driver"

/**
 * Escreve um valor inteiro em um arquivo sysfs.
 */
int write_sysfs(const char *param, int value)
{
    char path[128];
    int fd;
    ssize_t ret;
    char buffer[16];

    snprintf(path, sizeof(path), "%s/%s", SYSFS_BASE, param);

    fd = open(path, O_WRONLY);
    if (fd < 0)
    {
        perror("Error opening sysfs parameter");
        return -1;
    }

    snprintf(buffer, sizeof(buffer), "%d", value);
    ret = write(fd, buffer, strlen(buffer));
    if (ret < 0)
    {
        perror("Error writing sysfs value");
        close(fd);
        return -1;
    }

    close(fd);
    printf("[INFO] Wrote %d to %s\n", value, path);
    return 0;
}

/**
 * Escreve texto no LCD via dispositivo de caractere.
 */
int write_text(const char *text)
{
    int fd;
    ssize_t ret;

    fd = open(DEV_PATH, O_WRONLY);
    if (fd < 0)
    {
        perror("Error opening /dev/hd44780_driver");
        return -1;
    }

    ret = write(fd, text, strlen(text));
    if (ret < 0)
    {
        perror("Error writing to LCD");
        close(fd);
        return -1;
    }

    printf("[INFO] Wrote text to LCD: \"%s\"\n", text);

    close(fd);
    return 0;
}

/**
 * Limpa o display.
 */
void clear_display(void)
{
    write_sysfs("lcd_clear_flag", 1);
    usleep(100000);
}

/**
 * Posiciona o cursor (linha, coluna).
 */
void set_cursor(int row, int col)
{
    write_sysfs("lcd_row", row);
    write_sysfs("lcd_col", col);
    usleep(50000);
}

/**
 * Exemplo principal de uso.
 */
int main(void)
{
    printf("\n--- HD44780 LCD Control (C Version) ---\n");

    /* 1. Limpa o display */
    clear_display();

    /* 2. Escreve mensagem na primeira linha */
    set_cursor(0, 0);
    write_text("Hello, World!");

    /* 3. Escreve mensagem na segunda linha */
    set_cursor(1, 0);
    write_text("I2C LCD Active");

    /* 5. Aguarda e limpa novamente */
    sleep(3);
    clear_display();

    printf("[INFO] Demo complete.\n");
    return 0;
}
