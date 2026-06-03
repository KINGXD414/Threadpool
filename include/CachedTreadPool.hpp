// 用来把任务投递到线程池中，线程池中的线程会从任务队列中取出任务并执行
#include "syncQueue_3.hpp"
#include <functional>
#include <thread>
#include <list>
#include <atomic>
#include <future>
#include <memory>
#include <unordered_map>
using namespace std;

#ifndef CACHED_THREAD_POOL
#define CACHED_THREAD_POOL

namespace pool
{
    /*
    把各种各样要执行的逻辑，统一包装成同一种类型
    可以把任务存进容器（vector<TaskType>、队列）
    线程从队列里取出来，直接 task() 执行，不用关心任务原本是什么
    */
    using TaskType = std::function<void(void)>; // 包装器，把要执行的任务，包装成无参无返回值的可调用对象
    static const size_t InitThreadNums = 16;     // 初始线程个数
    class CachedTreadPool
    {
    private:
        std::unordered_map<std::thread::id, std::shared_ptr<std::thread>> m_threadgroup; // 线程组
        int m_coreThreadsize;                                                            // 核心线程数
        int m_maxThreadsize;                                                             // 最大线程数
        std::atomic_int m_idleThreadsize;                                                // 空闲线程数
        std::atomic_int m_curThreadsize;                                                 // 当前线程数
        std::mutex m_mutex;                                                              // 互斥锁
        std::condition_variable m_threadExit;                                            // 条件：不空就可以取任务，消费者
        pool::SyncQueue<TaskType> m_queue;                                               // 任务队列
        std::atomic_bool m_running;                                                      // 是否开始
        std::once_flag m_flag;                                                           // 保证线程池只能被启动一次
        int m_keyAliveTime; // 线程存活时间，单位为秒，超过这个时间没有任务执行，就让线程退出
        void Start(int numThreads)                                                       // 启动线程池,参数为线程池中线程的数量
        {
            m_running = true;             // 标记线程池开始运行
            m_curThreadsize = numThreads; // 当前线程数等于启动的线程数
            for (int i = 0; i < numThreads; ++i)
            {
                auto tha = std::make_shared<std::thread>(&CachedTreadPool::RunThread, this);// 创建线程对象，线程函数为RunThread，this指针传入RunThread函数中
                std::thread::id tid = tha->get_id();// 获取线程id
                m_threadgroup.emplace(tid, std::move(tha));// 把线程对象放进线程组中，线程id为key，线程对象为value
                m_idleThreadsize++; // 空闲线程数加1
            }
        }

        void RunThread() // 线程函数
        {
            auto tid=std::this_thread::get_id(); // 获取线程id
            time_t startTime=time(nullptr); // 记录线程开始执行任务的时间
            while (m_running) // 线程池正在运行
            {
                if(m_queue.Empty())
                {
                    time_t curnow=time(nullptr);
                    time_t intervalTime=curnow-startTime; // 计算线程空闲的时间
                    std::unique_lock<std::mutex>locker(m_mutex);
                    if(intervalTime>=m_keyAliveTime&&m_curThreadsize>m_coreThreadsize)
                    {
                        m_threadgroup.find(tid)->second->detach(); // 先把这个线程对象分离，让它自己退出，避免主线程等待它退出
                        //m_threadgroup.find(tid)->second->join();
                        m_threadgroup.erase(tid); // 从线程组中删除这个线程对象，释放资源
                        m_curThreadsize--; // 当前线程数减1
                        m_idleThreadsize--; // 空闲线程数减1
                        m_threadExit.notify_all(); // 通知等待的线程，线程池中有线程退出了
                        return; // 让线程退出
                    }
                }
                TaskType task;       // 定义一个任务对象
                if(m_running&&m_queue.Task(&task)==0) // 从任务队列中取出一个任务，如果取出成功了，就执行这个任务
                {
                    m_idleThreadsize--; // 空闲线程数减1
                    task(); // 执行任务
                    startTime=time(nullptr); // 记录线程执行完任务的时间
                    m_idleThreadsize++; // 空闲线程数加1
                }
            }

        }
        void StopThreadGroup() // 停止线程池
        {
            m_queue.WaitQueueEmptyStop(); // 等待队列清空，停止任务队列，唤醒所有等待的线程，让它们知道线程池要停止了
            m_coreThreadsize=0; // 核心线程数设为0，后面根据任务的情况动态添加线程
            m_keyAliveTime=0; // 线程存活时间设为0，后面根据任务的情况动态添加线程
            std::unique_lock<std::mutex> locker(m_mutex);
            while(!m_threadgroup.empty())
            {
                m_threadExit.wait_for(locker,std::chrono::milliseconds(100));
            }
            m_running = false; // 标记线程池停止运行
        }
        void AddnewThread() // 添加新线程
        {
            std::unique_lock<std::mutex> locker(m_mutex);
            if(m_idleThreadsize<=0&&m_curThreadsize<m_maxThreadsize) // 如果没有空闲线程了，并且当前线程数小于最大线程数了，就添加新线程
            {
                auto tha = std::make_shared<std::thread>(&CachedTreadPool::RunThread, this);// 创建线程对象，线程函数为RunThread，this指针传入RunThread函数中
                std::thread::id tid = tha->get_id();// 获取线程id
                m_threadgroup.emplace(tid, std::move(tha));// 把线程对象放进线程组中，线程id为key，线程对象为value
                m_curThreadsize++; // 当前线程数加1
                m_idleThreadsize++; // 空闲线程数加1
            }
        }

    public:
        CachedTreadPool(int initNumthreads = InitThreadNums, int taskQueueSize = MaxTaskCount,int keepAliveTime=5) // 获取系统逻辑内核个数
            : m_coreThreadsize(initNumthreads),
              m_maxThreadsize(std::thread::hardware_concurrency() + 2), // 计算密集性把逻辑核数+2；I/O密集型任务系统逻辑内核个数*2；
              m_idleThreadsize(0),                                      // 初始线程数为0，后面根据任务的情况动态添加线程
              m_curThreadsize(0),                                       // 当前线程数为0，后面根据任务的情况动态添加线程
              m_queue(taskQueueSize),                                   // 任务队列的大小，默认值为MaxTaskCount
              m_running(false),
              m_keyAliveTime(keepAliveTime)
        {
            Start(m_coreThreadsize); // 启动核心线程数的线程
        }
        ~CachedTreadPool()
        {
            if (m_running)
            {
                Stop();
            }
        }

        // 线程池的启动/停止，只能执行一次！
        void Stop() // 多线程环境下，Stop函数可能被调用多次，所以定义一个标志，第一次标志是未执行，执行一次过后标志设为已执行，然后后面每一次看到标志直接保证线程池只能被停止一次
        {
            std::call_once(m_flag, [this]() -> void
                           { StopThreadGroup(); });
        }
        void AddTask(const TaskType &task)
        {
            // 直接把任务对象放进任务队列，如果放不进去（可能是线程池停止了，或者队列满了），就直接执行这个任务
            if (m_queue.Put(task) != 0)
            {
                task();
            }
            AddnewThread(); // 根据任务的情况动态添加线程
        }
        void AddTask(TaskType &&task)
        {
            if (m_queue.Put(std::move(task)) != 0)
            {
                task();
            }
            AddnewThread();
        }
        template <class Func, class... Args>
        auto submit(Func &&func, Args &&...args)
        {
            // 任务提交接口，传入一个可调用对象和它的参数，返回一个future对象，调用者可以通过这个future对象获取任务的返回值
            using RetType = decltype(func(args...)); // 定义一个变量来存储任务的返回值
            auto task = std::make_shared<std::packaged_task<RetType()>>(
                std::bind(std::forward<Func>(func), std::forward<Args>(args)...));
            std::future<RetType> result = task->get_future(); // 获取这个任务对象的future对象，调用者可以通过这个future对象获取任务的返回值
            if (m_queue.Put([task]() mutable
                            { (*task)(); }) != 0) // 把这个任务对象包装成一个无参无返回值的可调用对象，投递到线程池中，线程池中的线程会执行这个可调用对象，也就是执行这个任务对象，输出func0、func1、...、func9999
            {
                cerr << "submit failed!" << endl;
                (*task)();
            }
            AddnewThread(); // 根据任务的情况动态添加线程
            return result;
        }
    };
}
#endif
