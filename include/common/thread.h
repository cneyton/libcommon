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
     * @param wait_start set to true if the thread need to wait before executing
     */
    virtual void start_thread(bool wait_start)
    {
        thread_ = std::thread([this] {run();});
        if (wait_start) {
            std::unique_lock<std::mutex> lock(mutex_);
            cond_.wait(lock);
        }
    }

    void notify_thread_running(int error)
    {
        std::unique_lock<std::mutex> lock(mutex_);
        run_error_ = error;
        cond_.notify_all();
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

    int get_thread_error()     {return run_error_;}

    virtual void join()        {thread_.join();}

    virtual bool joinable()    {return thread_.joinable();}

    virtual void detach()      {thread_.detach();}

    virtual void cancel()      {canceled_ = true;}

    virtual bool is_canceled() {return canceled_;}

    void reset_thread()        {canceled_ = false;}

private:
    std::thread             thread_;
    std::atomic_bool        canceled_ = ATOMIC_VAR_INIT(false);
    std::mutex              mutex_;
    std::condition_variable cond_;
    int                     run_error_ = 0;
};

} /* namespace common */

#endif /* THREAD_H */
