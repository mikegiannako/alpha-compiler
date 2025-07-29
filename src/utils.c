#include <stdio.h>
#include <stdlib.h>

#include "../include/utils.h"

unsigned int countDigits(unsigned int n){
    unsigned int count = 0;
    while(n != 0){
        n /= 10;
        count++;
    }

    if(count == 0) count = 1; // At least one digit for zero
    
    return count;
}