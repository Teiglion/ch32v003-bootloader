#define _DEFAULT_SOURCE

#include <stdio.h>
#include <termios.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdint.h>
#include <poll.h>

#define BEGIN 1u
#define DATA  2u
#define END   3u

#define READY 1u
#define ACK   2u
#define DONE  3u

#define READY_TIMEOUT_MS 3000

#define FLASH_CAPACITY 16384u
#define CRC32_INIT 0xFFFFFFFFu
#define CRC32_POLY 0xEDB88320u

static int open_port(const char *port, speed_t baudrate)
{
    int fd = open(port, O_RDWR | O_NOCTTY | O_SYNC);
    if (fd<0) 
    {
        printf("Port open error.\n");
        return -1;
    }

    struct termios tty;
    if (tcgetattr(fd, &tty) != 0) 
    {
        printf("Can't get port attributes\n");
        close(fd);
        return -1;
    }
    if (cfsetospeed(&tty, baudrate) != 0 || cfsetispeed(&tty, baudrate) != 0)
    {
        printf("Can't set port speed\n");
        close(fd);
        return -1;
    }

    tty.c_cflag = (tty.c_cflag & ~CSIZE) | CS8;
    tty.c_cflag &= ~(PARENB | CSTOPB | CRTSCTS);   
    tty.c_cflag |= CREAD | CLOCAL; 
    
    tty.c_lflag &= ~(ICANON | ECHO | ECHOE | ECHONL | ISIG | IEXTEN);

    tty.c_iflag &= ~(IXON | IXOFF | IXANY | IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR | ICRNL | INPCK);
    tty.c_oflag &= ~OPOST; 

    tty.c_cc[VMIN]  = 1;
    tty.c_cc[VTIME] = 0; 

    if (tcsetattr(fd, TCSANOW, &tty) != 0) 
    {
        printf("Can't set port attributes\n");
        close(fd);
        return -1;
    }
    return fd;
}

static uint32_t crc32_update(uint32_t crc, const uint8_t *data, uint32_t size)
{
    for (uint32_t i = 0; i < size; i++)
    {
        crc ^= (uint32_t)data[i];

        for (uint8_t bit = 0; bit < 8u; bit++)
        {
            if ((crc & 1u) != 0u) crc = (crc >> 1) ^ CRC32_POLY;
            else crc >>= 1;
        }
    }

    return crc;
}

static uint32_t crc32_finish(uint32_t crc) {return crc ^ 0xFFFFFFFFu;}

static int write_all(int fd, const uint8_t *data, size_t size)
{
    size_t sent = 0;
    while (sent < size)
    {
        ssize_t n = write(fd, data + sent, size - sent);
        if (n <= 0) return 0;
        sent += (size_t)n;
    }

    return 1;
}

static int read_all(int fd, uint8_t *data, size_t size, int timeout_ms)
{
    size_t received = 0;
    while (received < size)
    {
        struct pollfd pfd = 
        {
            .fd = fd,
            .events = POLLIN
        };

        int result = poll(&pfd, 1, timeout_ms);
        if (result == 0) 
        {
            printf("RX timeout: %zu/%zu bytes\n", received, size);
            return 0;
        }
        if (result < 0) return -1;

        if ((pfd.revents & (POLLERR | POLLHUP | POLLNVAL)) != 0) return -1;
        if ((pfd.revents & POLLIN) == 0) return -1;

        ssize_t n = read(fd, data + received, size - received);
        if (n <= 0) return -1;
        printf("RX:");
        for (ssize_t i = 0; i < n; i++)
        {
            printf(" %02X", (unsigned int)data[received + (size_t)i]);
        }
        printf("\n");
        received += (size_t)n;
    }

    return 1;
}

static int upload_firmware(int fd, FILE *file)
{
    if (fseek(file, 0L, SEEK_END) != 0)
    {
        printf("Can't seek firmware\n");
        return 1;
    }

    long file_size = ftell(file);
    if (file_size < 0)
    {
        printf("Can't get firmware size\n");
        return 1;
    }

    if (file_size == 0 || file_size > (long)FLASH_CAPACITY)
    {
        printf("Wrong firmware size: %ld bytes\n", file_size);
        return 1;
    }

    uint32_t image_size = (uint32_t)file_size;

    if (fseek(file, 0L, SEEK_SET) != 0)
    {
        printf("Can't rewind firmware\n");
        return 1;
    }

    printf("Firmware size: %lu bytes\n", (unsigned long)image_size);

    uint8_t buf[64];
    uint32_t remaining = image_size;
    uint32_t host_crc = CRC32_INIT;

    while (remaining != 0u)
    {
        uint32_t size = remaining > sizeof(buf) ? (uint32_t)sizeof(buf) : remaining;

        if (fread(buf, 1u, size, file) != size)
        {
            printf("Can't read firmware\n");
            return 1;
        }

        host_crc = crc32_update(host_crc, buf, size);
        remaining -= size;
    }
    host_crc = crc32_finish(host_crc);
    printf("Firmware CRC: %08lX\n", (unsigned long)host_crc);

    if (fseek(file, 0L, SEEK_SET) != 0)
    {
        printf("Can't rewind firmware\n");
        return 1;
    }

    uint8_t header[9];
    header[0] = BEGIN;

    for (uint8_t i = 0; i < 4u; i++)
    {
        header[1u + i] = (uint8_t)(image_size >> (8u * i));
        header[5u + i] = (uint8_t)(host_crc >> (8u * i));
    }

    if (tcflush(fd, TCIFLUSH) != 0)
    {
        printf("Can't clear port input\n");
        return 1;
    }

    if (!write_all(fd, header, sizeof(header)))
    {
        printf("Can't send firmware header\n");
        return 1;
    }

    uint8_t response[3];
    int result = read_all(fd, response, sizeof(response), READY_TIMEOUT_MS);

    if (result != 1)
    {
        if (result == 0) printf("NOT READY\n");
        else printf("Can't receive device response\n");
        return 1;
    }

    uint16_t response_page = (uint16_t)response[1];
    response_page |= (uint16_t)response[2] << 8;

    if (response[0] != READY || response_page != 0u)
    {
        printf("Device refused firmware: status=%u, page=%u\n", (unsigned int)response[0], (unsigned int)response_page);
        return 1;
    }
    printf("READY\n");

    uint8_t command = DATA;
    if (!write_all(fd, &command, 1u))
    {
        printf("Can't send DATA\n");
        return 1;
    }

    uint32_t full_pages = image_size / (uint32_t)sizeof(buf);
    uint32_t tail_size = image_size % (uint32_t)sizeof(buf);
    uint32_t pages_count = full_pages + (uint32_t)(tail_size != 0u);

    for (uint32_t page = 0; page < pages_count; page++)
    {
        uint32_t size = page < full_pages ? (uint32_t)sizeof(buf) : tail_size;

        if (fread(buf, 1u, size, file) != size)
        {
            printf("Can't read firmware\n");
            return 1;
        }

        uint32_t packet_crc = crc32_update(CRC32_INIT, buf, size);
        packet_crc = crc32_finish(packet_crc);

        uint8_t number[2] ={(uint8_t)page, (uint8_t)(page >> 8)};

        uint8_t crc_bytes[4];
        for (uint8_t i = 0; i < 4u; i++) crc_bytes[i] = (uint8_t)(packet_crc >> (8u * i));

        if (!write_all(fd, number, sizeof(number)) || !write_all(fd, buf, size) || !write_all(fd, crc_bytes, sizeof(crc_bytes)))
        {
            printf("Can't send page %u\n", (unsigned int)page);
            return 1;
        }

        result = read_all(fd, response, sizeof(response), READY_TIMEOUT_MS);
        if (result != 1)
        {
            if (result == 0) printf("ACK timeout for page %u\n", (unsigned int)page);
            else printf("Can't receive page response\n");
            return 1;
        }

        response_page = (uint16_t)response[1];
        response_page |= (uint16_t)response[2] << 8;

        if (response[0] != ACK || response_page != page)
        {
            printf("Page %u failed: status=%u, response page=%u\n", (unsigned int)page, (unsigned int)response[0], (unsigned int)response_page);
            return 1;
        }
    }

    printf("All pages sent\n");

    command = END;
    if (!write_all(fd, &command, 1u))
    {
        printf("Can't send END\n");
        return 1;
    }

    result = read_all(fd, response, sizeof(response), READY_TIMEOUT_MS);

    if (result != 1)
    {
        if (result == 0) printf("DONE timeout\n");
        else printf("Can't receive final response\n");

        return 1;
    }

    response_page = (uint16_t)response[1];
    response_page |= (uint16_t)response[2] << 8;

    if (response[0] != DONE || response_page != 0u)
    {
        printf("Flashing failed: status=%u, page=%u\n", (unsigned int)response[0], (unsigned int)response_page);
        return 1;
    }

    printf("Flashing successful\n");
    return 0;
}

int main(int argc, const char *argv[])
{
    if(argc !=3)
    {
        printf("Usage: %s <port> <firmware.bin>\n",argv[0]);
        return 1;
    }

    FILE *f = fopen(argv[2],"rb");
    if(!f)
    {
        printf("Can't open firmware\n");
        return 1;
    }

    int fd = open_port(argv[1], B115200);
    if (fd < 0)
    {
        fclose(f);
        return 1;
    }

    int result = upload_firmware(fd, f);

    fclose(f);
    close(fd);

    return result;
}