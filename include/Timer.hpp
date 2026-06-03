
#include <functional>
#include <unordered_map>
#include <atomic>
#include <mutex>
using namespace std;

#ifndef TIMER_HPP
#define TIMER_HPP
// namespace pool { class ThreadPool; }
namespace pool
{
    class FixedThreadPool;
    using TimerCallback = std::function<void(void)>;

    class Timer
    {
    private:
        //TimerCallback m_callback;                           // 定时器的回调函数
        //int tag;                                            // 标记

        int m_timerfd;                                      // 定时器的ID
        std::unordered_map<int, TimerCallback> m_callbacks; // tag -> callback
        std::atomic<int> m_nextTag;                         // 下一个可用的tag
        mutable std::mutex m_mutex;                         // 保护 m_callbacks
        static FixedThreadPool *s_threadPool;
        bool settimer(size_t interval); // 设计函数，来设计定时器时间间隔

    public:
        Timer();
        ~Timer();
        // Timer(const Timer&)=delete;
        // Timer &operator=(const Timer&)=delete;

        // 移动函数
        // Timer(Timer &&);
        // Timer&operator=(Timer&&);

        // 初始化定时器
        bool Init();

        // 添加回调，返回 tag
        int addCallback(const TimerCallback &cb);

        // 根据 tag 删除回调
        bool removeCallback(int tag);

        // 修改指定 tag 的回调
        bool modifyCallback(int tag, const TimerCallback &cb);

        // 检查是否有回调
        bool hasCallbacks() const { return !m_callbacks.empty(); }

        // 获取回调数量
        size_t callbackCount() const {
            std::lock_guard<std::mutex> lock(m_mutex);
            return m_callbacks.size();
        }

        //兼容旧接口：清空所有回调，添加一个默认回调（tag=0）
        bool set_timer(const TimerCallback &cb, size_t interval);
        // 只设置定时器间隔（不修改回调）
        bool reset_timer(size_t interval);
        // 处理
        void handle_event();
        // 获得定时器
        int getfd() const;
        // 关闭定时器
        bool close_time();

        // 设置全局线程池
        static void setThreadPool(FixedThreadPool *pool)
        {
            s_threadPool = pool;
        }
    };
}
#endif
