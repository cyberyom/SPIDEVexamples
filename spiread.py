import spidev
import argparse

def read_data_chunk(spi, address, length):
    # Command to read data (0x03 for SPI flash) followed by the 24-bit address
    read_command = [0x03, (address >> 16) & 0xFF, (address >> 8) & 0xFF, address & 0xFF]
    
    # Dummy bytes to read data
    read_command += [0x00] * length
    
    # Send read command and read the response
    response = spi.xfer2(read_command)
    
    # The response includes the sent command and address bytes, followed by the actual data
    data = response[4:]
    
    return data

def save_data_to_file(filename, data):
    with open(filename, 'wb') as file:
        file.write(bytearray(data))

def main():
    parser = argparse.ArgumentParser(description="Read data from SPI flash chip and save to a .bin file")
    parser.add_argument('-l', '--length', type=int, required=True, help='Specify the length of data to read (required)')
    parser.add_argument('-s', '--speed', type=int, default=5, help='Set the SPI speed in MHz (default is 5 MHz, max is 133 MHz)')
    parser.add_argument('-o', '--output', type=str, default='output.bin', help='Specify the output file name (default is "output.bin")')
    args = parser.parse_args()

    total_length = args.length
    spi_speed_mhz = args.speed
    output_file = args.output

    if total_length <= 0:
        raise ValueError("Invalid length specified")

    if spi_speed_mhz <= 0 or spi_speed_mhz > 133:
        raise ValueError("Invalid speed specified. Maximum is 133 MHz.")

    spi_speed_hz = spi_speed_mhz * 1000000

    # Open SPI bus
    spi = spidev.SpiDev()
    spi.open(0, 0)  # Open bus 0, device (chip select) 0
    spi.max_speed_hz = spi_speed_hz  # Set SPI speed (Hz)
    spi.mode = 0b00  # SPI mode (0, 1, 2, or 3)

    try:
        start_address = 0x000000  # Starting address to read from
        total_length = args.length
        all_data = []
        bytes_read = 0

        while total_length > 0:
            chunk_size = min(total_length, 4092)
            data = read_data_chunk(spi, start_address, chunk_size)
            all_data.extend(data)
            start_address += chunk_size
            total_length -= chunk_size
            bytes_read += chunk_size

            print(f"\rBytes read: {bytes_read} / {args.length}", end='')

        save_data_to_file(output_file, all_data)
        
        print(f"\nData successfully read and saved to '{output_file}'")
    finally:
        spi.close()  # Close the SPI connection

if __name__ == '__main__':
    main()
