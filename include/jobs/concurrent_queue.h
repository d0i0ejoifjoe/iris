#pragma once

#include <deque>
#include <mutex>

#include "core/exception.h"

namespace eng
{

/**
 * A container adaptor for a thread-safe FIFO queue.
 */
template<class T, class Container=std::deque<T>>
class ConcurrentQueue
{
    public:

        // member types
        using container_type = Container;
        using size_type = typename Container::size_type;
        using reference = typename Container::reference;

        /**
         * Construct an empty queue.
         */
        ConcurrentQueue()
            : container_(),
              mutex_(),
              empty_(true)
        { }

        /**
         * Check if the queue is empty.
         *
         * @returns
         *   True if queue is empty, else false.
         */
        bool empty() const
        {
            return empty_;
        }

        /**
         * Add an item to the end of the queue.
         *
         * @param args
         *   Arguments for object being places in queue, will be
         *   perfectly forwarded.
         */
        template<class... Args>
        void enqueue(Args&& ...args)
        {
            std::unique_lock lock(mutex_);

            container_.emplace_back(std::forward<Args>(args)...);

            empty_ = false;
        }

        /**
         * Tries to pop the end element off the queue.
         *
         * @param element
         *   Reference to store popped element.
         *
         * @returns
         *   True if an element could be dequeued, false otherwise.
         */
        bool try_dequeue(reference element)
        {
            auto dequeued = false;

            std::unique_lock lock(mutex_, std::try_to_lock);

            if(!empty_ && lock.owns_lock())
            {
                element = container_.front();
                container_.pop_front();

                empty_ = container_.empty();
                dequeued = true;
            }

            return dequeued;
        }

    private:

        /** Queue container. */
        container_type container_;
        
        /** Mutex for queue. */
        std::mutex mutex_;

        /** Flag indicating whether queue is empty. */
        std::atomic<bool> empty_;
};

}

