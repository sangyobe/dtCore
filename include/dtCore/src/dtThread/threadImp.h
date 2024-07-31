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

//* C/C++ System Headers -----------------------------------------------------*/
extern "C"
{
#include <unistd.h>
#if defined(_WIN32) || defined(__CYGWIN__)
#include <windows.h>
#include <pthread.h>
#elif defined(__APPLE__)
#include <sys/types.h>
#include <sys/sysctl.h>
#include <mach/thread_act.h>
#include <dispatch/dispatch.h> // semaphore
#include <pthread.h>
#else
#include <pthread.h>
#include <semaphore.h>
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
#if defined(__APPLE__)
    using dt_thread_t = pthread_t; // std::thread;
    using dt_mutex_t = pthread_mutex_t;
    using dt_sem_t = dispatch_semaphore_t;
#else
    using dt_thread_t = pthread_t;
    using dt_mutex_t = pthread_mutex_t;
    using dt_sem_t = sem_t;
#endif

typedef struct _threadTimeInfo
{
    double targetPeriod_ms = 0;
    double period_ms = 0;
    double algo_ms = 0;
    double algoAvg_ms = 0;
    double algoMax_ms = 0;
    int overrun = 0;
} ThreadTimeInfo;

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

typedef struct _semInfo
{
    const char *name = nullptr;
    dt_sem_t sem;
    int listIdx = 0;
} SemInfo;

typedef struct _mtxInfo
{
    dt_mutex_t mutex;
    int listIdx = 0;
} MtxInfo;

//* Public(Exported) Variables -----------------------------------------------*/
//* Public(Exported) Functions -----------------------------------------------*/
int GetCpuCount();

int CreateRtThread(ThreadInfo &thread);
int CreateNonRtThread(ThreadInfo &thread);
int DeleteThread(ThreadInfo &thread);
int DeleteAllThread();

int CreateSemaphore(SemInfo &semInfo, unsigned int initValue = 0);
inline int PostSemaphore(SemInfo &semInfo);
void PostAllSemaphore();
inline int WaitSemaphore(SemInfo &semInfo);
int DeleteSemaphore(SemInfo &semInfo);
int DeleteAllSemaphore();

int CreateMutex(MtxInfo &mtxInfo);

/**
 * Lock the given mutex.
 * @param mtxInfo Data structure containing information about mutex to lock.
 * @return It returns 0 if successful. Otherwise it returns the non-zero error code.
 */
inline int MutexLock(MtxInfo &mtxInfo);

/**
 * Try to lock the given mutex.
 * @param mtxInfo Data structure containing information about mutex to lock.
 * @return It returns 0 if a lock on the mutex object that is referenced by the mtxInfo.mutex parameter is acquired. Otherwise it returns the non-zero error code.
 */
inline int MutexTryLock(MtxInfo &mtxInfo);

/**
 * Unlock the given mutex.
 * @param mtxInfo Data structure containing information about mutex to unlock.
 * @return It returns 0 if successful. Otherwise it returns the non-zero error code.
 */
inline int MutexUnlock(MtxInfo &mtxInfo);

int DeleteMutex(MtxInfo &mtxInfo);
int DeleteAllMutex();

inline void SleepForMillis(unsigned int milliseconds)
{
#if defined(_WIN32)
    ::Sleep(milliseconds);
#else
    usleep(milliseconds * 1E3);
#endif
}

} // namespace Thread
} // namespace dt

#endif // __DT_THREAD_THREADIMP_H__
