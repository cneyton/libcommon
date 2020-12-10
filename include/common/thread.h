#pragma once

#include <thread>
#include <atomic>
#include <mutex>
#include <condition_variable>

namespace common {

/**
 * A thread is a thread of execution in a program.
 */
class Thread
{
public:
    virtual ~Thread() = default;

    /**
     * Causes this thread to begin execution.
     *
     * The result is that two threads are running concurrently: the current
     * thread (which returns from the call to the start method) and the other
     * thread (which executes its run method).
     *
     * It is never legal to start a thread more than once. In particular, a
     * thread may not be restarted once it has completed execution.
     *
     * @param wait_start set to true to block the caller until a notification is sent
     */
    virtual void start(bool wait_start)
    {
        run_ = true;
        std::unique_lock<std::mutex> lk(mutex_);
        thread_ = std::thread([this] {run();});
        if (wait_start) {
            started_ = false;
            cond_.wait(lk, [this]{return started_;});
        }
    }

    /**
     * Subclasses of Thread should override this method.
     *
     * Starting the thread causes the object's run method to be called in that
     * separately executing thread.
     *
     * The general contract of the method run is that it may take any action whatsoever.
     */
    virtual void run() = 0;

    virtual void stop()       {run_ = false;}
    virtual void join()       {thread_.join();}
    virtual void detach()     {thread_.detach();}
    virtual bool joinable()   const {return thread_.joinable();}
    virtual bool is_running() const {return run_;}

protected:
    void notify_running()
    {
        std::lock_guard<std::mutex> lk(mutex_);
        started_ = true;
        cond_.notify_all();
    }

private:
    std::thread             thread_;
    std::atomic_bool        run_ = false;
    std::mutex              mutex_;
    std::condition_variable cond_;
    bool                    started_;
};

/**
 * Thread class that runs into a parent scope and should keep a reference to it.
 *
 * @param P The parent class
 */
template<typename P>
class BaseThread: public Thread
{
public:
    BaseThread(P * parent): parent_(parent) {}
    virtual ~BaseThread() = default;

protected:
    P * parent_;
};

} /* namespace common */
