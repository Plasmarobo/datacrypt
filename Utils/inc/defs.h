#ifndef DEF_H
#define DEF_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define CONCAT(x, y) x##y
#define CONCATSTR(X, Y) CONCAT(#X, #Y)
#define TRICAT(x, y, z) x##y##z

#ifndef _Static_assert
#define _Static_assert static_assert
#endif

typedef void (*callback_t)(int32_t);
typedef uint32_t length_t;
typedef uint8_t* buffer_t;
typedef int32_t timespan_t;

// ========== uS timer ==========
#define MILLIS(x) (x * 1000)
#define MICROS(x) (x)

// Correct rollover depends on the width of timespan_t
#define CORRECT_ROLLOVER(x) (0xFFFFFFFF - x)

timespan_t microseconds();
timespan_t milliseconds();

extern void enter_critical(void);
extern void exit_critical(void);

#ifdef __cplusplus
}
#endif

#endif  // DEF_H
