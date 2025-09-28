#pragma once

#include <deque>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <future>

template<typename T>
class BoundedThreadSafeQueue {
public:
    explicit BoundedThreadSafeQueue(size_t max_size) : maxSize_(max_size) {}

    // Добавление элемента в очередь с ожиданием места
    bool push(T item) {
        std::unique_lock<std::mutex> lock(mutex_);
        condFull_.wait(lock, [this]{ return closed_ || queue_.size() < maxSize_; });  // Ждём, пока есть свободное место
        if(closed_){
            return false;
        }
        queue_.push_back(item);
        condEmpty_.notify_one();  // Уведомляем потребителей, что появилась новая запись

        return true;
    }

    // Извлечение элемента из начала очереди с ожиданием данных
    std::pair<std::optional<T>,bool> pop() {
        std::unique_lock<std::mutex> lock(mutex_);
        condEmpty_.wait(lock, [this]{ return closed_ || !queue_.empty(); });  // Ждём, пока есть элементы

        if(closed_ && queue_.empty()) {
            return {{}, true};
        }

        T frontItem = std::move(queue_.front());
        queue_.pop_front();
        condFull_.notify_one();  // Уведомляем производителей, что появилось место
        return {std::move(frontItem),false};
    }

    void close()
    {
        std::unique_lock<std::mutex> lock(mutex_);
        closed_ = true;
        condEmpty_.notify_all();  // Просыпаются все потоки потребления
        condFull_.notify_all();   // Просыпаются все потоки производства
    }

private:
    size_t maxSize_;                     // Максимальный размер очереди
    std::deque<T> queue_;               // Очередь
    std::mutex mutex_;           // Мьютекс для синхронизации
    std::condition_variable condEmpty_; // Условная переменная для ожидания данных
    std::condition_variable condFull_;  // Условная переменная для ожидания свободного места
    std::atomic<bool> closed_{false}; // Флаг закрытия очереди
};