#pragma once

#include <async/co_task_base.hpp>

#include <type_traits>
#include <utility>

namespace async
{
    struct task_void
    {
    public:
        struct promise_type : public promise_base
        {
        public:
            task_void get_return_object();
        public:
            void return_void();
        };

        using handle_type = std::coroutine_handle<promise_type>;
    public:
        task_void(handle_type h) noexcept
        :handle(h) 
        {}

        ~task_void();
    public:
        auto operator co_await() const & noexcept;

    public:
        template<detail::invocable_with<> Func>
        auto then(Func&& func);
        void error(std::function<void(std::exception&&)>);
        void get();
    private:
        handle_type handle;
    };

    inline task_void::~task_void()
    {
        // free coroutine handle
        handle.promise().free();
    }

    inline task_void task_void::promise_type::get_return_object() 
    { 
        return task_void(task_void::handle_type::from_promise(*this));
    }

    inline void task_void::promise_type::return_void()
    {
    }

    inline auto task_void::operator co_await() const & noexcept
    {
        struct _awaitable
        {
            bool await_ready() noexcept { 
                std::unique_lock lk(promise.mutex);

                auto h = handle_type::from_promise(promise);
                return h.done() || promise.exception_; //TODO done?
            }

            void await_resume() noexcept { 
              if(promise.exception_){
                std::rethrow_exception(promise.exception_);
              }
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

    namespace detail
    {
        struct void_handle_awaitable
        {
            task_void::handle_type h;

            bool await_ready() noexcept {
                std::unique_lock lk(h.promise().mutex);
                return h.done() || h.promise().exception_;
            }

            void await_resume() {
                if(h.promise().exception_){
                    std::rethrow_exception(h.promise().exception_);
                }
            }

            void await_suspend(std::coroutine_handle<> ch) noexcept {
                std::unique_lock lk(h.promise().mutex);
                h.promise().next = [ch](){
                    ch.resume();
                };
            }
        };

        template<detail::invocable_with<> Func>
        auto then_impl(task_void::handle_type h, Func f)
            -> then_task_t<std::invoke_result_t<Func>>
        {
            co_await void_handle_awaitable{h};
            if constexpr (std::is_void_v<std::invoke_result_t<Func>>) {
                f();
                co_return;
            } else {
                co_return f();
            }
        }
    }

    template<detail::invocable_with<> Func>
    inline auto task_void::then(Func&& func)
    {
        using F = std::decay_t<Func>;
        return detail::then_impl(handle, F(std::forward<Func>(func)));
    }

    inline void task_void::get()
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
    }
}