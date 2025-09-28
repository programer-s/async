#pragma once

#include "awaitable.hpp"

namespace async 
{
    struct Runner
    {
        template<class TimePoint>
        static inline void Run(std::coroutine_handle<> h, TimePoint tp)
        {
            std::jthread jt([h, tp](){
                std::this_thread::sleep_until(tp);
                if(h && !h.done()){
                    h.resume();
                }
            });

            jt.detach();
        }
    };


    template<typename _Rep, typename _Period>
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
                Runner::Run(h, this->tp);
            }

            awaitable(sc::time_point time)
            : tp(time){
            }

            sc::time_point tp;
        };

        return awaitable{tp};
    }

    template<typename T, typename _Rep, typename _Period>
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
                Runner::Run(h, this->tp);
            }

            awaitable(T&& value, sc::time_point time)
            : async::awaitable<T>(std::move(value))
            , tp(time){
            }

            sc::time_point tp;
    };

    return awaitable{std::move(value), tp};
    }
}