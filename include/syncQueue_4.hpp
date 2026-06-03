#include <deque>
#include <mutex>
#include <vector>
#include <assert.h>
#include <utility>
#include <condition_variable>
using namespace std;
#ifndef SYNC_QUEUE_4_HPP
#define SYNC_QUEUE_4_HPP

namespace pool
{
    const size_t MaxTaskCount = 500; // 最大任务数
    template <class T>               // 模板类T，在初始化同步队列时就已经给定
    class SyncQueue                  // 以下都为模板T的模板方法
    {
    private:
        std::vector<std::deque<T>> m_queue; // 有自己的队列数组，每个线程池线程对应一个队列
        size_t m_bucketSize;                // 队列数组的大小，vector大小线程池线程数,要配对
        size_t m_maxSize;                   // 每个队列的上限,deque大小
        mutable std::mutex m_mutex;         // 去常性互斥锁，mutable修饰的成员变量可以在const成员函数中被修改
        std::condition_variable m_notEmpty; // 条件：不空就可以取任务，消费者
        std::condition_variable m_notFull;  // 条件：不满就可以放任务，生产者
        std::condition_variable m_waitStop;

        int m_waitTime;
        bool m_needStop; // 为真，同步队列停止工作
        int m_timeout;   // 超时时间，单位毫秒

        // 内部私有接口
        bool IsFull(const int index) const // 判满
        {
            bool full = m_queue[index].size() >= m_maxSize; // 当此时任务数大于最大值
            return full;
        }
        bool IsEmpty(const int index) const // 判空
        {
            bool empty = m_queue[index].empty();
            return empty;
        }
        template <class F>                 // 模板F，F与T要进行兼容性检查
        int Add(F &&task, const int index) // 添加任务，这个&&代表的是引用型别未定义
        {
            // 加锁
            std::unique_lock<std::mutex> locker(m_mutex);

            while (!m_needStop && IsFull(index))
            {
                std::cv_status tag = m_notFull.wait_for(locker, std::chrono::milliseconds(m_timeout));

                if (tag == std::cv_status::timeout) // 如果等待了时间超时了，自己唤醒返回-1;
                {
                    return -1;
                }
            }
            if (m_needStop)
                return -2; // 如果停止了，就不添加了，直接返回-2
            // m_queue[index].push_back(std::forward<F>(task));
            m_queue[index].emplace_back(std::forward<F>(task)); // 原位构造
            m_notEmpty.notify_all();
            return 0; // 添加成功，返回0
        }

    public:
        SyncQueue(int bucketsize, int maxsize = MaxTaskCount, int timeout = 1)
            : m_bucketSize(bucketsize),
              m_maxSize(maxsize),
              m_timeout(timeout),
              m_waitTime(timeout),
              m_needStop(false) // runing
        {
            m_queue.resize(m_bucketSize); // 队列数组的大小，vector大小线程池线程数,要配对
        }
        ~SyncQueue() // 析构
        {
            if (!m_needStop) // 如果没有停止，就停止
            {
                Stop();
            }
        }

        int Put(const T &task, const int index) // 万能引用
        {
            return Add(task, index); // 传入一个const T类型的任务对象，调用Add方法添加任务
        }
        int Put(T &&task, const int index) // 万能引用
        {
            return Add(std::forward<T>(task), index); // 传入一个const T类型的任务对象，调用Add方法添加任务
        }

        int Task(std::deque<T> *pdeq, const int index) // 写数据、投任务、批量塞入
        {
            assert(pdeq != nullptr);
            std::unique_lock<std::mutex> locker(m_mutex);
            // 不为空
             while (!m_needStop && IsEmpty(index))
            {
                auto tag = m_notEmpty.wait_for(locker, std::chrono::milliseconds(m_waitTime));
                if (tag == std::cv_status::timeout)
                {
                    return -1; // 等待了时间超时了，自己唤醒返回-1
                }
            }
            if (m_needStop)
                return -2; // 如果停止了，就不取了，直接返回-2
            *pdeq = std::move(m_queue[index]); // 将任务一次取
            m_notFull.notify_all();
            m_waitStop.notify_all();
            return 0;
        }
        int Task(T *ps, const int index) // 传入一个指向单个T对象的指针
        {
            assert(ps != nullptr); // Debug版本有用
            // 加锁
            std::unique_lock<std::mutex> locker(m_mutex);
            while (!m_needStop && IsEmpty(index))
            {
                auto tag = m_notEmpty.wait_for(locker, std::chrono::milliseconds(m_waitTime));
                if (tag == std::cv_status::timeout)
                {
                    return -1; // 等待了时间超时了，自己唤醒返回-1
                }
            }
            if (m_needStop)
                return -2; // 如果停止了，就不取了，直接返回-2

            *ps = m_queue[index].front(); // 一次取一个
            m_queue[index].pop_front();   // 取完了就弹出
            m_notFull.notify_all();
            m_waitStop.notify_all();
            return 0; // 取任务成功，返回0
        }

        // 多个线程访问同一个变量，一定要加锁！
        void Stop() // 停止工作
        {
            // 加锁不是为了只停一次，是为了安全修改多线程共享的停止标记
            {
                std::unique_lock<std::mutex> locker(m_mutex); // 加锁
                m_needStop = true;                            // 标记停止
            }
            m_notEmpty.notify_all();
            m_notFull.notify_all();
        }
        void WaitQueueEmptyStop() // 等待队列清空
        {
            std::unique_lock<std::mutex> locker(m_mutex);
            for (int i = 0; i < m_bucketSize; ++i)
            {
                while (!IsEmpty(i))
                {

                    m_waitStop.wait_for(locker, std::chrono::milliseconds(m_waitTime)); // 等待的条件是队列不空，等待了时间超时了，自己唤醒继续判断，如果队列空了，就退出循环
                }
            }
            m_needStop = true;
            m_notEmpty.notify_all();
            m_notFull.notify_all();
        }
        // 外部判空判满接口
        bool Empty(const int index) const // 判空
        {
            std::unique_lock<std::mutex> locker(m_mutex);
            return m_queue[index].empty();
        }
        bool Full(const int index) const // 判满
        {
            std::unique_lock<std::mutex> locker(m_mutex);
            return m_queue[index].size() >= m_maxSize;
        }

        size_t Size(const int index) const // 返回当前任务数
        {
            std::unique_lock<std::mutex> locker(m_mutex); // 加锁要被修改，常方法不能被修改
            return m_queue[index].size();
        }
        size_t Count(const int index) const // 返回当前任务数
        {
            // 线程不安全，外部调用时要注意
            return m_queue[index].size();
        }
        size_t TotalTaskNum()const
        {
            std::unique_lock<std::mutex> locker(m_mutex);
            size_t total = 0;
            for (int i = 0; i < m_bucketSize; ++i)
            {
                total += m_queue[i].size();
            }
            return total;
        }
        void PintTaskInfo()const
        {
            std::unique_lock<std::mutex>locker(m_mutex);
            for(int i=0;i<m_bucketSize;i++)
            {
                printf("%d 桶中的任务数：%d\n",i,m_queue[i].size());
            }
            printf("\n====================================\n");
        }
    };
}
#endif
