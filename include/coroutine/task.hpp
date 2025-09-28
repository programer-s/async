#pragma once 

#include "task_void.hpp"
#include "task_value.hpp"
#include "await_time.hpp"

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