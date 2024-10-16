#define CATCH_CONFIG_MAIN
#define CACHE_CONFIG_FAST_COMPILE

#include <catch_amalgamated.hpp>

#include "scheduler.h"

static timespan_t _us = 0;
void tick(timespan_t us) { _us += us; }
static void tick_wrapper(int32_t amount) {
    task_delayed(tick_wrapper, 1);
    tick((timespan_t)amount);
}

timespan_t microseconds() { return _us; }
timespan_t milliseconds() { return _us / 1000; }
void enter_critical(){}
void exit_critical(){}
void error_handler() { FAIL("Error handler hit"); }

static int a, b, c;
void inc_a(int32_t x) { a += 1; }
void inc_b(int32_t x) { b += 1; }
void inc_c(int32_t x) { c += 1; }

TEST_CASE("Scheduler init clears state", "[scheduler]") {
    a = 0;
    scheduler_init();
    task_immediate(inc_a);
    REQUIRE(a == 0);
    scheduler_init();
    scheduler_exec();
    REQUIRE(a == 0);
}

TEST_CASE("Scheduler exec runs immediate task", "[scheduler]") {
    a = 0;
    b = 0;
    c = 0;
    scheduler_init();
    scheduler_exec();
    REQUIRE(a == 0);
    REQUIRE(b == 0);
    REQUIRE(c == 0);
    task_immediate(inc_a);
    REQUIRE(a == 0);
    scheduler_exec();
    REQUIRE(a == 1);
    tick(1);
    scheduler_exec();
    REQUIRE(a == 1);
    REQUIRE(b == 0);
    REQUIRE(c == 0);
}

TEST_CASE("Scheduler exec runs delayed task", "[scheduler]") {
    a = 0;
    b = 0;
    c = 0;
    scheduler_init();
    task_delayed(inc_a, 2);
    task_delayed(inc_b, 5);
    task_delayed(inc_c, 10);
    scheduler_exec();
    REQUIRE(a == 0);
    REQUIRE(b == 0);
    REQUIRE(c == 0);
    tick(1);
    scheduler_exec();
    scheduler_exec();
    REQUIRE(a == 0);
    REQUIRE(b == 0);
    REQUIRE(c == 0);
    tick(3);
    scheduler_exec();
    scheduler_exec();
    REQUIRE(a == 1);
    REQUIRE(b == 0);
    REQUIRE(c == 0);
    tick(10);
    scheduler_exec();
    REQUIRE(a == 1);
    REQUIRE(b == 1);
    REQUIRE(c == 1);
    tick(10);
    scheduler_exec();
    REQUIRE(a == 1);
    REQUIRE(b == 1);
    REQUIRE(c == 1);
}

TEST_CASE("Scheduler exec runs periodic task", "[scheduler]")
{
    a = 0;
    b = 0;
    c = 0;
    scheduler_init();
    task_periodic(inc_a, 10);
    task_periodic(inc_b, 20);
    task_periodic(inc_c, 9999);
    for(int i = 0; i < 100; ++i)
    {
        tick(1);
        scheduler_exec();
    }
    REQUIRE(a == 10);
    REQUIRE(b == 5);
    REQUIRE(c == 0);
    tick(9899);
    scheduler_exec();
    REQUIRE(c == 1);
    REQUIRE(a == 11);
    REQUIRE(b == 6);
}

TEST_CASE("Scheduler aborts immedate task", "[scheduler]")
{
    a = 0;
    b = 0;
    c = 0;
    scheduler_init();
    task_handle_t ah = task_immediate(inc_a);
    task_immediate(inc_b);
    REQUIRE(ah != NULL);
    task_abort(ah);
    scheduler_exec();
    tick(1);
    scheduler_exec();
    REQUIRE(a == 0);
    REQUIRE(b == 1);
}

TEST_CASE("Scheduler aborts delayed task", "[scheduler]")
{
    a = 0;
    b = 0;
    c = 0;
    scheduler_init();
    task_handle_t ah = task_delayed(inc_a, 10);
    task_immediate(inc_b);
    task_handle_t ch = task_delayed(inc_c, 5);
    scheduler_exec();
    tick(2);
    scheduler_exec();
    tick(2);
    task_abort(ah);
    task_abort(ch);
    scheduler_exec();
    tick(2);
    scheduler_exec();
    REQUIRE(a == 0);
    REQUIRE(b == 1);
    REQUIRE(c == 0);
    tick(2);
    scheduler_exec();
    tick(999);
    scheduler_exec();
    REQUIRE(a == 0);
    REQUIRE(b == 1);
    REQUIRE(c == 0);
}

TEST_CASE("Scheduler aborts periodic task", "[scheduler]")
{
    a = 0;
    b = 0;
    c = 0;
    scheduler_init();
    task_handle_t ah = task_periodic(inc_a, 5);
    task_handle_t bh = task_delayed(inc_b, 10);
    task_handle_t ch = task_periodic(inc_c, 5);
    scheduler_exec();
    tick(1);
    scheduler_exec();
    REQUIRE(a == 0);
    REQUIRE(b == 0);
    REQUIRE(c == 0);
    task_abort(ch);
    tick(1);
    scheduler_exec();
    tick(3);
    scheduler_exec();
    REQUIRE(a == 1);
    REQUIRE(b == 0);
    REQUIRE(c == 0);
    tick(10);
    scheduler_exec();
    REQUIRE(a == 2);
    REQUIRE(b == 1);
    REQUIRE(c == 0);
    tick(5);
    scheduler_exec();
    REQUIRE(a == 3);
    REQUIRE(b == 1);
    REQUIRE(c == 0);
    tick(5);
    task_abort(ah);
    scheduler_exec();
    REQUIRE(a == 3);
    REQUIRE(b == 1);
    REQUIRE(c == 0);
    tick(5);
    scheduler_exec();
    REQUIRE(a == 3);
    REQUIRE(b == 1);
    REQUIRE(c == 0);
}

TEST_CASE("Scheduler exec runs delayed unique task", "[scheduler]")
{
    a = 0;
    b = 0;
    c = 0;
    scheduler_init();
    task_delayed(inc_a, 10);
    task_delayed(inc_a, 11);
    task_delayed_unique(inc_b, 10);
    task_delayed_unique(inc_b, 11);
    task_delayed(inc_c, 5);
    task_delayed_unique(inc_c, 7);
    tick(1);
    scheduler_exec();

    tick(4);
    scheduler_exec();
    REQUIRE(a == 0);
    REQUIRE(b == 0);
    REQUIRE(c == 0);
    tick(2);
    scheduler_exec();
    REQUIRE(a == 0);
    REQUIRE(b == 0);
    REQUIRE(c == 1);
    tick(3);
    scheduler_exec();
    REQUIRE(a == 1);
    REQUIRE(b == 0);
    REQUIRE(c == 1);
    tick(1);
    scheduler_exec();
    REQUIRE(a == 2);
    REQUIRE(b == 1);
    REQUIRE(c == 1);
    task_delayed_unique(inc_b, 10);
    tick(5);
    scheduler_exec();
    task_delayed_unique(inc_b, 10);
    tick(5);
    scheduler_exec();
    REQUIRE(b == 1);
    tick(5);
    scheduler_exec();
    REQUIRE(b == 2);
}

TEST_CASE("Scheduler aborts delayed unique task", "[scheduler]")
{
    a = 0;
    b = 0;
    c = 0;
    scheduler_init();

    task_handle_t bh = task_delayed_unique(inc_b, 10);
    tick(1);
    scheduler_exec();
    task_delayed_unique(inc_b, 11);
    tick(1);
    scheduler_exec();
    task_abort(bh);
    tick(10);
    scheduler_exec();
    REQUIRE(b == 0);
    tick(999);
    scheduler_exec();
    REQUIRE(b == 0);
}

static void signala(int32_t sig)
{
    a += sig;
}

static void signalb(int32_t sig)
{
    b += sig;
}

TEST_CASE("Scheduler delivers signal to task", "[scheduler]")
{
    a = 0;
    b = 0;
    c = 0;
    scheduler_init();

    task_handle_t sah = task_immediate(signala);
    task_handle_t sbh = task_immediate_signal(signalb, 10);
    scheduler_exec();
    tick(1);
    scheduler_exec();
    REQUIRE(a == 0);
    REQUIRE(b == 10);
    sah = task_delayed_signal(signala, 5, 5);
    scheduler_exec();
    tick(1);
    task_signal(sah, 3);
    scheduler_exec();
    tick(4);
    scheduler_exec();
    REQUIRE(a == 3);
    REQUIRE(b == 10);
}

static task_handle_t ahandle;

void cancel_a(int32_t sig)
{
    task_abort(ahandle);
}

TEST_CASE("Scheduler abort from task", "[scheduler]")
{
    a = 0;
    b = 0;
    c = 0;
    scheduler_init();
    ahandle = task_delayed_unique(inc_a, 10);
    task_delayed(cancel_a, 9);
    scheduler_exec();
    tick(9);
    scheduler_exec();
    tick(1);
    scheduler_exec();
    tick(1);
    scheduler_exec();
    tick(999);
    scheduler_exec();
    REQUIRE(a == 0);
}

static callback_t future_cb;

static void resolve_blocking(int32_t status)
{
    if (NULL != future_cb)
    {
        future_cb(status);
        future_cb = NULL;
    }
}

static void nonblocking_op(callback_t on_complete)
{
    future_cb = on_complete;
    task_delayed_unique_signal(resolve_blocking, 10, 3);
}

static void tick_op(int32_t status)
{
    // Force the counter to advance
    tick(1);
}

TEST_CASE("Scheduler future await execution", "[futures]")
{
    int32_t status = 0;
    scheduler_init();
    // Kickstart the timer
    scheduler_exec();
    tick(1);
    task_periodic(tick_op, 1);
    // Acquire a future
    callback_t future = future_get();
    // Simulate nonblocking op
    nonblocking_op(future);
    // Await op
    status = future_await(future, 15);
    REQUIRE(status == 3);
}

TEST_CASE("Scheduler nested future rejected", "[futures]")
{
    scheduler_init();
    future_t a = future_get();
    future_t b = future_get();
    REQUIRE(a != NULL);
    REQUIRE(b == NULL);
}

static uint32_t circles = 0;

static void circular_future(int32_t _) {
    if (circles < 10) {
        ++circles;
        // Intentionally do not protect here
        future_t f = future_get();
        future_await(f, 10);
    }
}

TEST_CASE("Scheduler circular futures prevented", "[futures]") {
    scheduler_init();
    scheduler_exec();
    tick(1);
    // Simulate time
    task_periodic(tick_op, 1);
    tick(1);
    // Call function on next tick
    task_immediate(circular_future);
    scheduler_exec();
    // Function should have only been called once
    // if it's called more than once (or hangs), it make be invoked before the
    // timer can tick Which will fill the stack and lock up the scheduler
    REQUIRE(circles == 1);
}
