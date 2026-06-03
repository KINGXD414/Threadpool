#include "Timer.hpp"
#include "FixedThreadPool.hpp"
#include <sys/timerfd.h>
#include <unistd.h>
#include <iostream>
#include <cstring>


namespace pool
{
    // 静态成员初始化
    FixedThreadPool* Timer::s_threadPool = nullptr;

    Timer::Timer()
        : m_timerfd(-1), m_nextTag(1)
    {
    }

    Timer::~Timer()
    {
        close_time();
    }

    bool Timer::Init()
    {
        if (m_timerfd > 0)
        {
            return true;
        }
        m_timerfd = ::timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK);
        return m_timerfd >= 0;
    }

    bool Timer::settimer(size_t interval)
    {
        struct itimerspec new_value = {0};
        new_value.it_interval.tv_sec = interval / 1000;
        new_value.it_interval.tv_nsec = (interval % 1000) * 1000 * 1000;
        new_value.it_value = new_value.it_interval;

        if(::timerfd_settime(m_timerfd, 0, &new_value, nullptr) < 0)
        {
            fprintf(stderr, "settimer error: %s\n", strerror(errno));
            return false;
        }
        return true;
    }

    int Timer::addCallback(const TimerCallback &cb)
    {
        if (!cb) return -1;

        int tag = m_nextTag++;
        std::lock_guard<std::mutex> lock(m_mutex);
        m_callbacks[tag] = cb;
        return tag;
    }

    bool Timer::removeCallback(int tag)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto it = m_callbacks.find(tag);
        if (it != m_callbacks.end())
        {
            m_callbacks.erase(it);
            return true;
        }
        return false;
    }

    bool Timer::modifyCallback(int tag, const TimerCallback &cb)
    {
        if (!cb) return false;

        std::lock_guard<std::mutex> lock(m_mutex);
        auto it = m_callbacks.find(tag);
        if (it != m_callbacks.end())
        {
            it->second = cb;
            return true;
        }
        return false;
    }

    bool Timer::set_timer(const TimerCallback &cb, size_t interval)
    {
        if (m_timerfd <= 0) return false;

        if (!settimer(interval))
        {
            return false;
        }

        std::lock_guard<std::mutex> lock(m_mutex);
        m_callbacks.clear();
        if (cb)
        {
            m_callbacks[0] = cb;
        }
        return true;
    }

    bool Timer::reset_timer(size_t interval)
    {
        if (m_timerfd <= 0) return false;

        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_callbacks.empty())
        {
            return false;
        }
        return settimer(interval);
    }

    void Timer::handle_event()
    {
        uint64_t expire_cnt = 0;
        ssize_t n = read(m_timerfd, &expire_cnt, sizeof(expire_cnt));
        if (n != sizeof(expire_cnt))
        {
            return;
        }

        // 复制回调列表，避免持锁执行
        std::unordered_map<int, TimerCallback> callbacks_copy;
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            callbacks_copy = m_callbacks;
        }

        if (!callbacks_copy.empty())
        {
            if (s_threadPool)
            {
                for (auto& pair : callbacks_copy)
                {
                    if (pair.second)
                    {
                        s_threadPool->submit(pair.second);
                    }
                }
            }
            else
            {
                for (auto& pair : callbacks_copy)
                {
                    if (pair.second)
                    {
                        pair.second();
                    }
                }
            }
        }
    }

    int Timer::getfd() const
    {
        return m_timerfd;
    }

    bool Timer::close_time()
    {
        if (m_timerfd > 0)
        {
            close(m_timerfd);
            m_timerfd = -1;
            std::lock_guard<std::mutex> lock(m_mutex);
            m_callbacks.clear();
            return true;
        }
        return false;
    }
}
