import spidev
import time

# Open SPI bus
spi = spidev.SpiDev()
spi.open(0, 0)  # Open bus 0, device (chip select) 0
spi.max_speed_hz = 50000  # Set SPI speed (Hz)
spi.mode = 0b00  # SPI mode (0, 1, 2, or 3)

def read_manufacturer_id():
    # Command to read manufacturer ID (usually 0x90 or 0x9F)
    read_id_command = [0x9F]

    # Sending read ID command followed by three dummy bytes
    response = spi.xfer2(read_id_command + [0x00, 0x00, 0x00])

    # The response should now contain the manufacturer ID and device ID
    manufacturer_id = response[1]
    memory_type = response[2]
    memory_capacity = response[3]

    return manufacturer_id, memory_type, memory_capacity

def interpret_memory_capacity(capacity_code):
    capacity_lookup = {
        0x14: "8 Mbit (1 MByte)",
        0x15: "16 Mbit (2 MBytes)",
        0x16: "32 Mbit (4 MBytes)",
        0x17: "64 Mbit (8 MBytes)",
        0x18: "128 Mbit (16 MBytes)"
        # Add more entries based on the datasheet of your chip
    }
    return capacity_lookup.get(capacity_code, "Unknown capacity")

try:
    manufacturer_id, memory_type, memory_capacity = read_manufacturer_id()
    print("Manufacturer ID: {:#x}".format(manufacturer_id))
    print("Memory Type: {:#x}".format(memory_type))
    print("Memory Capacity: {:#x} ({})".format(memory_capacity, interpret_memory_capacity(memory_capacity)))
finally:
    spi.close()  # Close the SPI connection
