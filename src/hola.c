#include <stdio.h>

int main(){
    printf("Hola Sistemas Operativos I\n");

    int *puntero;
    int numero = 10;
    puntero = &numero;
    printf("Numero: %d\n", numero);
    printf("*puntero: %d\n", *puntero);
    printf("y la direccion de memoria es: %d\n", puntero);
    char i = 'I';

    printf("Hola Sistemas Operativos " + i );
    return 0;
}