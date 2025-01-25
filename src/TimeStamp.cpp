#include"TimeStamp.h"
#include <cstdio>
#include <ctime>

TimeStamp::TimeStamp() : mircoSecondsSinceEpoch_(0) {}

TimeStamp::TimeStamp(int64_t mircoSecondsSinceEpoch)
    : mircoSecondsSinceEpoch_(mircoSecondsSinceEpoch) {}

TimeStamp TimeStamp::now(){
    return TimeStamp(time(NULL));
}

std::string TimeStamp::toString() const{
    char buf[128] = {0};
    tm *tm_time = localtime(&mircoSecondsSinceEpoch_);
    snprintf(buf, 128, "%4d/%02d/%02d %02d:%02d:%02d",
        tm_time->tm_year + 1990,
        tm_time->tm_mon + 1,
        tm_time->tm_mday,
        tm_time->tm_hour,
        tm_time->tm_min,
        tm_time->tm_sec);
    return buf;
}

/* int main(){
    std::cout<<TimeStamp::now().toString()<<std::endl;
    return 0;
} */