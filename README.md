HD44780_DRIVER
=============

Descrição
---------
Este projeto contém um driver de kernel (módulo) para controlar displays de caracteres baseados no controlador HD44780 a partir de uma Raspberry Pi. O driver expõe:
- Um dispositivo de caractere em /dev/hd44780_driver para envio direto de texto.
- Parâmetros via sysfs para posicionamento do cursor e limpeza do display:
    /sys/module/hd44780_driver/parameters/lcd_row
    /sys/module/hd44780_driver/parameters/lcd_col
    /sys/module/hd44780_driver/parameters/lcd_clear_flag

Requisitos
----------
- Raspberry Pi com Linux (kernel compatível com os headers instalados).
- I2C ou GPIO habilitado conforme o hardware de interface entre a Pi e o display.
- Toolchain e make (build-essential, gcc etc.).

Estrutura do projeto
--------------------
Exemplo de layout:
./drivers/hd44780_driver.c
./drivers/Makefile
./drivers/README.txt    <- este arquivo

Instalação dos headers do kernel
--------------------------------
Atualize o sistema e instale os headers do kernel antes de compilar:

sudo apt update
sudo apt install raspberrypi-kernel-headers build-essential
sudo apt upgrade -y

Habilitar I2C (se aplicável)
----------------------------
Habilite I2C via raspi-config (se o seu hardware usa I2C):

sudo raspi-config

Reinicie a Raspberry Pi para aplicar alterações:

sudo shutdown -r now

Compilação
----------
Navegue até o diretório do projeto e compile:

cd ~/drivers
make all

Após compilação o módulo é gerado:
- hd44780_driver.ko

Carregar o driver
-----------------
Carregue o módulo manualmente (após reiniciar se necessário):

sudo insmod hd44780_driver.ko

Verifique se está carregado:

lsmod | grep hd44780_driver
dmesg | tail -n 20

Permissões de acesso
--------------------
Para permitir acesso ao sysfs e ao dispositivo sem sudo, ajuste permissões (exemplo):

sudo chmod 666 /sys/module/hd44780_driver/parameters/lcd_row
sudo chmod 666 /sys/module/hd44780_driver/parameters/lcd_col
sudo chmod 666 /sys/module/hd44780_driver/parameters/lcd_clear_flag
sudo chmod 666 /dev/hd44780_driver

Uso (com exemplos)
------------------
- Definir linha do cursor (0–3):
    echo 1 > /sys/module/hd44780_driver/parameters/lcd_row

- Definir coluna do cursor (0–15):
    echo 5 > /sys/module/hd44780_driver/parameters/lcd_col

- Limpar o display (flag é resetada automaticamente pelo driver):
    echo 1 > /sys/module/hd44780_driver/parameters/lcd_clear_flag

- Enviar texto para o display:
    echo "Hello World!" > /dev/hd44780_driver

Remoção do driver
-----------------
Para descarregar o módulo:

sudo rmmod hd44780_driver
lsmod | grep hd44780_driver

Depuração
---------
- Verifique mensagens do kernel: dmesg | tail -n 50
- Confirme presença de /dev/hd44780_driver e permissões com ls -l /dev/hd44780_driver
- Confirme a existência dos parâmetros em /sys/module/hd44780_driver/parameters/

Boas práticas e notas finais
----------------------------
- Faça backup de arquivos importantes antes de carregar módulos personalizados.
- Se o display não responde, verifique conexões físicas e endereço/linha I2C (se aplicável).
- Ajuste o driver conforme o hardware de interface (GPIO direto, PCA8574/PCF8574 I2C expander, etc.).

Licença
-------
MIT