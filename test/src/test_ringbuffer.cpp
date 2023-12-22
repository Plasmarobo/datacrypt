#include <catch_amalgamated.hpp>

#include "ringbuffer.h"

TEST_CASE("Ringbuffer push", "[ringbuffer]") {
    int a = 1;
    int b = 2;
    int c = 3;
    RINGBUFFER(tb, int, 3);
    REQUIRE(ringbuffer_empty(&tb) == true);
    REQUIRE(ringbuffer_push(&tb, &a));
    REQUIRE(ringbuffer_empty(&tb) == false);
    REQUIRE(ringbuffer_full(&tb) == false);
    REQUIRE(ringbuffer_push(&tb, &b));
    REQUIRE(ringbuffer_push(&tb, &c));
    REQUIRE(ringbuffer_full(&tb) == true);
    REQUIRE(ringbuffer_empty(&tb) == false);
}

TEST_CASE("Ringbuffer pop", "[ringbuffer]") {
    int a = 1;
    int b = 2;
    int c = 3;
    int aa = 0;
    int bb = 0;
    int cc = 0;
    RINGBUFFER(tb, int, 3);
    ringbuffer_push(&tb, &a);
    ringbuffer_push(&tb, &b);
    ringbuffer_push(&tb, &c);
    ringbuffer_peek(&tb, &aa);
    REQUIRE(aa == a);
    REQUIRE(ringbuffer_full(&tb) == true);
    ringbuffer_pop(&tb, &aa);
    REQUIRE(aa == a);
    ringbuffer_pop(&tb, &bb);
    REQUIRE(bb == b);
    ringbuffer_pop(&tb, &cc);
    REQUIRE(cc == c);
    REQUIRE(ringbuffer_empty(&tb) == true);
    REQUIRE(ringbuffer_full(&tb) == false);
}

TEST_CASE("Pointer queue", "[ringbuffer]") {
    int a = 1;
    int b = 2;
    int c = 3;
    int *pa = &a;
    int *pb = &b;
    int *pc = &c;
    int *ra = NULL;
    int *rb = NULL;
    int *rc = NULL;
    RINGBUFFER(tb, int *, 3);
    ringbuffer_push(&tb, &pa);
    ringbuffer_push(&tb, &pb);
    ringbuffer_push(&tb, &pc);
    ringbuffer_pop(&tb, &ra);
    ringbuffer_pop(&tb, &rb);
    ringbuffer_pop(&tb, &rc);
    REQUIRE(pa == ra);
    REQUIRE(ra == &a);
    REQUIRE(pb == rb);
    REQUIRE(rb == &b);
    REQUIRE(pc == rc);
    REQUIRE(rc == &c);
}
