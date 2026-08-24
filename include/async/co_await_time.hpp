#pragma once

#include <thread>
#include "co_awaitable.hpp"

#ifdef VSN_WEB_BUILD
    #include <emscripten.h>
#else
    #include "trd_scheduler.hpp"
#endif

namespace async 
{
    struct Runner
    {
        static void Invoke(void* address)
        {
            auto h = std::coroutine_handle<>::from_address(address);
            if(h && !h.done()){
                h.resume();
            }
        }

        template<class TimePoint>
        static inline void Run(std::coroutine_handle<> h, TimePoint tp)
        {
            /* // example run with thread 
            std::jthread jt([h, tp](){
                std::this_thread::sleep_until(tp);
                if(h && !h.done()){
                    h.resume();
                }
            });

            jt.detach();
            */

            #if defined(VSN_WEB_BUILD)
            using sc = std::chrono::system_clock;
            auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(tp - sc::now());
            size_t duration = ms.count();

            EM_ASM({
                setTimeout(()=>{
                    wasmTable.get($0).apply(null, [$1])
                }, $2);
            }, &Invoke, h.address(), duration);
            #else
                static Scheduler sch; sch.run_after([h](){
                    if(h && !h.done()){
                        h.resume();
                    }
                }, tp);
           #endif
        }
    };


    template<typename _Runner, typename _Rep, typename _Period>
    inline auto after(const std::chrono::duration<_Rep, _Period>& dt)
    {
        using sc = std::chrono::system_clock;
        const sc::time_point tp = sc::now() + std::chrono::duration_cast<sc::duration>(dt);

        struct awaitable : private async::awaitable<void>
        {
            using async::awaitable<void>::await_ready;
            using async::awaitable<void>::await_resume;

            void await_suspend(std::coroutine_handle<> h) noexcept 
            {
                _Runner::Run(h, this->tp);
            }

            awaitable(sc::time_point time)
            : tp(time){
            }

            sc::time_point tp;
        };

        return awaitable{tp};
    }


    template<typename _Rep, typename _Period>
    inline auto after(const std::chrono::duration<_Rep, _Period>& dt)
    {
        return after<Runner,_Rep,_Period>(dt);
    }

    template<typename T, typename _Runner, typename _Rep, typename _Period>
    inline auto after(const std::chrono::duration<_Rep, _Period>& dt, T&& value)
    {
        using sc = std::chrono::system_clock;
        const sc::time_point tp = sc::now() + std::chrono::duration_cast<sc::duration>(dt);

        struct awaitable : private async::awaitable<T>
        {
            using async::awaitable<T>::await_ready;
            using async::awaitable<T>::await_resume;

            void await_suspend(std::coroutine_handle<> h) noexcept 
            {
                _Runner::Run(h, this->tp);
            }

            awaitable(T&& value, sc::time_point time)
            : async::awaitable<T>(std::move(value))
            , tp(time){
            }

            sc::time_point tp;
    };

    return awaitable{std::move(value), tp};
    }

    template<typename T, typename _Rep, typename _Period>
    inline auto after(const std::chrono::duration<_Rep, _Period>& dt, T&& value)
    {
        return after<T,Runner,_Rep,_Period>(dt, std::move(value));
    }
}
