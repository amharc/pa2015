#include <stdio.h>
#include <stdint.h>
#include <inttypes.h>

static const unsigned LIMIT = 19 * 9 * 9;

unsigned digits_squares_sum(uint64_t num) {
    unsigned result = 0;
    while(num) {
        unsigned digit = (unsigned) (num % 10);
        result += digit * digit;
        num /= 10;
    }
    return result;
}

int main() {
    uint64_t k, a, b;
    (void) scanf("%" SCNu64 "%" SCNu64 "%" SCNu64, &k, &a, &b);

    unsigned count = 0, mul;
    for(mul = 1; mul <= LIMIT && k * mul <= b; ++mul) {
        if(digits_squares_sum(k * mul) == mul && a <= k * mul) {
            ++count; 
        }
    }

    printf("%u\n", count);
    return 0;
}
