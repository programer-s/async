#pragma once

#include "awaitable.hpp"
#include <coroutine>
#include <optional>
#include <functional>
#include <exception>
#include <condition_variable>

namespace async
{
    struct promise_base
    {
    public:
        struct init_awaitable
        {
            template<typename Type>
            constexpr void await_suspend(std::coroutine_handle<Type>) const noexcept {}
            constexpr bool await_ready() const noexcept { return true; }
            constexpr void await_resume() const noexcept {}
        };

        struct final_awaitable
        {
            template<typename Type>
            constexpr void await_suspend(std::coroutine_handle<Type>) const noexcept;
            constexpr bool await_ready() const noexcept { return false; }
            constexpr void await_resume() const noexcept {}
        };
    public:
        promise_base(){
            //std::cout << "promise_base::promise_base" << std::endl;
        }
        ~promise_base(){
            //std::cout << "promise_base::~promise_base" << std::endl;
        }
    public:
        init_awaitable initial_suspend() {return {};}
        final_awaitable final_suspend() noexcept {return {};}
    private:
        // void return_void() {}
        // void yield_value(From&& from) {}
        // void return_value(T&& in);
    public:
        void unhandled_exception();
    public:
        void free();
    public:
        std::function<void()> next;
        std::function<void(std::exception_ptr)> fail;
    public:
        std::exception_ptr exception_; // tread safe ??
        std::atomic<int> use_counter = {2};
        std::mutex mutex;
    };

    inline void promise_base::free()
    {
        if(--use_counter == 0){
            auto h = std::coroutine_handle<promise_base>::from_promise(*this);
            h.destroy();
        }
    }


    template<typename Type>
    constexpr void promise_base::final_awaitable::await_suspend(std::coroutine_handle<Type> h) const noexcept
    {
        auto& p = h.promise();
        std::unique_lock lk(p.mutex);

        if(p.next){
            p.next();
        }

        p.free();
    }

    inline void promise_base::unhandled_exception() 
    {
        std::unique_lock lk(mutex);
        exception_ = std::current_exception();
    }
}