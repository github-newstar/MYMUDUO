#pragma once

/*
*noncopyable被继承以后，派生类对象可以正常的拷贝和构造，但派生类对象
*无法直接拷贝和赋值操作
*/
class noncopyable
{
public:
    noncopyable(const noncopyable &) = delete;
    void operator=(const noncopyable &) = delete;

protected:
    noncopyable() = default;
    ~noncopyable() = default;
};