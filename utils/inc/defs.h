#ifndef DEF_H
#define DEF_H

#ifdef __cplusplus
#include <cstddef>
#include <cstdint>
extern "C" {
#else
#include <stddef.h>
#include <stdint.h>
#endif

#ifdef INTELLISENSE
#define __attribute__(x)
#endif

#define UNUSED(x) (void)(x)

#define STATUS_OK (0)

#define CONCAT(x, y) x##y
#define CONCATSTR(X, Y) CONCAT(#X, #Y)
#define TRICAT(x, y, z) x##y##z

#ifndef _Static_assert
#define _Static_assert static_assert
#endif

typedef void (*callback_t)(int32_t);
typedef uint32_t length_t;
typedef uint8_t* buffer_t;
typedef uint32_t timespan_t;

// ========== uS timer ==========
#define MILLIS(x) (x * 1000)
#define MICROS(x) (x)

timespan_t microseconds();
timespan_t milliseconds();

extern void enter_critical(void);
extern void exit_critical(void);

#ifdef __cplusplus
}
#endif

#endif  // DEF_H
