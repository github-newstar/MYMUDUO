 #include "Poller.h"
 #include <cstdlib>
 #include <stdlib.h>
/**
* 把Polle高层抽象层的实例实现拿出来，做到抽象不依赖于具体
*/
inline Poller* Poller::newDefaultPoller(EventLoop *Loop){
    if(::getenv("MUDUO_USE_POLL")){
        return nullptr;
    }else{
        return nullptr;
    }
}
