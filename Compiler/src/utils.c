#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../include/utils.h"

/**
 * Returns the count of digits of an unsigned integer
 */
unsigned int countDigits(unsigned int n){
    unsigned int count = 0;
    while(n != 0){
        n /= 10;
        count++;
    }

    if(count == 0) count = 1; // At least one digit for zero
    
    return count;
}

/**
 * Prints the <filler> character enough times so that <str> occupies <target_len> characters
 */
void FillSpace(FILE* buffer, const char* str, unsigned int target_len, const char* filler){
    unsigned int len = strlen(str);
    for(unsigned int i = len; i < target_len; i++){
        fprintf(buffer, "%s", filler);
    }
}
