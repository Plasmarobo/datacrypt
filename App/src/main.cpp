
#include "bsp.h"
#include "scheduler.h"

void main() {
    scheduler_init();
    bsp_init();
    scheduler_exec();
}
