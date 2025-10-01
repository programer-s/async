#include <iostream>

#include <scheduler.hpp>


Scheduler g_sch;

template<typename _Rep, typename _Period>
void async(std::function<void()> func, const std::chrono::duration<_Rep, _Period>& dt)
{
  g_sch.run_after(func, dt);
};

void async(std::function<void()> func)
{
  g_sch.run_after(func, 0s);
};


int main()
{
  
  async([]{
    std::cout << "running Task 2" << std::endl;
  },10.s);

  async([]{
    std::cout << "running Task 1" << std::endl;
  },1.s);

  async([]{
    std::cout << "running Task 3" << std::endl;
  },15.s);

  async([]{
    std::cout << "running Task 0" << std::endl;

    async([](){
      std::cout << "running sub task 0" << std::endl;
    });

    async([](){
      std::cout << "running sub task 1" << std::endl;
    }, 1.0s);

  });

  g_sch.wait();

  return 0;
}

