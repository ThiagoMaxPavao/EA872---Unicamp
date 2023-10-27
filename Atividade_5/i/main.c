#include<stdio.h>
#include<stdlib.h>

extern int get(char* webspace, char* resource);

int main(int argc, char *argv[]) {
    int status;

    if(argc != 2) {
        printf("Uso: %s <subdiretorio>", argv[0]);
        exit(1);
    }

    status = get("/home/EC21/ra247381/webspace", argv[1]);
    printf("Status: %d\n", status);

    return 0;
}
