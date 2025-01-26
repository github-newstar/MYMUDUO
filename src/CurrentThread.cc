#include "CurrentThread.h"

namespace CurrentThread{
    __thread int t_cachedTId = 0;
    void cachedTid(){
        if(t_cachedTId == 0){
            t_cachedTId = static_cast<pid_t>(::syscall(SYS_gettid));
        }
    }
}