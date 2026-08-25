#pragma once

#include <mutex>
#include <switch.h>

namespace
{
    class Lock
    {
        private:
            Mutex mutex;

        public:
            constexpr Lock() : mutex() {}

            void lock()
            {
                mutexLock(&this->mutex);
            }

            void unlock()
            {
                mutexUnlock(&this->mutex);
            }
    };

    using ScopedLock = std::scoped_lock<Lock>;

    #define CONCATENATE_IMPL(s1, s2) s1##s2
    #define CONCATENATE(s1, s2) CONCATENATE_IMPL(s1, s2)
    #define ANONYMOUS_VARIABLE(pref) CONCATENATE(pref, __COUNTER__)

    template<typename Function>
    struct ScopeGuard
    {
        ScopeGuard(Function&& function) : m_function(std::forward<Function>(function)) {}

        ~ScopeGuard()
        {
            m_function();
        }

        ScopeGuard(const ScopeGuard&) = delete;
        void operator=(const ScopeGuard&) = delete;

    private:
        const Function m_function;
    };

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

    #define ON_SCOPE_EXIT(_f) ScopeGuard ANONYMOUS_VARIABLE(SCOPE_EXIT_STATE_){[&] { _f; }};
    #define SCOPED_MUTEX(_m) ScopedMutex ANONYMOUS_VARIABLE(SCOPE_EXIT_STATE_){_m}
}
