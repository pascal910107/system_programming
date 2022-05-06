#include <stdio.h>
#include <string.h>

int main(){
    unsigned a,b;
    a = 0x1000;
    int c = 'A';
    char str[] = "";
    // strcat(str, c);
    sprintf(str, "%06x", a);
    printf("%s", str); 
}