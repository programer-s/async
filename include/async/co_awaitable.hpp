#pragma once

#include <coroutine>
#include <optional>

#include <iostream>


namespace async
{
    class awaitable_base
    {
    public:
        awaitable_base() noexcept {
            std::cout <<"awaitable_base "<< std::endl;
        };

        ~awaitable_base() noexcept {
            
        }
    public:
        awaitable_base(const awaitable_base&) = delete;
        awaitable_base& operator = (const awaitable_base&) = delete;
    public:
        bool await_ready() noexcept { 
            return false;
        }

        void await_suspend(std::coroutine_handle<> h) noexcept {
            std::cout << "await_suspend "<< h.address() << std::endl;
            handle = h;
        }
    protected:
        std::coroutine_handle<> handle;
    };


    class awaitable_void : private awaitable_base
    {
    public:
        void await_resume() noexcept {}

        bool await_ready() noexcept {
            return false;
        }

        void await_suspend(std::coroutine_handle<> h) noexcept {
            awaitable_base::await_suspend(h);
        }
    public: 
        void resume()
        {
            if(handle && !handle.done()){
                handle.resume();
            }
        }
    private:
    };


    template<typename Result>
    class awaitable_value : private awaitable_base
    {
    public:
        awaitable_value() noexcept = default;

        explicit awaitable_value(Result&& in) noexcept {
            value = std::move(in);
        }
    public:
        Result await_resume() noexcept { 
            return std::move(value.value()); 
        }

        bool await_ready() noexcept { 
            return handle && handle.done();// && value.has_value(); 
        }

        void await_suspend(std::coroutine_handle<> h) noexcept {
            awaitable_base::await_suspend(h);
        }
    public:
        void operator = ( Result&& in ) noexcept {
            value = std::move(in);

            if(handle && !handle.done()){
                handle.resume();
            }
        }
    private:
        std::optional<Result> value;
    };
 

    template<typename _T>
    struct awaitable_t
    {
        using T = awaitable_value<_T>;
    };

    template<>
    struct awaitable_t<void>
    {
        using T = awaitable_void;
    };

    template<typename T = void>
    using awaitable = awaitable_t<T>::T;

} // namespace async
