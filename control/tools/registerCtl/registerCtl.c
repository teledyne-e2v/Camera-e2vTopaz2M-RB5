#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <math.h>

#define E2V_NODE_PATH       "/sys/devices/platform/soc/ac50000.qcom,cci/ac50000.qcom,cci:qcom,cam-sensor3/e2v_node"

int hexStrToInt(char* hex_str)
{
    return strtoul(hex_str, NULL, 16);
}

void writeReg(int fd, int reg_addr, int reg_data)
{
    int            ret;
    char           buf[30] = {0};

    printf("reg_addr: 0x%x, reg_data: 0x%x\n", reg_addr, reg_data);

    sprintf(buf, "0x%02hx 0x%04hx", reg_addr, reg_data);
    printf("buf val: %s\n", buf);

    ret = write(fd, buf, strlen(buf)+1);
    printf("return write: %d\n", ret);
}

void readReg(int fd, int reg_addr)
{
    char buf[50] = {0};
    char read_buf[50] = {0};
    int ret;

    sprintf(buf, "0x%02hx 0x%04hx", reg_addr, 0x9999);

    ret = write(fd, buf, strlen(buf)+1);
    printf("return write: %d\n", ret);

    ret = lseek(fd, 0, SEEK_SET);
    printf("return lseek: %d\n", ret);
    //write() change the fd pos, need use lseek to make pos point to the start of the file

    ret = read(fd, read_buf, sizeof(read_buf));
    printf("return read: %d, buf: %s\n", ret, read_buf);
}

int main(int argc, char **argv)
{
    int fd;
    int reg_addr, reg_data;

    if(!strcmp(argv[1], "w"))
    {   
        if (argc < 4)
        {
            printf("Invalid input, usage: <%s w reg_addr reg_data>\n", argv[0]);
            return -1;
        }

        reg_addr = hexStrToInt(argv[2]);
        reg_data = hexStrToInt(argv[3]);

        fd = open(E2V_NODE_PATH, O_RDWR);

        if (fd < 0)
        {
            perror("open");
            return -1;
        }

        printf("Write register\n");
        writeReg(fd, reg_addr, reg_data);

        close(fd);
    }
    else if(!strcmp(argv[1], "r"))
    {
        if (argc < 3)
        {
            printf("Invalid input, usage: <%s r reg_addr>\n", argv[0]);
            return -1;
        }
        
        reg_addr = hexStrToInt(argv[2]);

        fd = open(E2V_NODE_PATH, O_RDWR);

        printf("Read register\n");
        readReg(fd, reg_addr);

        close(fd);
    }
    else
    {
        printf("Invalid input!\n");
        printf("Write operation: <%s w reg_addr reg_data>\n", argv[0]);
        printf("Read  operation: <%s r reg_addr>\n", argv[0]);
        return -1;
    }

    return 0;
}
