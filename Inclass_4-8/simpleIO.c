#include<stdio.h>


void NeverExecutes()
{
    printf("Hacked!!! This function was not supposed to run!\n");
}

void GetInput()
{
    char buffer[8];

    printf("Enter a string\n");
    gets(buffer);
    puts(buffer);
}


int main()
{
    printf("address of GetInput: %p\n", GetInput);
    GetInput();
    return 0;
}