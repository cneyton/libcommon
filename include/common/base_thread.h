#ifndef BASE_THREAD_H
#define BASE_THREAD_H

#include "thread.h"

namespace common
{

/**
 * Thread class that runs into a parent scope and should keep a reference to it.
 *
 * @param P The parent class
 */
template<typename P>
class BaseThread: public Thread
{
protected:
    P * parent_;

public:
    BaseThread(P * parent): parent_(parent) {}

    virtual ~BaseThread() {}
};

} /* namesapce common */

#endif /* BASE_THREAD_H */
