#pragma once

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <deque>
#include <functional>
#include <shared_mutex>
#include <thread>
#include <mutex>
#include <optional>


using namespace std::chrono_literals;


class Scheduler
{
  using SchedulerTask = std::function<void()>;

  struct DelayedTask
  {
      using sc = std::chrono::system_clock;
  
      sc::time_point start_time;
      SchedulerTask  function;
  };

public:
  template<typename _TimePoint>
  void run_after( std::function<void()> func, _TimePoint dt);

public:
    void wait(){
      std::unique_lock<std::mutex> lock(mutex);
      sub_cond.wait(lock, [this]{ return tasks.empty(); });
    };

private:
    std::optional<SchedulerTask> waitNextTask(std::stop_token token);
private:
    std::mutex mutex;
private:
    std::deque<DelayedTask> tasks;
    std::condition_variable_any add_cond;
    std::condition_variable_any sub_cond;
private:
    std::vector<std::jthread> pool;// ?
private:
    DelayedTask::sc::time_point tp;
    

    std::jthread _thread{ [this](std::stop_token token){
      while (!token.stop_requested())
      {
          auto func = waitNextTask(token);
          if(func.has_value()){
            (*func)();
            sub_cond.notify_all();
          }
      }
    }};

    std::jthread _work_threads;
};


inline std::optional<Scheduler::SchedulerTask> Scheduler::waitNextTask(std::stop_token token)
{
    std::unique_lock<std::mutex> lock(mutex);

    // wait first task
    if(tasks.empty()){
        add_cond.wait(lock, token, [this]{
          return !tasks.empty();
        });
    }

    while(!tasks.empty() && !token.stop_requested())
    {    
        auto tp = tasks[0].start_time;

        add_cond.wait_until(lock, token, tp, [this, tp]
        {
            return tasks[0].start_time < tp;
        });

        if(!token.stop_requested() && 
            tasks[0].start_time < DelayedTask::sc::now())
        {
            auto func = std::move(tasks[0].function);
            tasks.pop_front();

            return std::move(func);
        }
    }

    return {};
}



template<typename _TimePoint>
void Scheduler::run_after( std::function<void()> func, _TimePoint tp)
{
  std::lock_guard<std::mutex> lock(mutex);

  auto it = std::lower_bound(tasks.begin(), tasks.end(), tp, [](const DelayedTask& task, DelayedTask::sc::time_point tp){ 
    return task.start_time < tp;
  });

  tasks.insert( it, 
    { 
      .start_time = tp,
      .function = func,
    }
  );

  add_cond.notify_one();
}
