#pragma once

#include "co_task_base.hpp"

#include <type_traits>
#include <utility>

namespace async
{
    template<typename Type> 
    struct task_value
    {
    public:
        struct promise_type : public promise_base
        {
        public:
            task_value<Type> get_return_object();
        public:
            void return_value(Type&& in);
        public:
            std::optional<Type> value;
        };

        using handle_type = std::coroutine_handle<promise_type>;
    public:
        task_value(handle_type h) noexcept
        :handle(h) 
        {}

        ~task_value();
    public:
        auto operator co_await() const & noexcept;

    public:
        template<detail::invocable_with<Type&&> Func>
        auto then(Func&& func);
        void error(std::function<void(std::exception&&)>);
        Type get();
    private:
        handle_type handle;
    };

    template<typename Type>
    inline task_value<Type>::~task_value<Type>() 
    {
        // free coroutine handle
        handle.promise().free();
    }

    template<typename Type>
    inline task_value<Type> task_value<Type>::promise_type::get_return_object() 
    { 
        return task_value<Type>(task_value<Type>::handle_type::from_promise(*this));
    }

    template<typename Type>
    inline void task_value<Type>::promise_type::return_value(Type&& in)
    {
        value = std::move(in);
    }

    template<typename Type>
    inline auto task_value<Type>::operator co_await() const & noexcept
    {
        struct _awaitable
        {
            bool await_ready() noexcept { 
                std::unique_lock lk(promise.mutex);

                return promise.value || promise.exception_;
            }

            Type&& await_resume() { 
                if(promise.exception_){
                    std::rethrow_exception(promise.exception_);
                }

                return std::move(promise.value.value()); 
            }

            void await_suspend(std::coroutine_handle<> h) noexcept 
            {
                std::unique_lock lk(promise.mutex);

                promise.next = [h](){
                  h.resume();
                };
            }

            promise_type& promise;
        };

        return _awaitable{handle.promise()};
    }

    template<typename Type>
    inline Type task_value<Type>::get()
    {
        auto& p = handle.promise();
        std::unique_lock lk(p.mutex);

        //check befor runing coroutine
        if(p.exception_){
            std::rethrow_exception(p.exception_);
        }

        std::condition_variable cv;
        handle.promise().next = [&cv](){
            cv.notify_one();
        };

        cv.wait(lk);

        //check after runing coroutine
        if(p.exception_){
            std::rethrow_exception(p.exception_);
        }

        return std::move(*p.value);
    }

    namespace detail
    {
        template<typename Type>
        struct value_handle_awaitable
        {
            typename task_value<Type>::handle_type h;

            bool await_ready() noexcept {
                std::unique_lock lk(h.promise().mutex);
                return h.promise().value || h.promise().exception_;
            }

            Type&& await_resume() {
                if(h.promise().exception_){
                    std::rethrow_exception(h.promise().exception_);
                }

                return std::move(h.promise().value.value());
            }

            void await_suspend(std::coroutine_handle<> ch) noexcept {
                std::unique_lock lk(h.promise().mutex);
                h.promise().next = [ch](){
                    ch.resume();
                };
            }
        };

        template<typename Type, detail::invocable_with<Type&&> Func>
        auto then_impl(typename task_value<Type>::handle_type h, Func f)
            -> then_task_t<std::invoke_result_t<Func, Type&&>>
        {
            Type value = co_await value_handle_awaitable<Type>{h};
            if constexpr (std::is_void_v<std::invoke_result_t<Func, Type&&>>) {
                f(std::move(value));
                co_return;
            } else {
                co_return f(std::move(value));
            }
        }
    }

    template<typename Type>
    template<detail::invocable_with<Type&&> Func>
    inline auto task_value<Type>::then(Func&& func)
    {
        using F = std::decay_t<Func>;
        return detail::then_impl<Type>(handle, F(std::forward<Func>(func)));
    }

}
