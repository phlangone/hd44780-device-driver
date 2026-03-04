# HD44780 Driver

## Description

This project provides a **Linux kernel driver (kernel module)** to control **HD44780-based character LCD displays** using a **Raspberry Pi**.

The driver exposes:

- A **character device** at `/dev/hd44780_driver` for direct text output.
- **Sysfs parameters** to control cursor positioning and display clearing:

```
/sys/module/hd44780_driver/parameters/lcd_row
/sys/module/hd44780_driver/parameters/lcd_col
/sys/module/hd44780_driver/parameters/lcd_clear_flag
```

---

## Requirements

- Raspberry Pi running Linux (kernel compatible with the installed headers)
- I2C or GPIO enabled depending on the hardware interface between the Pi and the display
- Build tools and toolchain (`build-essential`, `gcc`, `make`, etc.)

---

## Project Structure

Example layout:

```
./drivers/hd44780_driver.c
./drivers/Makefile
./drivers/README.md
```

---

## Installing Kernel Headers

Update the system and install the kernel headers before compiling:

```bash
sudo apt update
sudo apt install raspberrypi-kernel-headers build-essential
sudo apt upgrade -y
```

---

## Enable I2C (if applicable)

Enable I2C through **raspi-config** if your hardware interface uses I2C:

```bash
sudo raspi-config
```

Reboot the Raspberry Pi to apply the changes:

```bash
sudo shutdown -r now
```

---

## Compilation

Navigate to the project directory and compile the driver:

```bash
cd ~/drivers
make all
```

After compilation, the kernel module will be generated:

```
hd44780_driver.ko
```

---

## Loading the Driver

Load the kernel module manually:

```bash
sudo insmod hd44780_driver.ko
```

Verify that it was successfully loaded:

```bash
lsmod | grep hd44780_driver
dmesg | tail -n 20
```

---

## Access Permissions

To allow access to the sysfs parameters and device without using `sudo`, adjust the permissions:

```bash
sudo chmod 666 /sys/module/hd44780_driver/parameters/lcd_row
sudo chmod 666 /sys/module/hd44780_driver/parameters/lcd_col
sudo chmod 666 /sys/module/hd44780_driver/parameters/lcd_clear_flag
sudo chmod 666 /dev/hd44780_driver
```

---

## Usage Examples

### Set cursor row (0–3)

```bash
echo 1 > /sys/module/hd44780_driver/parameters/lcd_row
```

### Set cursor column (0–15)

```bash
echo 5 > /sys/module/hd44780_driver/parameters/lcd_col
```

### Clear the display

(The flag is automatically reset by the driver)

```bash
echo 1 > /sys/module/hd44780_driver/parameters/lcd_clear_flag
```

### Send text to the display

```bash
echo "Hello World!" > /dev/hd44780_driver
```

---

## Removing the Driver

Unload the kernel module:

```bash
sudo rmmod hd44780_driver
```

Verify removal:

```bash
lsmod | grep hd44780_driver
```

---

## Debugging

Check kernel messages:

```bash
dmesg | tail -n 50
```

Verify the device:

```bash
ls -l /dev/hd44780_driver
```

Confirm the sysfs parameters:

```
/sys/module/hd44780_driver/parameters/
```

---

## Best Practices and Notes

- Backup important files before loading custom kernel modules.
- If the display does not respond, verify **physical wiring and I2C address/bus configuration** (if applicable).
- Adjust the driver depending on the hardware interface used:
  - Direct GPIO control
  - I2C expanders such as **PCA8574 / PCF8574**

---

## License

MIT
