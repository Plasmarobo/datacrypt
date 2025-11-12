#include "hal.h"
#include "defs.h"
#include "sensors.h"
// ========== Analog Channels ==========
// External voltage divider
uint16_t bat_read()
{
    return 3700; // Simulate full battery
}
// Floating analog pin
uint16_t rng_read()
{
    return 1234; // Fixed value for simulator
}
// Internal temperature sensor (converted)
uint16_t temp_read()
{
    return 25; // Fixed value for simulator
}
// Internal temperature sensor (raw)
uint16_t temp_read_raw()
{
    return 1000; // Fixed value for simulator
}
// Internal vbat
uint16_t vbat_read()
{
    return 3700; // Simulate full battery
}
