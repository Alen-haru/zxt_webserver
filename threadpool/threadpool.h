#ifndef THREADPOOL_H
#define THREADPOOL_H

#include <list>
#include <cstdio>
#include <exception>
#include <pthread.h>
#include "../lock/locker.h"
#include "../CGImysql/sql_connection_pool.h"


//线程池类，定义成模板类为了代码的复用
template <typename T>
class threadpool
{
public:
    /*thread_number是线程池中线程的数量，max_requests是请求队列中最多允许的、等待处理的请求的数量，connPool是数据库连接池指针*/
    threadpool(int actor_model, connection_pool *connPool, int thread_number = 8, int max_request = 10000);
    ~threadpool();
    //往工作队列中添加任务
    bool append(T *request, int state);
    bool append_p(T *request);

private:
    /*工作线程运行的函数，它不断从工作队列中取出任务并执行之*/
    static void *worker(void *arg);
    void run();

private:
    int m_thread_number;        //线程池中的线程数量
    int m_max_requests;         //请求队列中允许的最大（等待处理的）请求数量
    pthread_t *m_threads;       //线程池的数组，其大小为m_thread_number
    std::list<T *> m_workqueue; //请求队列
    locker m_queuelocker;       //保护请求队列的互斥锁
    sem m_queuestat;            //信号量用来判断是否有任务需要处理
    connection_pool *m_connPool;  //数据库，数据库连接指针
    int m_actor_model;          //模型切换
};
/*构造函数中创建线程池,
pthread_create函数中将类的对象作为参数传递给静态函数(worker),
在静态函数中引用这个对象,并调用其动态方法(run)。
具体的，
类对象传递时用this指针，传递给静态函数后，将其转换为线程池类，
并调用私有成员函数run。
*/
template <typename T>
threadpool<T>::threadpool( int actor_model, connection_pool *connPool, int thread_number, int max_requests) : m_actor_model(actor_model),m_thread_number(thread_number), m_max_requests(max_requests), m_threads(NULL),m_connPool(connPool)
{
    if (thread_number <= 0 || max_requests <= 0)
        throw std::exception();
    m_threads = new pthread_t[m_thread_number];
    if (!m_threads)
        throw std::exception();
    //创建thread_number个线程，并将它们设置为线程脱离
    /*int pthread_create 
        (pthread_t *thread_tid,             //返回新生成的线程的id
        const pthread_attr_t *attr,         //指向线程属性的指针,通常设置为NULL
        void * (*start_routine) (void *),   //处理线程函数的地址
        void *arg);                         //start_routine()中的参数
    函数原型中的第三个参数，为函数指针，指向处理线程函数的地址。
    该函数，要求为静态函数。
    如果处理线程函数为类成员函数时，需要将其设置为静态成员函数。
    */
    for (int i = 0; i < thread_number; ++i)
    {   //this作为参数传递到worker
        //循环创建线程，并将工作线程按要求进行运行
        if (pthread_create(m_threads + i, NULL, worker, this) != 0)
        {
            delete[] m_threads;
            throw std::exception();
        }
        //将线程进行分离后，不用单独对工作线程进行回收
        if (pthread_detach(m_threads[i]))
        {
            delete[] m_threads;
            throw std::exception();
        }
    }
}
template <typename T>
threadpool<T>::~threadpool()
{
    delete[] m_threads;
}
template <typename T>
bool threadpool<T>::append(T *request, int state)
{
    m_queuelocker.lock();
    if (m_workqueue.size() >= m_max_requests)
    {
        m_queuelocker.unlock();
        return false;
    }
    request->m_state = state;
    m_workqueue.push_back(request);
    m_queuelocker.unlock();
    m_queuestat.post();
    return true;
}
/*向请求队列中添加任务
通过list容器创建请求队列，向队列中添加时，
通过互斥锁保证线程安全，添加完成后通过信号量提醒有任务要处理，最后注意线程同步。
*/
template <typename T>
bool threadpool<T>::append_p(T *request)
{
    m_queuelocker.lock();
    //根据硬件，预先设置请求队列的最大值
    if (m_workqueue.size() >= m_max_requests)
    {
        m_queuelocker.unlock();
        return false;
    }
    //添加任务
    m_workqueue.push_back(request);
    m_queuelocker.unlock();
    //信号量提醒有任务要处理
    m_queuestat.post();
    return true;
}
/*
线程处理函数
内部访问私有成员函数run，完成线程处理要求。
*/
template <typename T>
void *threadpool<T>::worker(void *arg)
{   //将参数强转为线程池类，调用成员方法
    threadpool *pool = (threadpool *)arg;
    //线程池创建出来运行，从工作队列中取数据
    pool->run();
    return pool;
}
/*run执行任务
主要实现，工作线程从请求队列中取出某个任务进行处理，注意线程同步。*/
template <typename T>
void threadpool<T>::run()
{
    while (true)
    {   //从队列中取任务

        //判断有没有任务，有信号量-1，没有阻塞
         //信号量等待
        m_queuestat.wait();
        //有任务，上锁
        //被唤醒后先加互斥锁
        m_queuelocker.lock();
        //判断是否为空
        if (m_workqueue.empty())
        {   //解锁
            m_queuelocker.unlock();
            continue;
        }
        //获取第一个任务
        //从请求队列中取出第一个任务
        //将任务从请求队列删除
        T *request = m_workqueue.front();
        //取出然后删除
        m_workqueue.pop_front();
        //解锁
        m_queuelocker.unlock();
        //操作
        //获取到往下走
        if (!request)
            continue;
        if (1 == m_actor_model)
        {
            if (0 == request->m_state)
            {
                if (request->read_once())
                {
                    request->improv = 1;
                    connectionRAII mysqlcon(&request->mysql, m_connPool);
                    //process(模板类中的方法,这里是http类)进行处理
                    request->process();
                }
                else
                {
                    request->improv = 1;
                    request->timer_flag = 1;
                }
            }
            else
            {
                if (request->write())
                {
                    request->improv = 1;
                }
                else
                {
                    request->improv = 1;
                    request->timer_flag = 1;
                }
            }
        }
        else
        {
            connectionRAII mysqlcon(&request->mysql, m_connPool);
            request->process();
        }
    }
}
#endif
