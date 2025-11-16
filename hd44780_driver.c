/*
 * hd44780_i2c.c
 * Driver HD44780 via I2C (PCF8574)
 *
 * Autor: Paulo Henrique Langone (base: Fernando Simplicio)
 *
 * - Sysfs callbacks (module_param_cb) para comandos: lcd_row, lcd_col, lcd_clear_flag
 * - char device (/dev/hd44780_driver) para escrita de texto via write()
 * - Envio via I2C com pulso de EN adequado
 *
 * Observações:
 * - Ajuste LCD_SLAVE_ADDRESS conforme seu adaptador (0x20 / 0x27 / 0x3F / etc.)
 * - Este código é um esqueleto pronto para testes e extensões.
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include <linux/i2c.h>
#include <linux/delay.h>
#include <linux/types.h>
#include <linux/string.h>
#include <linux/mutex.h>
#include <linux/stat.h>
#include <linux/err.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Paulo Henrique Langone");
MODULE_DESCRIPTION("Driver HD44780 com interface I2C (PCF8574)");
MODULE_VERSION("0.2.0");

/** -------------------------------------------------------------------------
 *  Configurações gerais
 *  ------------------------------------------------------------------------- */
#define DRIVER_NAME     "hd44780_driver"
#define DEVICE_NAME     "hd44780_driver"
#define DRIVER_CLASS    "hd44780_class"

#define I2C_BUS_NUMBER          1
#define I2C_BUS_AVAILABLE       1
#define LCD_SLAVE_ADDRESS       0x27

#define LCD_BACKLIGHT  (1 << 3)
#define LCD_ENABLE     (1 << 2)
#define LCD_RW         (1 << 1)
#define LCD_RS         (1 << 0)

/* Pequenos tempos de espera */
#define LCD_PULSE_US    50
#define LCD_POST_CMD_MS 2

/** -------------------------------------------------------------------------
 *  Contexto do driver
 *  ------------------------------------------------------------------------- */
static dev_t myDeviceNr;
static struct class *myClass;
static struct cdev myDevice;

/* Ponteiros para o cliente e adaptador I2C */
static struct i2c_adapter *lcd_i2c_adapter = NULL;
static struct i2c_client  *lcd_i2c_client  = NULL;

/* Estrutura de driver e dispositivo I2C */
static struct i2c_driver lcd_driver = {
    .driver = {
        .name  = DRIVER_NAME,
        .owner = THIS_MODULE
    }
};

/* Informações de identificação do dispositivo I2C */
static struct i2c_board_info lcd_i2c_board_info = {
    I2C_BOARD_INFO(DRIVER_NAME, LCD_SLAVE_ADDRESS)
};

/* Mutex para proteger escritas simultâneas no display */
static DEFINE_MUTEX(lcd_lock);

/* Parâmetros controlados via sysfs */
static int lcd_row = 0;
static int lcd_col = 0;
static int lcd_clear_flag = 0;

/* Protótipos de funções internas */
static void lcd_goto(u8 row, u8 col);
static void lcd_clear(void);
static void lcd_send_cmd(u8 cmd);
static void lcd_send_data(u8 data);
static void lcd_init_sequence(void);

/** -------------------------------------------------------------------------
 *  Funções de baixo nível (I2C -> PCF8574 -> HD44780)
 *  ------------------------------------------------------------------------- */

/**
 * Envia um nibble (4 bits) para o PCF8574 com o pulso de habilitação (EN).
 * @param nibble Nibble a ser enviado (0x0–0xF)
 * @param rs     Bit de seleção de registro (0 = comando, 1 = dado)
 */
static void lcd_send_nibble(u8 nibble, u8 rs)
{
    int ret;
    u8 rs_bit = rs ? LCD_RS : 0;
    u8 base = ((nibble & 0x0F) << 4) | LCD_BACKLIGHT | rs_bit;

    if (!lcd_i2c_client) {
        pr_err("lcd: i2c client not initialized\n");
        return;
    }

    /* Pulso EN = 1 */
    ret = i2c_smbus_write_byte(lcd_i2c_client, base | LCD_ENABLE);
    if (ret < 0) {
        pr_err("lcd: i2c write (EN=1) failed: %d\n", ret);
        return;
    }
    udelay(LCD_PULSE_US);

    /* Pulso EN = 0 */
    ret = i2c_smbus_write_byte(lcd_i2c_client, base & ~LCD_ENABLE);
    if (ret < 0) {
        pr_err("lcd: i2c write (EN=0) failed: %d\n", ret);
        return;
    }

    udelay(LCD_PULSE_US);
}

/**
 * Envia um byte completo em dois nibbles.
 * @param byte Valor de 8 bits
 * @param rs   Bit de controle (0 = comando, 1 = dado)
 */
static void lcd_send_byte(u8 byte, u8 rs)
{
    lcd_send_nibble((byte >> 4) & 0x0F, rs);
    lcd_send_nibble(byte & 0x0F, rs);
    udelay(50);
}

/** Envia comando ao LCD */
static void lcd_send_cmd(u8 cmd)
{
    lcd_send_byte(cmd, 0);
}

/** Envia dado (caractere) ao LCD */
static void lcd_send_data(u8 data)
{
    lcd_send_byte(data, 1);
}

/** Limpa o display e aplica pequeno atraso */
static void lcd_clear(void)
{
    lcd_send_cmd(0x01);
    msleep(LCD_POST_CMD_MS);
}

/**
 * Posiciona o cursor no display.
 * @param row Linha (0–3)
 * @param col Coluna (0–15)
 */
static void lcd_goto(u8 row, u8 col)
{
    static const u8 row_offsets[] = { 0x00, 0x40, 0x14, 0x54 };
    u8 addr;

    if (row > 3)
        row = 3;

    addr = row_offsets[row] + col;
    lcd_send_cmd(0x80 | addr);
}

/**
 * Sequência de inicialização do HD44780 em modo 4 bits via PCF8574.
 * Baseada na recomendação do datasheet.
 */
static void lcd_init_sequence(void)
{
    msleep(50); /* Delay inicial >40ms após power-on */

    /* Força modo 8 bits três vezes */
    lcd_send_nibble(0x03, 0);
    msleep(5);
    lcd_send_nibble(0x03, 0);
    udelay(150);
    lcd_send_nibble(0x03, 0);
    udelay(150);

    /* Muda para 4 bits */
    lcd_send_nibble(0x02, 0);
    msleep(2);

    /* Configuração padrão */
    lcd_send_cmd(0x28);  /* 4 bits, 2 linhas, 5x8 dots */
    lcd_send_cmd(0x0C);  /* Display ON, cursor OFF, blink OFF */
    lcd_send_cmd(0x06);  /* Incrementa cursor automaticamente */
    lcd_clear();
}

/** -------------------------------------------------------------------------
 *  Callback do module_param_cb (controle via sysfs)
 *  ------------------------------------------------------------------------- */
static int lcd_callback(const char *val, const struct kernel_param *kp)
{
    int ret = param_set_int(val, kp);
    if (ret)
        return ret;

    if (strcmp(kp->name, "lcd_row") == 0) {
        pr_info("lcd: row -> %d\n", lcd_row);
        lcd_goto(lcd_row, lcd_col);
    } else if (strcmp(kp->name, "lcd_col") == 0) {
        pr_info("lcd: col -> %d\n", lcd_col);
        lcd_goto(lcd_row, lcd_col);
    } else if (strcmp(kp->name, "lcd_clear_flag") == 0) {
        if (lcd_clear_flag) {
            pr_info("lcd: clear requested\n");
            lcd_clear();
            lcd_clear_flag = 0;
        }
    }
    return 0;
}

/* Estrutura de operações do parâmetro */
static const struct kernel_param_ops lcd_param_ops = {
    .set = &lcd_callback,
    .get = &param_get_int,
};

/* Parâmetros exportados via sysfs */
module_param_cb(lcd_row, &lcd_param_ops, &lcd_row, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(lcd_row, "Define a linha do cursor (0-3)");

module_param_cb(lcd_col, &lcd_param_ops, &lcd_col, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(lcd_col, "Define a coluna do cursor (0-15)");

module_param_cb(lcd_clear_flag, &lcd_param_ops, &lcd_clear_flag, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(lcd_clear_flag, "Set to 1 to clear display");

/** --------------------------------------------------------------------------
 *  Funções do char device
 *  -------------------------------------------------------------------------- */

/** Função open() - executada quando o dispositivo é aberto */
static int driver_open(struct inode *deviceFile, struct file *instance)
{
    printk(KERN_INFO "hd44780_driver: device opened\n");
    return 0;
}

/** Função release() - executada quando o dispositivo é fechado */
static int driver_close(struct inode *deviceFile, struct file *instance)
{
    printk(KERN_INFO "hd44780_driver: device closed\n");
    return 0;
}

/**
 * Função write() - escreve caracteres no LCD.
 * Cada caractere recebido é enviado como dado ao display.
 */
static ssize_t driver_write(struct file *File, const char *user_buffer, size_t count, loff_t *offs)
{
    if (!mutex_trylock(&lcd_lock))
        return -EBUSY;

    char buffer[64];
    int to_copy, not_copied, delta;

    /* Calcula quantidade de bytes a copiar */
    to_copy = min(sizeof(buffer), count);

    /* Copia dados do espaço do usuário */
    not_copied = copy_from_user(buffer, user_buffer, to_copy);

    for (int i = 0; i < to_copy; i++) {
        if (buffer[i] == '\n') {
            lcd_send_cmd(0xC0); /* Segunda linha */
        } else {
            lcd_send_data(buffer[i]);
        }
    }

    delta = to_copy - not_copied;
    mutex_unlock(&lcd_lock);
    return delta;
}

/* Estrutura de operações de arquivo */
static struct file_operations fops = {
    .owner   = THIS_MODULE,
    .open    = driver_open,
    .release = driver_close,
    .write   = driver_write
};

/** --------------------------------------------------------------------------
 *  Funções de inicialização e finalização do módulo
 *  -------------------------------------------------------------------------- */

static int __init lcd_module_init(void)
{
    int ret = -1;

    printk(KERN_INFO "Starting HD44780 driver...\n");

    if (alloc_chrdev_region(&myDeviceNr, 0, 1, DRIVER_NAME) < 0) {
        printk(KERN_ERR "Error allocating device number!\n");
        return -1;
    }

    if ((myClass = class_create(DRIVER_CLASS)) == NULL) {
        printk(KERN_ERR "Error creating device class!\n");
        goto ClassError;
    }

    if (device_create(myClass, NULL, myDeviceNr, NULL, DRIVER_NAME) == NULL) {
        printk(KERN_ERR "Error creating device file!\n");
        goto FileError;
    }

    cdev_init(&myDevice, &fops);
    if (cdev_add(&myDevice, myDeviceNr, 1) == -1) {
        printk(KERN_ERR "Error registering device with kernel!\n");
        goto KernelError;
    }

    lcd_i2c_adapter = i2c_get_adapter(I2C_BUS_AVAILABLE);
    if (lcd_i2c_adapter != NULL) {
        lcd_i2c_client = i2c_new_client_device(lcd_i2c_adapter, &lcd_i2c_board_info);
        if (lcd_i2c_client != NULL) {
            if (i2c_add_driver(&lcd_driver) != -1) {
                ret = 0;
                printk(KERN_INFO "I2C driver registered successfully.\n");
            } else {
                printk(KERN_ERR "Failed to add I2C driver.\n");
            }
        }
        i2c_put_adapter(lcd_i2c_adapter);
    }

    lcd_init_sequence();
    printk(KERN_INFO "HD44780 driver loaded successfully.\n");
    return ret;

KernelError:
    device_destroy(myClass, myDeviceNr);
FileError:
    class_destroy(myClass);
ClassError:
    unregister_chrdev_region(myDeviceNr, 1);
    return (-1);
}

/** Função executada ao remover o módulo */
static void __exit lcd_module_exit(void)
{
    if (lcd_i2c_client)
        i2c_unregister_device(lcd_i2c_client);

    i2c_del_driver(&lcd_driver);
    cdev_del(&myDevice);
    device_destroy(myClass, myDeviceNr);
    class_destroy(myClass);
    unregister_chrdev_region(myDeviceNr, 1);

    printk(KERN_INFO "HD44780 driver unloaded from kernel.\n");
}

/** --------------------------------------------------------------------------
 *  Macros de inicialização e finalização
 *  -------------------------------------------------------------------------- */
module_init(lcd_module_init);
module_exit(lcd_module_exit);
