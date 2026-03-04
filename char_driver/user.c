#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

static char* user_string = "Hello from user.c";
char last_byte;

int main(int argc, char* argv[]){
    int fd, wr, rd;
    fd = open("/dev/char0", O_RDWR);
    if(fd < 0){
        perror("Open failed");
        return 1;
    }

    wr = write(fd, user_string, strlen(user_string));

    if(wr < 0){
        perror("write failed");
        return wr;
    }

    lseek(fd, 0, SEEK_SET);

    rd = read(fd, &last_byte, 1);
    if(rd < 0){
        perror("read failed");
        return rd;
    }

    printf("Last byte from kernel to user is %c\n",last_byte);

    return 0;
}

/*Use sudo ./a.out to run program 
,as we dont want to change permissions manually*/