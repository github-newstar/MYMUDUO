#pragma once

#include<unistd.h>
#include<sys/syscall.h>

namespace CurrentThread{
    extern __thread int t_cachedTId;
    void cachedTid();
    
    inline int tid(){
        if(__builtin_expect(t_cachedTId == 0, 0)){
            cachedTid();
        }
        return t_cachedTId;
    }
}