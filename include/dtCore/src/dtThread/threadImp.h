/*!
 \file      threadImp.h
 \brief     POSIX thread, semaphore, mutex implementation
 \author    ChanMook Lim, cm.Lim@hyundai.com
 \author    Dong-hyun Lee, lee.dh@hyundai.com
 \date      2023. 08. 23
 \version   1.0.0
 \copyright RoboticsLab ART All rights reserved.
*/

#ifndef __DT_THREAD_THREADIMP_H__
#define __DT_THREAD_THREADIMP_H__

/** \addtogroup dtThread
 *
 */

//* C/C++ System Headers -----------------------------------------------------*/
extern "C"
{
#if defined(_WIN32) || defined(__CYGWIN__)
#include <windows.h>
//#include <thread>
//#include <mutex>
// #include <semaphore>
#else
#include <pthread.h>
#include <unistd.h>
#if defined(__APPLE__)
#include <sys/types.h>
#include <sys/sysctl.h>
#include <mach/thread_act.h>
#include <dispatch/dispatch.h> // semaphore
#include <pthread.h>
#else
#include <pthread.h>
#include <semaphore.h>
#endif
#endif
}

//* Other Lib Headers --------------------------------------------------------*/
//* Project Headers ----------------------------------------------------------*/
//* System-Specific Headers --------------------------------------------------*/

namespace dt
{
namespace Thread
{
//* Public(Exported) Macro ---------------------------------------------------*/
//* Public(Exported) Types ---------------------------------------------------*/
#if defined(_WIN32) || defined(__CYGWIN__)
    using dt_thread_t = HANDLE; //std::thread;
    using dt_mutex_t = HANDLE; //std::mutex;
    using dt_sem_t = HANDLE; //counting_semaphore;
#elif defined(__APPLE__)
    using dt_thread_t = pthread_t; // std::thread;
    using dt_mutex_t = pthread_mutex_t;
    using dt_sem_t = dispatch_semaphore_t;
#else
    using dt_thread_t = pthread_t;
    using dt_mutex_t = pthread_mutex_t;
    using dt_sem_t = sem_t;
#endif

/**
 * Data structure to hold runtime statistics of a thread such as period and etc.
 */
typedef struct _threadTimeInfo
    {
        double targetPeriod_ms = 0;
        double period_ms = 0;
        double algo_ms = 0;
        double algoAvg_ms = 0;
        double algoMax_ms = 0;
        int overrun = 0;
} ThreadTimeInfo;

/**
 * Data structure to hold information of a thread created by dt::Thread.
 */
typedef struct _threadInfo
{
    const char *name = nullptr;
    void *(*procFunc)(void *arg) = nullptr;
    void *procFuncArg = nullptr;
    int cpuIdx = 0;
    int priority = 0;
    size_t stackSz = 0;
    dt_thread_t id = 0;
    int listIdx = 0;
} ThreadInfo;

/**
 * Information of semaphore created.
 */
typedef struct _semInfo
{
    const char *name = nullptr;
    dt_sem_t sem;
    int listIdx = 0;
} SemInfo;

/**
 * Information of mutex created.
 */
typedef struct _mtxInfo
{
    dt_mutex_t mutex;
    int listIdx = 0;
} MtxInfo;

//* Public(Exported) Variables -----------------------------------------------*/
//* Public(Exported) Functions -----------------------------------------------*/
/**
 * Count CPU installed.
 * @return It returns number of CPU installed.
 */
int GetCpuCount();

/**
 * Create a RT thread.
 * @param[in, out] thread Thread attributes to create.
 * @return It returns 0 if successful. Otherwise it returns non-zero error code.
 */
int CreateRtThread(ThreadInfo &thread);

/**
 * Create a non-RT thread.
 * @param[in, out] thread Thread attributes to create.
 * @return It returns 0 if successful. Otherwise it returns non-zero error code.
 */
int CreateNonRtThread(ThreadInfo &thread);

/**
 * Joint a thread and remove it from thread list.
 * @param[in] thread Thread to delete.
 * @return It returns 0 if successful. Otherwise it returns non-zero error code.
 */
int DeleteThread(ThreadInfo &thread);

/**
 * Delete all threads registered and clear thread list.
 * @return It returns 0 if successful. Otherwise it returns non-zero error code.
 */
int DeleteAllThread();

/**
 * Create a semaphore.
 * @param[out] semInfo Data structure that holds information of semaphore created.
 * @param[in] initValue Initail value of semaphore to create.
 * @return It returns 0 if successful. Otherwise it returns non-zero error code.
 */
int CreateSemaphore(SemInfo &semInfo, unsigned int initValue = 0);

/**
 * Signal semaphore.
 * @param[in] semInfo Semaphore to signal.
 * @return It returns 0 if successful. Otherwise it returns non-zero error code.
 */
inline int PostSemaphore(SemInfo &semInfo)
{
#if defined(_WIN32) || defined(__CYGWIN__)
    return 0;
#elif defined(__APPLE__)
    return dispatch_semaphore_signal(semInfo.sem);
#else
    return sem_post(&semInfo.sem);
#endif
}

/**
 * Signal all semaphores.
 */
void PostAllSemaphore();

/**
 * Wait until it becomes greater than zero.
 * @param[in] semInfo Semaphore to wait.
 * @return It returns 0 if successful. Otherwise it returns non-zero error code.
 */
inline int WaitSemaphore(SemInfo &semInfo)
{
#if defined(_WIN32) || defined(__CYGWIN__)
    return 0;
#elif defined(__APPLE__)
    return dispatch_semaphore_wait(semInfo.sem, DISPATCH_TIME_FOREVER);
#else
    return sem_wait(&semInfo.sem);
#endif
}

/**
 * Delete a semaphore and remove it from semaphore list.
 * @param[in] semInfo Semaphore to delete.
 * @return It returns 0 if successful. Otherwise it returns non-zero error code.
 */
int DeleteSemaphore(SemInfo &semInfo);

/**
 * Delete all semaphores registered.
 * @return It returns 0 if successful. Otherwise it returns non-zero error code.
 */
int DeleteAllSemaphore();

/**
 * Create a mutex with given information.
 * @param[out] mtxInfo Data structure to hold information about a new mutex to be created.
 * @return It returns 0 if successful. Otherwise it returns non-zero error code.
 */
int CreateMutex(MtxInfo &mtxInfo);

/**
 * Lock the given mutex.
 * @param[in] mtxInfo Data structure containing information about mutex to lock.
 * @return It returns 0 if successful. Otherwise it returns non-zero error code.
 */
inline int MutexLock(MtxInfo &mtxInfo)
{
#if defined(_WIN32) || defined(__CYGWIN__)
    return 0;
#else
    return pthread_mutex_lock(&mtxInfo.mutex);
#endif
}

/**
 * Try to lock the given mutex.
 * @param[in] mtxInfo Data structure containing information about mutex to lock.
 * @return It returns 0 if successful. Otherwise it returns non-zero error code.
 */
inline int MutexTryLock(MtxInfo &mtxInfo)
{
#if defined(_WIN32) || defined(__CYGWIN__)
    return 0;
#else
    return pthread_mutex_trylock(&mtxInfo.mutex); // returns 0 if successful.
#endif
}

/**
 * Unlock the given mutex.
 * @param[in] mtxInfo Data structure containing information about mutex to unlock.
 * @return It returns 0 if successful. Otherwise it returns non-zero error code.
 */
inline int MutexUnlock(MtxInfo &mtxInfo)
{
#if defined(_WIN32) || defined(__CYGWIN__)
    return 0;
#else
    return pthread_mutex_unlock(&mtxInfo.mutex);
#endif
}

/**
 * Delete specified mutex. 
 * @param[in] mtxInfo Mutex to delete.
 * @return It returns 0 if successful. Otherwise it returns non-zero error code.
 */
int DeleteMutex(MtxInfo &mtxInfo);

/**
 * Delete all mutex created.
 * @return It returns 0 if successful. Otherwise it returns non-zero error code.
 */
int DeleteAllMutex();

/**
 * Sleep calling thread.
 * @param[in] milliseconds Sleep duration in milliseconds.
 */
inline void SleepForMillis(unsigned int milliseconds)
{
#if defined(_WIN32) || defined(__CYGWIN__)
    ::Sleep(milliseconds);
#else
    usleep(milliseconds * 1E3);
#endif
}

} // namespace Thread
} // namespace dt

#endif // __DT_THREAD_THREADIMP_H__
