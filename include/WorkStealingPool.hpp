#include "syncQueue_4.hpp"
#include <thread>
#include <memory>
#include <future>
#include <vector>
#include <deque>
#include <atomic>
using namespace std;

#ifndef WORK_STEALING_POOL_HPP
#define WORK_STEALING_POOL_HPP
namespace pool
{
    using TaskType = std::function<void(void)>; // 包装器，把要执行的任务，包装成无参无返回值的可调用对象

    class WorkStealingPool
    {
    private:
        const size_t m_numThreads;                               // 线程池中线程的数量
        pool::SyncQueue<TaskType> m_queue;                       // 工作窃取队列
        std::vector<std::shared_ptr<std::thread>> m_threadgroup; // 线程组
        std::atomic_bool m_running;                              // 是否开始
        std::once_flag m_flag;                                   // 保证线程池只能被启动一次

        // 线程运行逻辑 + 工作窃取
        void RunThread(const int index)
        {
            while (m_running)
            {
                std::deque<TaskType> qutask;
                int ret = m_queue.Task(&qutask, index);

                // 1. 成功拿到任务，执行
                if (ret == 0)
                {
                    for (auto &task : qutask)
                        if (task)
                            task();
                    continue;
                }

                // 2. 收到停止信号，退出循环
                if (ret == -2)
                {
                    break;
                }

                // 3. 超时没拿到任务，开始偷别的线程的任务
                bool stolen = false;
                for (int i = 0; i < m_numThreads; ++i)
                {
                    if (i == index)
                        continue;

                    std::deque<TaskType> tmp;
                    if (m_queue.Task(&tmp, i) == 0)
                    {
                        for (auto &t : tmp)
                            if (t)
                                t();
                        stolen = true;
                        break;
                    }
                }

                // 4. 没偷到任务，让出CPU，避免忙等
                if (!stolen)
                {
                    std::this_thread::yield();
                }
            }
        }

        void StopThreadGroup() // 停止线程池
        {
            m_queue.Stop();
            m_running = false;
            for (auto &tha : m_threadgroup)
            {
                if (tha && tha->joinable())
                {
                    tha->join();
                }
            }
            m_threadgroup.clear();
        }

        void Start(int numthreads) // 启动线程池,参数为线程池中线程的数量
        {
            m_running = true;
            m_threadgroup.reserve(numthreads); // 提前分配线程组的内存，避免在 emplace_back 时频繁扩容
            for (int i = 0; i < numthreads; ++i)
            {
                // 创建线程对象，线程函数为 WorkStealingPool::RunThread，this 指针传入 RunThread 函数中，同时传入线程索引i，以便 RunThread 知道自己是哪个队列
                m_threadgroup.emplace_back(
                    std::make_shared<std::thread>(&WorkStealingPool::RunThread, this, i));
            }
        }

        int threadIndex() // 有八个线程的话，返回值范围是0-7，轮询分配线程索引
        {
            static int num = 0;          // 线程安全的静态局部变量，C++11及以上标准保证其初始化是线程安全的
            return ++num % m_numThreads; // 轮询分配线程索引，返回值范围是0到m_numThreads-1
        }

    public:
        WorkStealingPool(const int qusize = MaxTaskCount,
                         const int numthreads = 16)
            : m_numThreads(numthreads),
              m_queue(m_numThreads, qusize),
              m_running(false)
        {
            Start(m_numThreads);
        }

        ~WorkStealingPool()
        {
            stop();
        }

        // 正确实现 stop() → 不会卡死
        void stop()
        {
            std::call_once(m_flag, [this]() -> void
                           { StopThreadGroup(); });
        }

        // 提交任务到指定队列
        void AddTask(const TaskType &task)
        {
            m_queue.Put(task, threadIndex());
        }
        void AddTask(TaskType &&task)
        {
            m_queue.Put(std::move(task), threadIndex());
        }

        // submit 完全保留你的写法
        template <class Func, class... Args>
        auto submit(Func &&func, Args &&...args)
        {
            using RetType = decltype(func(args...));
            auto task = std::make_shared<std::packaged_task<RetType()>>(
                std::bind(std::forward<Func>(func), std::forward<Args>(args)...));
            std::future<RetType> result = task->get_future();

            m_queue.Put([task]()
                        { (*task)(); }, threadIndex());
            return result;
        }
    };
}
#endif
