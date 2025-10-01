#include <iostream>

#include <coroutine>
#include <chrono>
#include <thread>

#include <functional>
#include <format>


#include <coroutine/task.hpp>

//#include <ucontext.h>

// https://en.cppreference.com/w/cpp/language/coroutines
// https://habr.com/ru/articles/520756/

using namespace std::chrono_literals;


async::task<int> test(int n);


async::task<int> test(int n)
{
  std::cout << "enter" << n << std::endl;
  if(n > 0){
    auto res = co_await test(--n);
    co_await async::after(100ms);
    
    std::cout << "exit" << res << std::endl;
    co_return co_await async::after(1000ms, res - 1);
  }

  std::cout << "exit" << n << std::endl;

  co_return co_await async::after(1000ms, 10);
}

async::task<int> test_1()
{
  //throw(0);
  int value = co_await async::after(0ms, 10); 
  //throw(0);

  co_return value;
}

async::task<> test_2()
{

  for(int i = 0; i < 2; ++i){
    co_await test_1(); 
  }

  co_return;
}


void _test_()
{
    test_2()
    /*.then([](){
      std::cout << "end" << std::endl;
    });*/
    .get();

  std::cout << "final";
}





int main()
{
  std::function<async::task<>()> f = []() -> async::task<> 
  {
    co_return;
  };


  try
  {
    _test_();
  }
  catch(...)
  {
  }
  
  

  //std::this_thread::sleep_for(100000ms);

  return 0;
}

