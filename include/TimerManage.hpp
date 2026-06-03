#ifndef TIMER_MANAGE_HPP
#define TIMER_MANAGE_HPP

#include "Timer.hpp"
#include <sys/epoll.h>
#include <vector>
#include <atomic>
#include <memory>
#include <thread>
#include <functional>
#include "Timestamp.hpp"
#include "FixedThreadPool.hpp"
namespace pool
{
    class TimerManage
    {
    private:
        int m_epollfd;                            // 句柄
        std::vector<struct epoll_event> m_events; // epoll_event events[100];//只要有事件发生就搜集事件,服务于句柄
        static const size_t eventSize = 16;
        std::unordered_map<int, std::shared_ptr<pool::Timer>> m_timers;
        bool m_stop; // 停止标志
        std::thread workerThread;

        std::unique_ptr<FixedThreadPool> m_threadPool;  // 使用你的线程池

        void loop(int timeout);

    public:
        TimerManage();
        ~TimerManage();

        // 禁止拷贝
        // TimerManage(const TimerManage &) = delete;
        // TimerManage &operator=(const TimerManage &) = delete;

        bool Init(int timeout,int poolSize = 16);
        void set_stop();

        // 定时任务接口
        void runAt(Timestamp time, TimerCallback cb);
        void runAfter(int delay, TimerCallback cb);
        void runEvery(int interval, TimerCallback cb);
    };
}
#endif
