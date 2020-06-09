#ifndef PROTECTED_Q_HPP
#define PROTECTED_Q_HPP

#include <thread>
#include <queue>
#include <condition_variable>

template <typename T>
class ProtectedQ
{
    private:
        std::queue<T> q;
        std::condition_variable cond;
        mutable std::mutex m;

    public:
        // the object calls (q(), cond() & m()) initialize all the members properly
        ProtectedQ(): q(), cond(), m()
        {
            // constructor
        }
        void addItem(T item)
        {
            // locks mutex in this code block
            std::unique_lock<std::mutex> lock(m);
            // add item to queue
            q.push(item);
            // notify thread waiting on condition
            cond.notify_one();
        }

        T popItem()
        {
            std::unique_lock<std::mutex> lock(m);
            while(q.empty())
            {
                // release lock as long as the wait and reaquire it afterwards.
                cond.wait(lock);
            }
            T val = q.front(); // get the front element from queue
            q.pop(); // remove the front element from queue
            return val;
        }
};

#endif // PROTECTED_Q_HPP