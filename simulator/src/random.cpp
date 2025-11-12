#include "defs.h"
#include "random.h"
#include <random>

void random_init()
{
}
// Produce a 16 bit prng number
uint32_t random_int()
{
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<uint32_t> dis(0, 0xFFFF);
    return dis(gen);
}
uint32_t uniform(uint32_t min, uint32_t max)
{
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<uint32_t> dis(min, max);
    return dis(gen);
}
