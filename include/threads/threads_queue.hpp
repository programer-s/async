
#include <deque>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <functional>


template<typename T>
class TaskQueue {
public:
    explicit TaskQueue(std::function<void(T&&)> task, size_t max_size) 
    : maxSize_(max_size) 
    , task_(task)
    {}

    // Добавление элемента в очередь с ожиданием места
    bool push(T item) 
    {
        std::unique_lock<std::mutex> lock(mutex_);

        if(closed_){
            return false;
        }

        if((threads_.empty() || queue_.size() == maxSize_))
        {
            if(threads_.size() < 10 )
            {
                threads_.emplace_back([this](std::stop_token stop){
                    for(;;)
                    {
                        auto [value,close] = pop(stop);
                        if(close){
                            break;
                        }

                        task_(std::move(*value));

                        --numValues;
                        condEnd_.notify_all();
                    }
                });
            }
        }

        condFull_.wait(lock, [this]{ return queue_.size() < maxSize_; });  // Ждём, пока есть свободное место

        queue_.push_back(item); ++numValues;
        condEmpty_.notify_one();  // Уведомляем потребителей, что появилась новая запись

        return true;
    }

    void wait()
    {
        std::unique_lock<std::mutex> lock(mutex_);
        condEnd_.wait(lock, [this]{return numValues == 0;});
    }

    void close()
    {
        std::unique_lock<std::mutex> lock(mutex_);
        closed_ = true;
        condEmpty_.notify_all();  // Просыпаются все потоки потребления
        condFull_.notify_all();   // Просыпаются все потоки производства
    }
private:
    // Извлечение элемента из начала очереди с ожиданием данных
    std::pair<std::optional<T>,bool> pop(std::stop_token st) 
    {
        std::unique_lock<std::mutex> lock(mutex_);
        condEmpty_.wait(lock, st, [this]{ return closed_ || !queue_.empty(); });  // Ждём, пока есть элементы

        if(closed_){
            return {{},true};
        }

        if(st.stop_requested() && queue_.empty()) {
            return {{}, true};
        }

        T frontItem = std::move(queue_.front());
        queue_.pop_front();
        condFull_.notify_one();  // Уведомляем производителей, что появилось место
        return {std::move(frontItem),false};
    }

private:
    size_t maxSize_;      // Максимальный размер очереди
    std::deque<T> queue_; // Очередь
    std::mutex mutex_;    // Мьютекс для синхронизации

    std::condition_variable_any condEmpty_; // Условная переменная для ожидания данных
    std::condition_variable condFull_;  // Условная переменная для ожидания свободного места
    std::condition_variable condEnd_;  // Условная переменная для ожидания окончания работ

    std::vector<std::jthread> threads_;

    std::atomic<size_t> numValues{0};
    std::atomic<bool> closed_{false}; // Флаг закрытия очереди

    std::function<void(T&&)> task_;
};