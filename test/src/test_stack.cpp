#define CATCH_CONFIG_MAIN
#include <catch_amalgamated.hpp>

#include "stack.h"

TEST_CASE("Stack push", "[stack]") {
    int a = 1;
    int b = -2;
    int c = 0x00FF0000;
    int test;
    STACK(teststack, int, 3);
    REQUIRE(stack_empty(&teststack) == true);
    stack_push(&teststack, &a);
    REQUIRE(stack_empty(&teststack) == false);
    stack_peek(&teststack, &test);
    REQUIRE(a == test);
    stack_push(&teststack, &b);
    stack_peek(&teststack, &test);
    REQUIRE(b == test);
    stack_push(&teststack, &c);
    REQUIRE(stack_full(&teststack) == true);
    stack_peek(&teststack, &test);
    REQUIRE(c == test);
}

TEST_CASE("Stack pop", "[stack]") {
    int a = 1;
    int b = 2;
    int c = 3;
    int test;
    STACK(teststack, int, 3);
    stack_push(&teststack, &a);
    stack_push(&teststack, &b);
    stack_push(&teststack, &c);
    REQUIRE(stack_full(&teststack) == true);
    stack_pop(&teststack, &test);
    REQUIRE(stack_full(&teststack) == false);
    REQUIRE(stack_empty(&teststack) == false);
    REQUIRE(test == c);
    stack_pop(&teststack, &test);
    REQUIRE(test == b);
    stack_pop(&teststack, &test);
    REQUIRE(test == a);
    REQUIRE(stack_full(&teststack) == false);
    REQUIRE(stack_empty(&teststack) == true);
}

TEST_CASE("Pointer stack", "[stack]") {
    int a = 1;
    int b = 2;
    int c = 3;
    int* ra = &a;
    int* rb = &b;
    int* rc = &c;
    STACK(ts, int*, 3);
    stack_push(&ts, &ra);
    stack_push(&ts, &rb);
    stack_push(&ts, &rc);
    int* ta;
    int* tb;
    int* tc;
    b += 1;
    c += 2;
    stack_pop(&ts, &tc);
    stack_pop(&ts, &tb);
    stack_pop(&ts, &ta);
    REQUIRE(tc == rc);
    REQUIRE(*tc == c);
    REQUIRE(tb == rb);
    REQUIRE(*tb == b);
    REQUIRE(ta == ra);
    REQUIRE(*ta == a);
    REQUIRE(*tc == 5);
    REQUIRE(*tb == 3);
    REQUIRE(*ta == 1);
}
