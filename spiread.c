#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/spi/spidev.h>
#include <string.h>
#include <stdint.h>  // Include this header for uint32_t and uint8_t
#include <getopt.h>

#define SPI_PATH "/dev/spidev0.0"
#define MAX_CHUNK_SIZE 4092
#define DEFAULT_SPI_SPEED 5  // Default SPI speed is 5 MHz

// Function to read a chunk of data from the SPI flash chip
int read_data_chunk(int fd, uint32_t address, uint8_t *buffer, size_t length, uint32_t speed_mhz) {
    struct spi_ioc_transfer transfer;
    uint8_t tx[MAX_CHUNK_SIZE + 4] = {0};
    uint8_t rx[MAX_CHUNK_SIZE + 4] = {0};

    // Convert speed from MHz to Hz
    uint32_t speed_hz = speed_mhz * 1000000;

    // Command to read data (0x03 for SPI flash) followed by the 24-bit address
    tx[0] = 0x03;
    tx[1] = (address >> 16) & 0xFF;
    tx[2] = (address >> 8) & 0xFF;
    tx[3] = address & 0xFF;

    memset(&transfer, 0, sizeof(transfer));
    transfer.tx_buf = (unsigned long)tx;
    transfer.rx_buf = (unsigned long)rx;
    transfer.len = length + 4;
    transfer.speed_hz = speed_hz;  // Set the SPI speed
    transfer.bits_per_word = 8;

    if (ioctl(fd, SPI_IOC_MESSAGE(1), &transfer) < 0) {
        perror("Failed to send SPI message");
        return -1;
    }

    memcpy(buffer, &rx[4], length);
    return 0;
}

// Function to save data to a .bin file
int save_data_to_file(const char *filename, uint8_t *data, size_t length) {
    FILE *file = fopen(filename, "wb");
    if (!file) {
        perror("Failed to open file");
        return -1;
    }
    fwrite(data, 1, length, file);
    fclose(file);
    return 0;
}

void print_usage(const char *prog_name) {
    fprintf(stderr, "Usage: %s -l <length> [-s <speed>] [-o <output file>]\n", prog_name);
    fprintf(stderr, "Read data from SPI flash chip and save to a .bin file\n");
    fprintf(stderr, "Options:\n");
    fprintf(stderr, "  -l <length>        Specify the length of data to read (required)\n");
    fprintf(stderr, "  -s <speed>         Set the SPI speed in MHz (default is 5 MHz, max is 133 MHz)\n");
    fprintf(stderr, "  -o <output file>   Specify the output file name (default is 'output.bin')\n");
    fprintf(stderr, "  -h                 Display this help message and exit\n");
}

int main(int argc, char *argv[]) {
    int opt;
    size_t total_length = 0;
    uint32_t spi_speed = DEFAULT_SPI_SPEED;
    char output_file[256] = "output.bin";  // Default output file name

    while ((opt = getopt(argc, argv, "l:s:o:h")) != -1) {
        switch (opt) {
            case 'l':
                total_length = atoi(optarg);
                if (total_length <= 0) {
                    fprintf(stderr, "Invalid length specified\n");
                    return 1;
                }
                break;
            case 's':
                spi_speed = atoi(optarg);
                if (spi_speed <= 0 || spi_speed > 133) {
                    fprintf(stderr, "Invalid speed specified. Maximum is 133 MHz.\n");
                    return 1;
                }
                break;
            case 'o':
                strncpy(output_file, optarg, sizeof(output_file) - 1);
                output_file[sizeof(output_file) - 1] = '\0';  // Ensure null-terminated string
                break;
            case 'h':
            default:
                print_usage(argv[0]);
                return 1;
        }
    }

    if (total_length == 0) {
        print_usage(argv[0]);
        return 1;
    }

    int fd = open(SPI_PATH, O_RDWR);
    if (fd < 0) {
        perror("Failed to open SPI device");
        return 1;
    }

    uint8_t *all_data = malloc(total_length);
    if (!all_data) {
        perror("Failed to allocate memory");
        close(fd);
        return 1;
    }

    uint32_t start_address = 0x000000;
    size_t remaining_length = total_length;
    size_t chunk_size;
    uint8_t buffer[MAX_CHUNK_SIZE];
    size_t bytes_read = 0;

    while (remaining_length > 0) {
        chunk_size = (remaining_length < MAX_CHUNK_SIZE) ? remaining_length : MAX_CHUNK_SIZE;
        if (read_data_chunk(fd, start_address, buffer, chunk_size, spi_speed) < 0) {
            free(all_data);
            close(fd);
            return 1;
        }
        memcpy(all_data + bytes_read, buffer, chunk_size);
        start_address += chunk_size;
        remaining_length -= chunk_size;
        bytes_read += chunk_size;

        // Print the progress
        printf("\rBytes read: %zu / %zu", bytes_read, total_length);
        fflush(stdout);  // Flush the output buffer to ensure the line is updated
    }

    if (save_data_to_file(output_file, all_data, total_length) < 0) {
        free(all_data);
        close(fd);
        return 1;
    }

    printf("\nData successfully read and saved to '%s'\n", output_file);

    free(all_data);
    close(fd);
    return 0;
}
