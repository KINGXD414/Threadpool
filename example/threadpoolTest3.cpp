// WorkStealingPool线程池测试
#include <thread>
#include <iostream>
#include <functional>
#include <vector>
#include <atomic>
#include <cassert>
#include <algorithm>
#include <random>
#include <chrono>
#include <latch>
using namespace std;

#include "WorkStealingPool.hpp"
#include "Timestamp.hpp"

//把大小改小，先保证能跑起来！
const int n = 10000;
const int m = 10000;

vector<vector<int>> vec_nopool;
vector<vector<int>> vec_steal;

void RandVec1(vector<int> *pvec)
{
    assert(pvec != nullptr);
    for (int i = 0; i < m; ++i)
    {
        pvec->push_back(rand() % 10000);
        //(*pvec)[i]=rand()%10000;
    }
}

void SortVec1(vector<int> *pvec)
{
    assert(pvec != nullptr);
    sort(pvec->begin(), pvec->end());
}

void test1()
{
    cout << "===== 不使用线程池 =====" << endl;
    pool::Timestamp begin, end;

    begin.now();
    for (int i = 0; i < n; ++i)
    {
        vec_nopool[i].reserve(m);
        //vec_nopool[i].resize(m);
        RandVec1(&vec_nopool[i]);
    }
    end.now();
    cout << "生成数据耗时: " << pool::diffMicro(end, begin) << " 微秒\n" << endl;

    begin.now();
    for (int i = 0; i < n; ++i)
    {
        SortVec1(&vec_nopool[i]);
    }
    end.now();
    cout << "排序耗时: " << pool::diffMicro(end, begin) << " 微秒\n" << endl;
}


void test2()
{
    cout << "===== 使用工作窃取线程池 =====" << endl;

    pool::WorkStealingPool mypool(10000,16);

    std::latch lat_steal(n);
    std::latch lat_sort_steal(n);

    pool::Timestamp begin, end;

    begin.now();
    for (int i = 0; i < n; ++i)
    {
        vec_steal[i].reserve(m);
        //vec_steal[i].resize(m);
        mypool.submit([&, pvec=&vec_steal[i]]()
        {
            static thread_local mt19937 gen(random_device{}());
            static thread_local uniform_int_distribution<> dis(0, 9999);

            for (int j = 0; j < m; j++)
            pvec->push_back(dis(gen));
            // (*pvec)[i]=dis(gen);
            lat_steal.count_down();
        });
    }
    lat_steal.wait();
    end.now();
    cout << "生成数据耗时: " << pool::diffMicro(end, begin) << " 微秒\n" << endl;

    begin.now();
    for (int i = 0; i < n; ++i)
    {
        mypool.submit([&, pvec=&vec_steal[i]]()
        {
            sort(pvec->begin(), pvec->end());
            lat_sort_steal.count_down();
        });
    }
    lat_sort_steal.wait();
    end.now();
    cout << "排序耗时: " << pool::diffMicro(end, begin) << " 微秒\n";
}

int main()
{
    //vec_nopool.resize(n);
    //vec_steal.resize(n);

    vec_nopool.reserve(n);
    vec_steal.reserve(n);

    test1();
    test2();

    return 0;
}
//测试1：测试线程池在高并发提交场景下的稳定性
//创建大量并发任务
#if 0
int add(int a, int b, int s)
{
    // clog << "add begin ..." << endl;
    int c = a + b;
    // std::this_thread::sleep_for(std::chrono::seconds(s));
    // clog << "add end .. " << endl;
    return c;
}
//测试任务提交,验证任务执行
void add_a(int ch, int x, int y, pool::WorkStealingPool& pool)
{
    auto r = pool.submit(add, x, y, rand() % 10);
    cout << "add_" << ch << " result: " << r.get() << endl;
}

int main()
{
    const int n = 2000;
    pool::WorkStealingPool mypool;  // 使用默认构造，注意这里没有参数
    std::vector<std::thread> tha;   // 使用vector动态管理线程
    tha.reserve(n);

    int success_count = 0;

    for (int i = 0; i < n; ++i)
    {
        try
        {
            tha.emplace_back(add_a, i, i + 20, i + 10, std::ref(mypool));
            success_count++;
        }
        catch (std::system_error &e)
        {
            cout << "Failed to create thread " << i << ": " << e.what() << endl;
            break;
        }
    }

    // 等待所有线程完成
    for (auto& t : tha)
    {
        if (t.joinable())
        {
            t.join();
        }
    }

    cout << "Successfully created threads: " << success_count << endl;

    // 等待线程池完成所有任务
    this_thread::sleep_for(chrono::milliseconds(100));
    mypool.stop();

    return 0;
}
#endif
