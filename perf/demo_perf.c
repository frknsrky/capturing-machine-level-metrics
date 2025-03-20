#include <stdio.h>

void some_computation() {
    volatile int sum = 0;
    for (int i = 0; i < 1000000; i++) {
        sum += i;  // Simple computation to generate instructions and cache usage
    }
}

int main(){
        some_computation()
        return 0;
}