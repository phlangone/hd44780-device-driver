#!/usr/bin/env python3
# -----------------------------------------------------------------------------
# LCD Control Script (HD44780 via I2C)
# Autor: Paulo Henrique Langone
# Descrição: Script de controle para o driver hd44780_driver
# -----------------------------------------------------------------------------
# - Permite escrever texto no display via /dev/hd44780_driver
# - Controla linha, coluna e limpeza via sysfs
# -----------------------------------------------------------------------------

import os
import time

# Caminhos do sysfs e do device
SYSFS_BASE = "/sys/module/hd44780_driver/parameters"
DEV_PATH = "/dev/hd44780_driver"

# -----------------------------------------------------------------------------
# Funções auxiliares
# -----------------------------------------------------------------------------

def write_sysfs(param, value):
    """
    Escreve um valor em um parâmetro sysfs do driver.
    """
    path = f"{SYSFS_BASE}/{param}"
    try:
        with open(path, "w") as f:
            f.write(str(value))
        print(f"[INFO] Wrote '{value}' to {path}")
    except Exception as e:
        print(f"[ERROR] Failed to write '{param}': {e}")

def write_text(text):
    """
    Envia texto para o display via device file.
    """
    try:
        with open(DEV_PATH, "w") as f:
            f.write(text)
        print(f"[INFO] Wrote text to LCD: '{text}'")
    except Exception as e:
        print(f"[ERROR] Failed to write text: {e}")

def clear_display():
    """
    Limpa o display via flag de controle.
    """
    write_sysfs("lcd_clear_flag", 1)
    time.sleep(0.1)

def set_cursor(row, col):
    """
    Posiciona o cursor (linha, coluna).
    """
    write_sysfs("lcd_row", row)
    write_sysfs("lcd_col", col)
    time.sleep(0.05)

# -----------------------------------------------------------------------------
# Exemplo de uso
# -----------------------------------------------------------------------------
if __name__ == "__main__":
    print("\n--- HD44780 LCD Control Script ---\n")

    # Limpa o display
    clear_display()

    # Escreve uma mensagem
    set_cursor(0, 0)
    write_text("Hello, World!")

    # Muda de linha
    set_cursor(1, 0)
    write_text("I2C LCD Active")

    # Aguarda e limpa novamente
    time.sleep(3)
    clear_display()

    print("\n[INFO] Demo complete.\n")
