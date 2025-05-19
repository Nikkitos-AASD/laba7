#include <stdio.h>

const int LIMIT = 100;

int check(int x) {
    if (x < LIMIT)
        return x * 2;
    return x + LIMIT;
}

int main(void) {
    int value  = 42;
    int result = check(value);
    printf("Result = %d\n", result);
    return 0;
}
