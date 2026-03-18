#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>


int main(){
    int fd;
    fd = open("/dev/rev_char_file", O_RDWR);
    if(fd < 0){
        perror("open failed");
        return 1;
    }
    char s[] = "Hello Hi How Are You";
    int wr;
    wr = write(fd, s, strlen(s));
    if(wr < 0){
        perror("Write failed");
        return 2;
    }


    int rd;
    rd = read(fd, s, sizeof(s));
    if(rd < 0){
        perror("Read failed");
        return 3;
    }

    printf("%s\n",s);

    return 0;

}
