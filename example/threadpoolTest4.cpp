// ScheduleThreadPool线程池测试
#include <thread>
#include <iostream>
#include <chrono>
using namespace std;

#include "Timestamp.hpp"
#include "Timer.hpp"
#include "TimerManage.hpp"
//测试3：1:n回调和1：1
#if 0
void test_one_to_one()
{
    pool::TimerManage mytimer;
    cout << "\n===== 1:1 模式（每个定时器单独创建） =====" << endl;
    mytimer.Init(100, 8);

    atomic<int> executedCount(0);
    pool::Timestamp begin, end;

    const int TIMER_COUNT = 1000;  // 1000个定时器
    const int DURATION_SEC = 5;

    begin.now();

    // 每个定时器只有一个回调
    for (int i = 0; i < TIMER_COUNT; ++i)
    {
        mytimer.runEvery(100, [&]() {
            executedCount++;
        });
    }

    this_thread::sleep_for(chrono::seconds(DURATION_SEC));

    end.now();

    int expected = TIMER_COUNT * (DURATION_SEC * 1000 / 100);

    cout << "定时器数量: " << TIMER_COUNT << endl;
    cout << "实际执行次数: " << executedCount.load() << endl;
    cout << "预期执行次数: " << expected << endl;
    cout << "执行率: " << (executedCount.load() * 100.0 / expected) << "%" << endl;
    cout << "耗时: " << pool::diffMicro(end, begin) / 1000 << " ms" << endl;
}

void test_one_to_many()
{
    cout << "\n===== 1:N 模式（直接使用 Timer 类） =====" << endl;

    atomic<int> executedCount(0);
    pool::Timestamp begin, end;

    const int CALLBACK_COUNT = 1000;
    const int DURATION_SEC = 5;

    begin.now();

    // 直接使用 Timer 类
    auto timer = make_shared<pool::Timer>();
    timer->Init();
    timer->set_timer(nullptr, 100);  // 设置间隔，不设回调

    // 添加1000个回调
    for (int i = 0; i < CALLBACK_COUNT; ++i)
    {
        timer->addCallback([&]() {
            executedCount++;
        });
    }

    // 简单的事件循环
    auto start = chrono::steady_clock::now();
    while (chrono::duration_cast<chrono::seconds>(
        chrono::steady_clock::now() - start).count() < DURATION_SEC)
    {
        timer->handle_event();
        this_thread::sleep_for(chrono::milliseconds(1));
    }

    end.now();

    int expected = CALLBACK_COUNT * (DURATION_SEC * 1000 / 100);

    cout << "回调数量: " << CALLBACK_COUNT << endl;
    cout << "定时器数量: 1 个" << endl;
    cout << "实际执行次数: " << executedCount.load() << endl;
    cout << "预期执行次数: " << expected << endl;
    cout << "执行率: " << (executedCount.load() * 100.0 / expected) << "%" << endl;
    cout << "耗时: " << pool::diffMicro(end, begin) / 1000 << " ms" << endl;
}

// 测试3: 对比资源占用
void test_resource_usage()
{
    cout << "\n===== 资源占用对比 =====" << endl;

    const int COUNT = 1000;

    // 1:1 模式 - 需要1000个timerfd
    cout << "\n1:1 模式:" << endl;
    cout << "  - timerfd 数量: " << COUNT << " 个" << endl;
    cout << "  - epoll 事件数: " << COUNT << " 个" << endl;
    cout << "  - 内存占用: ~" << COUNT * 200 << " 字节" << endl;

    // 1:N 模式 - 只需要1个timerfd
    cout << "\n1:N 模式:" << endl;
    cout << "  - timerfd 数量: 1 个" << endl;
    cout << "  - epoll 事件数: 1 个" << endl;
    cout << "  - 内存占用: ~" << COUNT * 32 + 200 << " 字节" << endl;

    cout << "\n资源节省:" << endl;
    cout << "  - timerfd 节省: " << COUNT - 1 << " 个" << endl;
    cout << "  - epoll 事件节省: " << COUNT - 1 << " 个" << endl;
    cout << "  - 内存节省: ~" << (COUNT * 200 - (COUNT * 32 + 200)) << " 字节" << endl;
}

int main()
{
    cout << "========================================" << endl;
    cout << "   1:1 vs 1:N 对比测试" << endl;
    cout << "========================================" << endl;

    test_one_to_one();
    test_one_to_many();
    test_resource_usage();

    cout << "\n========================================" << endl;
    cout << "测试完成!" << endl;
    cout << "========================================" << endl;

    return 0;
}
#endif

//测试2：定时器精度
//#if 0
using namespace std;
using namespace pool;

void testPrecision()
{
    TimerManage timer;
    timer.Init(100, 4);

    atomic<int> count = 0;
    atomic<long long> totalError = 0;
    chrono::steady_clock::time_point lastTime;

    // 记录第一次执行时间
    bool first = true;

    timer.runEvery(1000, [&]()
                   {
        auto now = chrono::steady_clock::now();

        if (first) {
            lastTime = now;
            first = false;
        } else {
            // 计算实际间隔与理论间隔的误差
            auto elapsed = chrono::duration_cast<chrono::microseconds>(now - lastTime);
            long long error = abs(elapsed.count() - 1000000);  // 理论1000ms = 1,000,000us
            totalError += error;
            count++;
            lastTime = now;

            if (count % 10 == 0) {
                cout << "Average error after " << count << " times: "
                     << totalError / count << " us" << endl;
            }
        } });

    this_thread::sleep_for(chrono::seconds(60));
}

int main()
{
    testPrecision();
    return 0;
}
//#endif

// 测试1：多个计时器，延时测试
#if 0
pool::TimerManage mytimer;

void heavyTask() {
    static int cnt = 0;
    cout << "Heavy task START " << cnt << " at "
         << chrono::duration_cast<chrono::milliseconds>(
                chrono::system_clock::now().time_since_epoch()
            ).count() << endl;

    this_thread::sleep_for(chrono::seconds(2));  // 耗时2秒

    cout << "Heavy task END " << cnt++ << " at "
         << chrono::duration_cast<chrono::milliseconds>(
                chrono::system_clock::now().time_since_epoch()
            ).count() << endl;
}

void fastTask(int id) {
    static int cnt = 0;
    cout << "Fast task " << id << " count:" << cnt++ << " at "
         << chrono::duration_cast<chrono::milliseconds>(
                chrono::system_clock::now().time_since_epoch()
            ).count() << endl;
}

int main()
{
    // 初始化：epoll超时100ms，线程池4个线程
    mytimer.Init(100, 4);

    // 耗时任务 - 不会阻塞其他定时器
    mytimer.runEvery(3000, heavyTask);

    // 快速任务 - 不受影响
    mytimer.runEvery(1000, std::bind(fastTask, 1));
    mytimer.runEvery(2000, std::bind(fastTask, 2));

    cout << "Timer started with thread pool!" << endl;

    while(true) {
        this_thread::sleep_for(chrono::seconds(1));
    }

    return 0;
}
#endif
