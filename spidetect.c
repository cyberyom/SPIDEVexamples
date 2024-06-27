#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/spi/spidev.h>
#include <string.h>
#include <stdint.h>

#define SPI_PATH "/dev/spidev0.0"
#define MAX_SPEED_HZ 50000
#define SPI_MODE 0

// Function to read manufacturer ID
int read_manufacturer_id(int fd, uint8_t *manufacturer_id, uint8_t *memory_type, uint8_t *memory_capacity) {
    uint8_t tx[] = {0x9F, 0x00, 0x00, 0x00}; // Read ID command followed by three dummy bytes
    uint8_t rx[4] = {0};

    struct spi_ioc_transfer transfer = {
        .tx_buf = (unsigned long)tx,
        .rx_buf = (unsigned long)rx,
        .len = sizeof(tx),
        .speed_hz = MAX_SPEED_HZ,
        .bits_per_word = 8,
    };

    if (ioctl(fd, SPI_IOC_MESSAGE(1), &transfer) < 0) {
        perror("Failed to send SPI message");
        return -1;
    }

    *manufacturer_id = rx[1];
    *memory_type = rx[2];
    *memory_capacity = rx[3];

    return 0;
}

// Function to interpret memory capacity
const char* interpret_memory_capacity(uint8_t capacity_code) {
    switch (capacity_code) {
        case 0x14: return "8 Mbit (1 MByte)";
        case 0x15: return "16 Mbit (2 MBytes)";
        case 0x16: return "32 Mbit (4 MBytes)";
        case 0x17: return "64 Mbit (8 MBytes)";
        case 0x18: return "128 Mbit (16 MBytes)";
        default: return "Unknown capacity";
    }
}

int main() {
    int fd = open(SPI_PATH, O_RDWR);
    if (fd < 0) {
        perror("Failed to open SPI device");
        return 1;
    }

    uint8_t mode = SPI_MODE;
    uint32_t speed = MAX_SPEED_HZ;

    if (ioctl(fd, SPI_IOC_WR_MODE, &mode) < 0) {
        perror("Failed to set SPI mode");
        close(fd);
        return 1;
    }

    if (ioctl(fd, SPI_IOC_WR_MAX_SPEED_HZ, &speed) < 0) {
        perror("Failed to set SPI speed");
        close(fd);
        return 1;
    }

    uint8_t manufacturer_id, memory_type, memory_capacity;

    if (read_manufacturer_id(fd, &manufacturer_id, &memory_type, &memory_capacity) == 0) {
        printf("Manufacturer ID: 0x%02x\n", manufacturer_id);
        printf("Memory Type: 0x%02x\n", memory_type);
        printf("Memory Capacity: 0x%02x (%s)\n", memory_capacity, interpret_memory_capacity(memory_capacity));
    }

    close(fd);
    return 0;
}
