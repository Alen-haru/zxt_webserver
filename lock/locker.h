#ifndef LOCKER_H
#define LOCKER_H

#include <exception>
#include <pthread.h>
#include <semaphore.h>

//信号量类
class sem
{
public:
    //构造函数
    //信号量初始化
    //sem_init函数用于初始化一个未命名的信号量
    sem()
    {   //信号量初始化
        //sem_init函数用于初始化一个未命名的信号量
        if (sem_init(&m_sem, 0, 0) != 0)
        {
            throw std::exception();
        }
    }
    sem(int num)
    {
        if (sem_init(&m_sem, 0, num) != 0)
        {
            throw std::exception();
        }
    }
    //析构函数
    ~sem()
    {   //信号量销毁
        sem_destroy(&m_sem);
    }
    //等待信号量，将以原子操作方式将信号量减一,信号量为0时,sem_wait阻塞
    bool wait()
    {
        return sem_wait(&m_sem) == 0;
    }
    //增加信号量，以原子操作方式将信号量加一,信号量大于0时,唤醒调用sem_post的线程
    bool post()
    {
        return sem_post(&m_sem) == 0;
    }

private:
    sem_t m_sem;//信号量
};
//线程同步机制封装类
//互斥锁类。互斥锁,也成互斥量,可以保护关键代码段,以确保独占式访问.当进入关键代码段,获得互斥锁将其加锁;离开关键代码段,唤醒等待该互斥锁的线程
class locker
{
public:
//构造函数，初始化互斥锁
    locker()
    {   //pthread_mutex_init函数用于初始化互斥锁
        if (pthread_mutex_init(&m_mutex, NULL) != 0)
        {   //抛出异常
            throw std::exception();
        }
    }
    //析构，销毁；用于销毁互斥锁
    ~locker()
    {   //用于销毁互斥锁
        pthread_mutex_destroy(&m_mutex);
    }
    //锁的方式，上锁；以原子操作方式给互斥锁加锁
    bool lock()
    {
        return pthread_mutex_lock(&m_mutex) == 0;
    }
    //解锁；以原子操作方式给互斥锁解锁
    bool unlock()
    {
        return pthread_mutex_unlock(&m_mutex) == 0;
    }
    //获得锁
    pthread_mutex_t *get()
    {
        return &m_mutex;
    }

private:
    //该类型的互斥锁
    pthread_mutex_t m_mutex;
};
//条件变量类：条件变量提供了一种线程间的通知机制,当某个共享数据达到某个值时,唤醒等待这个共享数据的线程.
class cond
{
public:
//用于初始化条件变量
    cond()
    {
        if (pthread_cond_init(&m_cond, NULL) != 0)
        {
            //pthread_mutex_destroy(&m_mutex);
            throw std::exception();
        }
    }
    //销毁条件变量
    ~cond()
    {
        pthread_cond_destroy(&m_cond);
    }
    //条件变量配合互斥锁使用
    /*
    //条件变量配合互斥锁使用
    pthread_cond_wait函数用于等待目标条件变量.
    该函数调用时需要传入 mutex参数(加锁的互斥锁) ,
    函数执行时,
    先把调用线程放入条件变量的请求队列,
    然后将互斥锁mutex解锁,
    当函数成功返回为0时,
    互斥锁会再次被锁上. 
    也就是说函数内部会有一次解锁和加锁操作.
    */
    bool wait(pthread_mutex_t *m_mutex)
    {
        int ret = 0;
        //pthread_mutex_lock(&m_mutex);
        ret = pthread_cond_wait(&m_cond, m_mutex);
        //pthread_mutex_unlock(&m_mutex);
        return ret == 0;
    }
    //超时时间
    bool timewait(pthread_mutex_t *m_mutex, struct timespec t)
    {
        int ret = 0;
        //pthread_mutex_lock(&m_mutex);
        ret = pthread_cond_timedwait(&m_cond, m_mutex, &t);
        //pthread_mutex_unlock(&m_mutex);
        return ret == 0;
    }
    //让一个或多个线程唤醒
    bool signal()
    {
        return pthread_cond_signal(&m_cond) == 0;
    }
    //让所有线程唤醒；以广播的方式唤醒所有等待目标条件变量的线程
    bool broadcast()
    {
        return pthread_cond_broadcast(&m_cond) == 0;
    }

private:
    //static pthread_mutex_t m_mutex;
    pthread_cond_t m_cond;
};
#endif
