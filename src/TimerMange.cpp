#include "TimerManage.hpp"

namespace pool
{

    // int m_epollfd;                            // 句柄
    // std::vector<struct epoll_event> m_events; // epoll_event events[100];//只要有事件发生就搜集事件,服务于句柄
    // static const size_t eventSize = 16;
    // std::unordered_map<int, std::shared_ptr<pool::Timer>> m_timers;
    // bool m_stop; // 停止标志
    // std::thread workerThread;
    TimerManage::TimerManage()
        : m_epollfd(-1), m_stop(false)
    {
        m_events.resize(eventSize);
    }
    TimerManage::~TimerManage()
    {
        set_stop();
    }

    // 禁止拷贝
    // TimerManage(const TimerManage &) = delete;
    // TimerManage &operator=(const TimerManage &) = delete;

    bool TimerManage::Init(int timeout, int poolSize)
    {
        bool ret = false;
        m_epollfd = ::epoll_create1(EPOLL_CLOEXEC);
        if (m_epollfd > 0)
        {
            // 创建你的线程池
            m_threadPool = std::make_unique<FixedThreadPool>(poolSize);
            // 设置到 Timer 类
            Timer::setThreadPool(m_threadPool.get());

            m_stop = false;
            workerThread = std::thread(&TimerManage::loop, this, timeout);
            ret = true;
        }
        return ret;
    }

    void TimerManage::loop(int timeout)
    {
        while (!m_stop)
        {
            int n = epoll_wait(m_epollfd, m_events.data(), m_events.size(), timeout);
            for (int i = 0; i < n; i++)
            {
                int fd = m_events[i].data.fd;
                auto it = m_timers.find(fd);
                if (it != m_timers.end())
                {
                    it->second->handle_event();
                }
            }
            if (n >= m_events.size())
            {
                m_events.resize(m_events.size() * 2);
            }
        }
    }
    void TimerManage::set_stop()
    {
        m_stop = true;
        if (workerThread.joinable())
        {
            workerThread.join();
        }
        close(m_epollfd);
        m_epollfd = -1;
    }

    // 定时任务接口
    void TimerManage::runAt(Timestamp time, TimerCallback cb)
    {
    }
    void TimerManage::runAfter(int delay, TimerCallback cb)
    {
    }
    void TimerManage::runEvery(int interval, TimerCallback cb)
    {
        std::shared_ptr<pool::Timer> pt = std::make_shared<pool::Timer>(); // 创建一个新的定时器对象，交给智能指针管理
        pt->Init();
        pt->set_timer(cb, interval);
        struct epoll_event evt = {};
        evt.data.fd = pt->getfd();
        evt.events = EPOLLIN;
        if (epoll_ctl(m_epollfd, EPOLL_CTL_ADD, pt->getfd(), &evt) < 0)
        {
            return;
        }
        pt->set_timer(cb, interval);
        m_timers[pt->getfd()] = pt;
        return;
    }
}
