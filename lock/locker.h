#ifndef LOCKER_H
#define LOCKER_H

#include <exception>
#include <pthread.h>
#include <semaphore.h>

//信号量类
class sem
{
public:
    sem()
    {
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
    ~sem()
    {
        sem_destroy(&m_sem);
    }
    //等待信号量
    bool wait()
    {
        return sem_wait(&m_sem) == 0;
    }
    //增加信号量
    bool post()
    {
        return sem_post(&m_sem) == 0;
    }

private:
    sem_t m_sem;
};
//线程同步机制封装类
//互斥锁类
class locker
{
public:
//构造函数
    locker()
    {
        if (pthread_mutex_init(&m_mutex, NULL) != 0)
        {   //抛出异常
            throw std::exception();
        }
    }
    //析构，销毁
    ~locker()
    {
        pthread_mutex_destroy(&m_mutex);
    }
    //锁的方式，上锁
    bool lock()
    {
        return pthread_mutex_lock(&m_mutex) == 0;
    }
    //解锁
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
//条件变量类
class cond
{
public:
    cond()
    {
        if (pthread_cond_init(&m_cond, NULL) != 0)
        {
            //pthread_mutex_destroy(&m_mutex);
            throw std::exception();
        }
    }
    ~cond()
    {
        pthread_cond_destroy(&m_cond);
    }
    //条件变量配合互斥锁使用
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
    //让所有线程唤醒
    bool broadcast()
    {
        return pthread_cond_broadcast(&m_cond) == 0;
    }

private:
    //static pthread_mutex_t m_mutex;
    pthread_cond_t m_cond;
};
#endif
