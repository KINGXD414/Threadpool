// CachedTreadPool线程池测试
#include <thread>
#include <iostream>
#include <functional>
#include <vector>
#include <atomic>
#include <cassert>
#include <algorithm>
#include <random>
#include <latch>
using namespace std;

#include "CachedTreadPool.hpp"
#include "Timestamp.hpp"

const int n = 10000;
const int m = 10000;

pool::CachedTreadPool mypool(16, 10000);

vector<vector<int>> ivec1;
vector<vector<int>> ivec2;
std::atomic<int> numcount = 0;

std::latch mylat(n);
std::latch sortlat(n);

void RandVec1(vector<int> *pvec)
{
    assert(pvec != nullptr);
    for (int i = 0; i < m; ++i)
    {
        //(*pvec)[i] = rand() % 10000;
        pvec->push_back(rand() % 10000);
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
        //ivec1[i].resize(m);
        ivec1[i].reserve(m);
        RandVec1(&ivec1[i]);
    }
    end.now();
    cout << "生成数据耗时: " << pool::diffMicro(end, begin) << " 微秒\n" << endl;
    begin.now();
    for (int i = 0; i < n; ++i)
    {
        SortVec1(&ivec1[i]);
    }
    end.now();
    cout << "排序耗时: " << pool::diffMicro(end, begin) << " 微秒\n" << endl;
}

void RandVec2(vector<int> *pvec)
{
    assert(pvec != nullptr);
    //static thread_local mt19937 gen(rand());
    //uniform_int_distribution<> dis(0, 9999);

    for (int i = 0; i < m; ++i)
    {
        //(*pvec)[i] = dis(gen);
        //pvec->push_back(dis(gen));
        pvec->push_back(rand() % 10000);
    }
    mylat.count_down();
}

void SortVec2(vector<int> *pvec)
{
    assert(pvec != nullptr);
    sort(pvec->begin(), pvec->end());
    sortlat.count_down();
}

void test2()
{
    cout << "===== 使用缓存线程池 =====" << endl;
    pool::Timestamp begin, end;

    begin.now();
    for (int i = 0; i < n; ++i)
    {
        //ivec2[i].resize(m);
        ivec2[i].reserve(m);
        mypool.submit(bind(RandVec2, &ivec2[i]));
    }

    mylat.wait();
    end.now();
    cout << "生成数据耗时: " << pool::diffMicro(end, begin) << " 微秒\n" << endl;

    begin.now();
    for (int i = 0; i < n; ++i)
    {
        mypool.submit(bind(SortVec2, &ivec2[i]));
    }

    sortlat.wait();
    end.now();
    cout << "排序耗时: " << pool::diffMicro(end, begin) << " 微秒\n";
}

int main()
{
    ivec1.resize(n);
    ivec2.resize(n);

    test1();
    test2();
    return 0;
}
// 测试2:是否实现了核心的动态扩容、空闲回收和任务调度功能
#if 0
pool::CachedTreadPool mypool(2);//初始线程数为2，后面根据任务的情况动态添加线程
int add(int a, int b, int s)//这个函数也是一个任务，线程池中的线程会执行这个函数，输出a+b的结果，s是这个任务执行的时间，单位为秒
{
    std::this_thread::sleep_for(std::chrono::seconds(s));//模拟任务执行的时间，单位为秒
    int c = a + b;//计算a+b的结果
    cout << "add begin ..." << endl;//输出任务开始执行的提示信息
    return c;//返回a+b的结果
}
void add_a()//这个函数就是一个任务，线程池中的线程会执行这个函数，输出a+b的结果
{

    auto r = mypool.submit(add, 10, 20, 4);//把add函数和参数10、20、4绑定成一个可调用对象，投递到线程池中，线程池中的线程会执行这个可调用对象，也就是执行add函数，输出a+b的结果
    cout << "add_a: " << r.get() << endl;//通过get()获取add函数的返回值，也就是a+b的结果，并输出这个结果
}
void add_b()
{
    auto r = mypool.submit(add, 20, 30, 6);
    cout << "add_b: " << r.get() << endl;
}
void add_c()
{
    auto r = mypool.submit(add, 30, 40, 1);
    cout << "add_c: " << r.get() << endl;
}
void add_d()
{
    auto r = mypool.submit(add, 10, 40, 9);
    cout << "add_d: " << r.get() << endl;
}
int main()
{
    std::thread tha(add_a);//创建一个线程，执行add_a函数，add_a函数会向线程池中投递一个任务，这个任务就是调用add函数并传入参数10、20、4，线程池中的线程会执行这个任务，输出add_a: 30
    std::thread thb(add_b);
    std::thread thc(add_c);
    std::thread thd(add_d);
    tha.join();
    thb.join();
    thc.join();
    thd.join();
    std::this_thread::sleep_for(std::chrono::seconds(20));
    std::thread the(add_a);//再次提交一个任务，看看线程池能不能动态添加线程来执行这个任务
    std::thread thf(add_b);
    the.join();
    thf.join();
    return 0;
}
#endif
// 测试1：线程池完整功能 + 正确性 + 稳定性
/*
线程池能否正确提交并执行两种不同类型的任务
带返回值的任务（std::future）是否能正常获取结果
线程池在高并发任务下是否稳定、无死锁 / 崩溃
*/
#if 0
void func(int index)//这个函数就是一个任务，线程池中的线程会执行这个函数
{
    static int num = 0;
    cout << "func_" << index << " num: " << ++num << endl;
}
int add(int a, int b)//这个函数也是一个任务，线程池中的线程会执行这个函数，输出a+b的结果
{
    return a + b;
}
int main()
{
    pool::CachedTreadPool mypool;//
    for (int i = 0; i < 1000; ++i)
    {
        if (i % 2 == 0)
        {
            auto pa = mypool.submit(add, i, i + 1);//把add函数和参数i、i+1绑定成一个可调用对象，投递到线程池中，线程池中的线程会执行这个可调用对象，也就是执行add函数，输出a+b的结果
            cout << pa.get() << endl;
        }
        else
        {
            mypool.submit(func, i);//把func函数和参数i绑定成一个可调用对象，投递到线程池中，线程池中的线程会执行这个可调用对象，也就是执行func函数，输出func0、func1、...、func999
        }
    }
}
#endif
