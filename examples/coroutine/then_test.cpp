#include <iostream>
#include <chrono>

#include <async/co_task.hpp>

using namespace std::chrono_literals;

static int g_checks = 0;
static int g_fails = 0;

#define CHECK(cond) do { \
    if(cond){ ++g_checks; std::cout << "ok: " << #cond << std::endl; } \
    else { ++g_fails; std::cout << "FAIL: " << #cond << std::endl; } \
} while(0)

async::task<int> value_task(int v)
{
    co_await async::after(1ms);
    co_return v;
}

async::task<> void_task()
{
    co_await async::after(1ms);
    co_return;
}

async::task<int> throwing_task()
{
    co_await async::after(1ms);
    throw std::runtime_error("boom");
    co_return 0;
}

int main()
{
    // 1. void task -> then returning value
    int r1 = void_task().then([]{ return 42; }).get();
    CHECK(r1 == 42);

    // 2. void task -> then returning void
    bool called = false;
    void_task().then([&called]{ called = true; }).get();
    CHECK(called);

    // 3. value task -> then transforming value
    int r3 = value_task(21).then([](int&& x){ return x * 2; }).get();
    CHECK(r3 == 42);

    // 4. value task -> then void
    bool called4 = false;
    value_task(7).then([&called4](int&& x){ called4 = (x == 7); }).get();
    CHECK(called4);

    // 5. exception propagates through then
    bool caught = false;
    try {
        throwing_task().then([](int&& x){ return x; }).get();
    } catch(const std::runtime_error&) {
        caught = true;
    }
    CHECK(caught);

    // 6. chained then: value -> value -> value
    int r6 = value_task(1)
        .then([](int&& x){ return x + 1; })
        .then([](int&& x){ return x + 40; })
        .get();
    CHECK(r6 == 42);

    std::cout << "checks=" << g_checks << " fails=" << g_fails << std::endl;
    return g_fails == 0 ? 0 : 1;
}
