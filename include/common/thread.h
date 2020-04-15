#ifndef THREAD_H
#define THREAD_H

#include <thread>
#include <atomic>
#include <mutex>
#include <condition_variable>

namespace common
{

/**
 * A thread is a thread of execution in a program.
 */
class Thread
{
public:
    Thread(): run_(false) {}
    virtual ~Thread() {}

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
        thread_ = std::thread([this] {run();});
        if (wait_start) {
            std::unique_lock<std::mutex> lock(mutex_);
            cond_.wait(lock);
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

    int get_thread_error()    {return run_error_;}

    virtual void join()       {thread_.join();}

    virtual bool joinable()   {return thread_.joinable();}

    virtual void detach()     {thread_.detach();}

    virtual bool is_running() const {return run_;}

protected:
    void notify_running(int error)
    {
        std::unique_lock<std::mutex> lock(mutex_);
        run_error_ = error;
        cond_.notify_all();
    }

private:
    std::thread             thread_;
    std::atomic_bool        run_;
    std::mutex              mutex_;
    std::condition_variable cond_;
    int                     run_error_ = 0;
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
    virtual ~BaseThread() {}

protected:
    P * parent_;
};

} /* namespace common */

#endif /* THREAD_H */
