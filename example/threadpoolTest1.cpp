// FixedThreadPool线程池测试
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

#include "FixedThreadPool.hpp"
#include "Timestamp.hpp"

//测试4：比较析构顺序
#if 0
const int n = 10000;
const int m = 10000;

void RandVec1(vector<int> *pvec)
{
    assert(pvec != nullptr);
    for (int i = 0; i < m; ++i)
    {
        pvec->push_back(rand() % 10000);
    }
}
int main()
{
    //先创建的先析构
    std::vector<std::vector<int>> ivec1;
    pool::FixedThreadPool mypool(10000);
    ivec1.resize(n);
    for(int i=0;i<n;++i)
    {
        mypool.AddTask(std::bind(RandVec1, &ivec1[i]));
    }
    cout<<"\n=====================\n"
    <<endl;
}
#endif
//测试3：比较使用线程池和不使用线程池的性能差异，生成10000个长度为10000的随机整数向量，并对每个向量进行排序。测试1是单线程版本，测试2是使用线程池版本。最后输出生成数据和排序的耗时。
//优化后：使用线程池版本中，生成数据和排序的任务都提交到线程池中执行，主线程通过一个原子计数器来等待所有任务完成。每个任务在完成后都会将计数器加1，主线程通过轮询计数器的值来判断是否所有任务都完成了。这样可以避免主线程睡眠等待，提升效率。同时在生成数据时，使用了线程局部随机数生成器，避免了多线程环境下的竞争问题。

//门闩版本,最优化版本：使用std::latch来等待所有任务完成，避免了主线程轮询计数器的值，提升效率。同时在生成数据时，使用了线程局部随机数生成器，避免了多线程环境下的竞争问题。
pool::FixedThreadPool mypool(10000, 16);
const int n = 10000;
const int m = 10000;

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

        //编写的随机数
        (*pvec)[i] = rand() % 10000;
        //pvec->push_back(rand() % 10000);
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
        ivec1[i].resize(m);
        //ivec1[i].reserve(m);
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
    cout << "排序耗时: " << pool::diffMicro(end, begin) << " 微秒\n";
}

void RandVec2(vector<int> *pvec)
{
    assert(pvec != nullptr);
    //static thread_local mt19937 gen(rand());
    //uniform_int_distribution<> dis(0, 9999);

    for (int i = 0; i < m; ++i)
    {
        //库函数中生成的随机数
        //(*pvec)[i] = dis(gen);
        //pvec->push_back(dis(gen));

        //编写的随机数
        (*pvec)[i] = rand() % 10000;
        //pvec->push_back(rand() % 10000);
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
    cout << "===== 使用线程池 =====" << endl;
    pool::Timestamp begin, end;

    begin.now();

    for (int i = 0; i < n; ++i)
    {
        ivec2[i].resize(m);
        //ivec2[i].reserve(m);
        mypool.AddTask(bind(RandVec2, &ivec2[i]));
    }

    mylat.wait();
    end.now();
    cout << "生成数据耗时: " << pool::diffMicro(end, begin) << " 微秒" << endl;
    begin.now();

    for (int i = 0; i < n; ++i)
    {
        mypool.AddTask(bind(SortVec2, &ivec2[i]));
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
#if 0
pool::FixedThreadPool mypool(10000, 16);
const int n = 10000;
const int m = 10000;

vector<vector<int>> ivec1;
vector<vector<int>> ivec2;

atomic<int> g_finished = 0;

void RandVec1(vector<int> *pvec)
{
    assert(pvec != nullptr);
    for (int i = 0; i < m; ++i)
    {
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
        RandVec1(&ivec1[i]);
    }
    end.now();
    cout << "生成数据耗时: " << pool::diffMicro(end, begin) << " 微秒\n" << endl;

    cout << "单线程排序" << endl;
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
    static thread_local mt19937 gen(rand());
    uniform_int_distribution<> dis(0, 9999);

    for (int i = 0; i < m; ++i)
    {
        //(*pvec)[i] = dis(gen);
        pvec->push_back(dis(gen));
    }

    g_finished++;
}

void SortVec2(vector<int> *pvec)
{
    assert(pvec != nullptr);
    sort(pvec->begin(), pvec->end());
    g_finished++;
}

void test2()
{
    cout << "===== 使用线程池 =====" << endl;
    pool::Timestamp begin, end;

    g_finished = 0;
    begin.now();

    for (int i = 0; i < n; ++i)
    {
        //ivec2[i].resize(m);
        ivec2[i].reserve(m);
        mypool.AddTask(bind(RandVec2, &ivec2[i]));
    }

    while (g_finished < n)
        this_thread::yield();

    end.now();
    cout << "\n生成数据总耗时: " << pool::diffMicro(end, begin) << " 微秒" << endl;

    g_finished = 0;
    begin.now();

    for (int i = 0; i < n; ++i)
    {
        mypool.AddTask(bind(SortVec2, &ivec2[i]));
    }

    while (g_finished < n)
        this_thread::yield();

    end.now();
    cout << "排序总耗时: " << pool::diffMicro(end, begin) << " 微秒\n";
}

int main()
{
    ivec1.resize(n);
    ivec2.resize(n);

    test1();
    test2();
    return 0;
}
#endif
//优化前：
#if 0
pool::FixedThreadPool mypool(10000, 16);
const int n = 10000;
const int m = 10000;

vector<vector<int>> ivec1;
vector<vector<int>> ivec2;

atomic<int> g_finished = 0;
atomic<int> g_submitted = 0;

void RandVec1(vector<int> *pvec)
{
    assert(pvec != nullptr);
    for (int i = 0; i < m; ++i)
    {
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
        RandVec1(&ivec1[i]);
    }
    end.now();
    cout << "生成数据耗时: " << pool::diffMicro(end, begin) << " 微秒\n" << endl;

    cout<<"单线程排序"<<endl;
    begin.now();
    for(int i=0;i<n;++i)
    {
        ivec1[i].reserve(m);
        SortVec1(&ivec1[i]);
    }
    end.now();
    cout << "排序耗时: " << pool::diffMicro(end, begin) << " 微秒\n" << endl;
}

void RandVec2(vector<int> *pvec)
{
    assert(pvec != nullptr);
    for (int i = 0; i < m; ++i)
    {
        pvec->push_back(rand() % 10000);
    }
    g_finished++;
}

void SortVec2(vector<int> *pvec)
{
    assert(pvec != nullptr);
    sort(pvec->begin(), pvec->end());
    g_finished++;
}

void test2()
{
    cout << "===== 使用线程池 =====" << endl;
    pool::Timestamp begin, end;

    g_submitted = 0;
    g_finished = 0;

    begin.now();
    for (int i = 0; i < n; ++i)
    {
        ivec2[i].reserve(m);
        mypool.AddTask(bind(RandVec2, &ivec2[i]));
        g_submitted++;
    }

    while (g_finished < n)
    {
        this_thread::yield();
    }
    end.now();
    cout << "生成数据总耗时: " << pool::diffMicro(end, begin) << " 微秒" << endl;

    g_finished = 0;
    g_submitted = 0;
    begin.now();
    for(int i = 0; i < n; ++i)
    {
        mypool.AddTask(bind(SortVec2, &ivec2[i]));
        g_submitted++;
    }

    while (g_finished < n)
    {
        this_thread::yield();
    }
    end.now();
    cout << "排序总耗时: " << pool::diffMicro(end, begin) << " 微秒\n";
}

int main()
{
    ivec1.resize(n);
    ivec2.resize(n);

    test1();
    test2();

    return 0;
}
#endif
// 测试2：创建一个全局的线程池对象，线程池中的线程已经启动了，正在等待任务投递进来。定义一个函数Add(int a,int b)，这个函数就是一个任务，线程池中的线程会执行这个函数，输出a+b的结果。定义5个函数Add1()、Add2()、Add3()、Add4()、Add5()，每个函数都向线程池中投递2000个任务，这些任务就是调用Add函数并传入不同的参数i。主函数中创建5个线程，每个线程执行一个Add函数，等待这5个线程执行完后
#if 0
// 队列中以及确定任务类型为无返回值类型
int Add(int a, int b)
{
    return a + b;
}
void Add1()
{
    for (int i = 0; i < 2000; ++i)
    {
        // mypool.AddTask(std::bind(Add, i,i));
        auto a = mypool.submit(Add, i, i);
        cout << "Add(" << i << "," << i << ")=" << a.get() << endl;
    }
}
void Add2()
{
    for (int i = 2000; i < 4000; ++i)
    {
        // mypool.AddTask(std::bind(Add, i, i));
        auto a = mypool.submit(Add, i, i);
        cout << "Add(" << i << "," << i << ")=" << a.get() << endl;
    }
}
void Add3()
{
    for (int i = 4000; i < 6000; ++i)
    {
        auto a = mypool.submit(Add, i, i);
        cout << "Add(" << i << "," << i << ")=" << a.get() << endl;
    }
}
void Add4()
{
    for (int i = 6000; i < 8000; ++i)
    {
        auto a = mypool.submit(Add, i, i);
        cout << "Add(" << i << "," << i << ")=" << a.get() << endl;
    }
}
void Add5()
{
    for (int i = 8000; i < 10000; ++i)
    {
        // mypool.AddTask(std::bind(Add, i, i));
        auto a = mypool.submit(Add, i, i);
        cout << "Add(" << i << "," << i << ")=" << a.get() << endl;
    }
}

int main()
{
    thread tha(Add1);
    thread thb(Add2);
    thread thc(Add3);
    thread thd(Add4);
    thread the(Add5);
    tha.join();
    thb.join();
    thc.join();
    thd.join();
    the.join();
    std::this_thread::yield();

    return 0;
}
#endif
// 测试1：创建一个全局的线程池对象，线程池中的线程已经启动了，正在等待任务投递进来。定义一个原子变量ac作为线程安全的计数器，用来统计已经执行了多少个任务。定义一个函数func(int i)，这个函数就是一个任务，线程池中的线程会执行这个函数，输出func0、func1、...、func9999，并且每执行一个任务，计数器加1。定义5个函数Add1()、Add2()、Add3()、Add4()、Add5()，每个函数都向线程池中投递2000个任务，这些任务就是调用func函数并传入不同的参数i。主函数中创建5个线程，每个线程执行一个Add函数，等待这5个线程执行完后，主线程睡眠100毫秒，等待线程池中的线程执行完任务后再退出程序，否则可能会出现线程池中的线程还没有来得及执行任务就退出了程序，导致任务没有被执行的情况。最后输出已经执行了多少个任务，应该是10000。
#if 0
pool::FixedThreadPool mypool(20);//全局对象，main函数之前就已经创建了线程池对象，线程池中的线程已经启动了，正在等待任务投递进来
std::atomic<int>ac(0);//原子变量，线程安全的计数器，用来统计已经执行了多少个任务
void func(int i)
{
    cout<<"func"<<i<<endl;//这个函数就是一个任务，线程池中的线程会执行这个函数
    ++ac;//每执行一个任务，计数器加1
}
void Add1()
{
    for(int i=0;i<2000;++i)
    {
        mypool.AddTask(std::bind(func, i));//把func函数和参数i绑定成一个可调用对象，投递到线程池中，线程池中的线程会执行这个可调用对象，也就是执行func函数，输出func0、func1、...、func9999
        //mypool.AddTask([i]()->void{func(i);});
    }
}
void Add2()
{
    for(int i=2000;i<4000;++i)
    {
        mypool.AddTask(std::bind(func, i));
    }
}
void Add3()
{
    for(int i=4000;i<6000;++i)
    {
        mypool.AddTask(std::bind(func, i));
    }
}
void Add4()
{
    for(int i=6000;i<8000;++i)
    {
        mypool.AddTask(std::bind(func, i));
    }
}
void Add5()
{
    for(int i=8000;i<10000;++i)
    {
        mypool.AddTask(std::bind(func, i));
    }
}
int main()
{
    std::thread tha(Add1);
    std::thread thb(Add2);
    std::thread thc(Add3);
    std::thread thd(Add4);
    std::thread the(Add5);
    tha.join();
    thb.join();
    thc.join();
    thd.join();
    the.join();
    //std::this_thread::sleep_for(chrono::milliseconds(100));//主线程睡眠100毫秒，等待线程池中的线程执行完任务后再退出程序，否则可能会出现线程池中的线程还没有来得及执行任务就退出了程序，导致任务没有被执行的情况

    std::this_thread::yield();
    cout<<"ac:"<<ac.load()<<endl;//输出已经执行了多少个任务，应该是10000

    return 0;
}
#endif
