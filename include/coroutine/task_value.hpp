#pragma once

#include "task_base.hpp"

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
        Type&& get();

        void then(std::function<void(Type&& value)>);
    private:
        handle_type handle;
    };

    template<typename Type>
    inline task_value<Type>::~task_value<Type>() 
    {
        handle.destroy();
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
    inline Type&& task_value<Type>::get()
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

        return *p.value;
    }

    template<typename Type>
    void task_value<Type>::then(std::function<void(Type&& value)> func)
    {
        auto& p = handle.promise();
        std::unique_lock lk(p.mutex);

        //check befor runing coroutine
        if(p.exception_){
            std::rethrow_exception(p.exception_);
        }

        p.then = [func, &p](){
            //check after runing coroutine
            if(p.exception_){
                std::rethrow_exception(p.exception_);
            }

            fun(std::move(*p.value));
        };
    }
}