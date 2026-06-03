

#include <cstdint>
#include <string>
using namespace std;

#ifndef TIMESTAMP_HPP
#define TIMESTAMP_HPP
namespace pool{
    class Timestamp
    {
    private:
        uint64_t micro_; // 微妙 // 1970/01/01 0:0:0 ----- 2026/01/27 14:34:45.xxxx
    public:
        Timestamp(uint64_t ms = 0);
        ~Timestamp();
        Timestamp(const Timestamp&)=default;
        Timestamp& operator=(const Timestamp&)=default;

        void swap(Timestamp &other);

        std::string toString() const;
        // 20260123.162323
        std::string toFormattedString(bool showms = true) const;
        // 2026/01/23 16:23:23.12233Z
        std::string toFormattedFile() const;//文件命名
        // 20260123_152323.2344//文件命名中不能有/会以为是分隔符

        bool valid() const;//判断时间戳是否有效，为0失效的时间戳，大于0有效的时间戳

        time_t getSecond() const;
        uint64_t getMills() const;
        uint64_t getMicro() const;
        const Timestamp &now();

        operator uint64_t() const;//将当前的转换为无符号整形

    public:
        static Timestamp Now();//no this;Timestamp::Now();
        static Timestamp Invalid();
        //转换函数
        static const int kMicS = 1000 * 1000; // s => mics;
        static const int kMilS = 1000;        // s => mill
    };

    inline time_t diffSecond(const Timestamp &a, const Timestamp &b)
    {
        return a.getSecond() - b.getSecond();
    }
    inline uint64_t diffMills(const Timestamp &a, const Timestamp &b)
    {
        return a.getMills() - b.getMills();
    }
    inline uint64_t diffMicro(const Timestamp &a, const Timestamp &b)
    {
        return a.getMicro() - b.getMicro();
    }

    inline Timestamp addTimeSecond(const Timestamp &a, time_t sec)
    {
        return Timestamp(a.getMicro() + sec * pool::Timestamp::kMicS);
    }
    inline Timestamp addTimeMills(const Timestamp &a, uint64_t mills)
    {
        return Timestamp(a.getMicro() + mills * pool::Timestamp::kMilS);
    }
    inline Timestamp addTimeMicors(const Timestamp &a, uint64_t micro)
    {
        return Timestamp(a.getMicro() + micro);
    }

}
// namespace pool
//  1s => 1000 mills
//  1s => 1000 * 1000 micro;
//  1s => 1000 * 1000 * 1000 nias;
#endif
