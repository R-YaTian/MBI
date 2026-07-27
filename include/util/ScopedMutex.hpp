#pragma once

#include <switch.h>

namespace app
{
    #define CONCATENATE_IMPL(s1, s2) s1##s2
    #define CONCATENATE(s1, s2) CONCATENATE_IMPL(s1, s2)
    #define ANONYMOUS_VARIABLE(pref) CONCATENATE(pref, __COUNTER__)

    struct ScopedMutex
    {
        ScopedMutex(Mutex* mutex) : m_mutex{mutex}
        {
            mutexLock(m_mutex);
        }

        ~ScopedMutex()
        {
            mutexUnlock(m_mutex);
        }

        ScopedMutex(const ScopedMutex&) = delete;
        void operator=(const ScopedMutex&) = delete;

    private:
        Mutex* const m_mutex;
    };

    #define SCOPED_MUTEX(_m) ScopedMutex ANONYMOUS_VARIABLE(SCOPE_EXIT_STATE_){_m}
}
