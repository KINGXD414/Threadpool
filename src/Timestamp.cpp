
//Linux api
#include <sys/time.h>
//C++ STL
#include <string>
#include <sstream>
using namespace std;
//C api
#include <time.h>
#include <stdio.h>

#include "LogCommon.hpp"
#include "Timestamp.hpp"


namespace pool
{
    // Timestamp
    // uint64_t micro_; // 微妙

    Timestamp::Timestamp(uint64_t ms) : micro_(ms)
    {
    }
    Timestamp::~Timestamp()
    {
    }

    void Timestamp::swap(Timestamp &other)
    {
        std::swap(this->micro_, other.micro_);
    }
    std::string Timestamp::toString() const
    {
        char buff[pool::SMALL_BUFF_SIZE] = {};
        time_t ss = micro_ / kMicS;//微妙除1000000
        time_t ms = micro_ % kMicS;//微妙取余1000000
        sprintf(buff, "%lu.%lu", ss, ms);
        return buff;
    } // 123456667.160911
    std::string Timestamp::toFormattedString(bool showms) const
    {
        // micro_ 1234567654323452323434;/ kMS;
        char buff[pool::SMALL_BUFF_SIZE] = {};
        time_t ss = micro_ / kMicS;
        time_t ms = micro_ % kMicS;

        struct tm dtm = {};//定义本地结构体
        //gmtime_s(&ss,&dtm);//windows
        localtime_r(&ss, &dtm); // localtime
        //补齐
        int pos = sprintf(buff, "%04d/%02d/%02d-%02d:%02d:%02d",
                          dtm.tm_year + 1900,
                          dtm.tm_mon + 1, // 1
                          dtm.tm_mday,
                          dtm.tm_hour,
                          dtm.tm_min,
                          dtm.tm_sec);
        if (showms)
        {
            sprintf(buff + pos, ".%ldZ", ms);
        }
        return buff;
    }
    std::string Timestamp::toFormattedFile() const
    {
        char buff[SMALL_BUFF_SIZE] = {};
        time_t ss = micro_ / kMicS;
        time_t ms = micro_ % kMicS;

        struct tm dtm = {};
        localtime_r(&ss, &dtm); // localtime
        int pos = sprintf(buff, "%04d%02d%02d_%02d%02d%02d",
                          dtm.tm_year + 1900,
                          dtm.tm_mon + 1, // 1
                          dtm.tm_mday,
                          dtm.tm_hour,
                          dtm.tm_min,
                          dtm.tm_sec);
        sprintf(buff + pos, ".%ldZ", ms);
        return buff;

    } // string str("20260123_161209.1234Z")

    bool Timestamp::valid() const
    {
        return micro_ > 0;
    }

    time_t Timestamp::getSecond() const
    {
        return micro_ / kMicS;
    }
    uint64_t Timestamp::getMills() const
    {
        return micro_ / kMilS;
    }
    uint64_t Timestamp::getMicro() const
    {
        return micro_;
    }

    const Timestamp &Timestamp::now()
    {
        *this = Timestamp::Now();
        return *this;
    }

    Timestamp::operator uint64_t() const
    {
        return micro_;
    }
    Timestamp Timestamp::Now()
    {
        struct timeval tv;//#include<sys/time.h>
        gettimeofday(&tv, nullptr); // 1970 , 1 1, 08,0,0 ==> time; mills micro ; na
        uint64_t seconds = tv.tv_sec;
        return Timestamp(seconds * kMicS + tv.tv_usec);
    }

    Timestamp Timestamp::Invalid()
    {
        return Timestamp(0);//初始化了一个失效函数
    }

}
