#include <iostream>
#include <thread>
#include <chrono>
#include <queue>
#include <condition_variable>
#include <boost/format.hpp>

// the objective here is to pass around some strings in a queue

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

void producer (int mswait, ProtectedQ<std::string> *q)
{
    int count = 0;
    std::string str;
    while(true)
    {
        count++;
        str = (boost::format("producer count %d") % count).str();
        q->addItem(str);
        std::this_thread::sleep_for(std::chrono::milliseconds(mswait));
    }
}

void consumer (ProtectedQ<std::string> *q)
{
    std::string str;
    while(true)
    {
        str = q->popItem();
        std::cout << "consumer heard: " << str << std::endl;
    }
}


int main(int argc, char* argv[])
{
    int count = 0;
    ProtectedQ<std::string> fifo;

    std::thread prod(&producer, 250, &fifo);
    std::thread con(&consumer, &fifo);
    
    while(true)
    {
        count++;
        std::cout << boost::format(" main count: %d") % count << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }

    return ~0;
}