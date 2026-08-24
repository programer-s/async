#pragma once 

#include <async/co_task_void.hpp>
#include <async/co_task_value.hpp>
#include <async/co_await_time.hpp>

namespace async
{
    template<typename _T>
    struct task_t
    {
        using T = task_value<_T>;
    };

    template<>
    struct task_t<void>
    {
        using T = task_void;
    };

    template<typename T = void>
    using task = task_t<T>::T;
}

// Async task
template<typename Type>
using atask = async::task<Type>;