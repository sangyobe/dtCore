//* Related Headers ----------------------------------------------------------*/
#include "dtCore/src/dtThread/threadImp.h"

//* C/C++ System Headers -----------------------------------------------------*/
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#if defined(_WIN32) || defined(__CYGWIN__)
#else
#include <unistd.h>
#endif

//* Other Lib Headers --------------------------------------------------------*/
//* Project Headers ----------------------------------------------------------*/
#include "dtCore/src/dtUtils/dtTerminal.h"

//* System-Specific Headers --------------------------------------------------*/

namespace dt
{
namespace Thread
{
//* Public(Exported) Variables -----------------------------------------------*/
//* Private Macro ------------------------------------------------------------*/
//* Private Types ------------------------------------------------------------*/
#if defined(__APPLE__)
#include <TargetConditionals.h>

#define SYSCTL_CORE_COUNT   "machdep.cpu.core_count"

typedef struct cpu_set {
    uint32_t    count;
} cpu_set_t;

static const size_t
CPU_SETSIZE = sizeof(cpu_set_t);

static inline void
CPU_ZERO(cpu_set_t *cs) { cs->count = 0; }

static inline void
CPU_SET(int num, cpu_set_t *cs) { cs->count |= (1 << num); }

static inline int
CPU_ISSET(int num, cpu_set_t *cs) { return (cs->count & (1 << num)); }

static inline cpu_set_t *
CPU_ALLOC(int count) { return (cpu_set_t *)malloc(sizeof(cpu_set_t)); }

static inline size_t
CPU_ALLOC_SIZE(int count) { return sizeof(cpu_set_t); }

static inline void
CPU_FREE(cpu_set_t *cpuset) { if (cpuset) free(cpuset); }

int sched_getaffinity(pid_t pid, size_t cpusetsize, cpu_set_t *mask)
{
    int32_t core_count = 0;
    size_t  len = sizeof(core_count);
    int ret = sysctlbyname(SYSCTL_CORE_COUNT, &core_count, &len, 0, 0);
    if (ret) {
        dtTerm::Printf("error while get core count %d\n", ret);
        return -1;
    }
    mask->count = 0;
    for (int i = 0; i < core_count; i++) {
        mask->count |= (1 << i);
    }
    return 0;
}

int pthread_setaffinity_np(pthread_t thread, size_t cpusetsize, cpu_set_t *mask)
{
    thread_port_t mach_thread;
    int core = 0;

    for (core = 0; core < 8 * cpusetsize; core++) {
        if (CPU_ISSET(core, mask)) break;
    }
    dtTerm::Printf("binding to core %d\n", core);
    thread_affinity_policy_data_t policy = { core };
    mach_thread = pthread_mach_thread_np(thread);
    thread_policy_set(mach_thread, THREAD_AFFINITY_POLICY, (thread_policy_t)&policy, 1);
    return 0;
}

int pthread_attr_setaffinity_np(pthread_attr_t *attr, size_t cpusetsize, const cpu_set_t *mask)
{
    return 0;
}
#endif

//* Private Variables --------------------------------------------------------*/
static int threadNum = 0; // thread list number
static int semNum = 0;
static int mtxNum = 0;
static dt_thread_t *threadList[32];
static dt_sem_t *semList[64];
static dt_mutex_t *mtxList[64];
static int maxCpuCnt = 0;
//* Private Functions --------------------------------------------------------*/
#if defined(_WIN32) || defined(__CYGWIN__)
#else
int PrintThreadAttr(const pthread_attr_t *attr);
#endif

//* Private Functions Definition ---------------------------------------------*/
#if defined(_WIN32) || defined(__CYGWIN__)
#else
int PrintThreadAttr(const pthread_attr_t *attr)
{
    size_t memSize;
    struct sched_param schedparam;
    int rtnParam;

    if (pthread_attr_getstacksize(attr, &memSize)) goto error;
    dtTerm::Printf("  Stack size: %zd\n", memSize);

    if (pthread_attr_getguardsize(attr, &memSize)) goto error;
    dtTerm::Printf("  Guard size: %zd\n", memSize);

    if (pthread_attr_getschedpolicy(attr, &rtnParam)) goto error;
    if (rtnParam == SCHED_FIFO)
        dtTerm::Print("  Scheduling policy: SCHED_FIFO\n");
    else if (rtnParam == SCHED_RR)
        dtTerm::Print("  Scheduling policy: SCHED_RR\n");
    else if (rtnParam == SCHED_OTHER)
        dtTerm::Print("  Scheduling policy: SCHED_OTHER\n");
    else
        dtTerm::Print("  Scheduling policy: [unknown]\n");

    if (pthread_attr_getschedparam(attr, &schedparam)) goto error;
    dtTerm::Printf("  Scheduling priority: %d\n", schedparam.sched_priority);

    if (pthread_attr_getdetachstate(attr, &rtnParam)) goto error;
    if (rtnParam == PTHREAD_CREATE_DETACHED)
        dtTerm::Print("  Detach state: DETACHED\n");
    else if (rtnParam == PTHREAD_CREATE_JOINABLE)
        dtTerm::Print("  Detach state: JOINABLE\n");
    else
        dtTerm::Print("[unknown]\n");

    if (pthread_attr_getinheritsched(attr, &rtnParam)) goto error;
    if (rtnParam == PTHREAD_INHERIT_SCHED)
        dtTerm::Print("  Inherit scheduler: INHERIT\n");
    else if (rtnParam == PTHREAD_EXPLICIT_SCHED)
        dtTerm::Print("  Inherit scheduler: EXPLICIT\n");
    else
        dtTerm::Print("  Inherit scheduler: [unknown]\n");

    return 0;

error:
    fprintf(stderr, "!Error! PrintThreadAttr() : %s(%d)\n", strerror(errno), errno);
    dtTerm::PrintEndLine();
    return -1;
}
#endif

//* Public(Exported) Functions Definition ------------------------------------*/
#if defined(_WIN32) || defined(__CYGWIN__)
#else
int GetCpuCount(void)
{
    // start by assuming a maximum of 128 hardware threads and keep growing until
    // the cpu_set_t is big enough to hold the mask for the entire machine
    for (int max_count = 128; true; max_count *= 2)
    {
        cpu_set_t *cpusetp = CPU_ALLOC(max_count);
        size_t setsize = CPU_ALLOC_SIZE(max_count);

        if (!sched_getaffinity(getpid(), setsize, cpusetp))
        {
            // success
            // maxCpuCnt = CPU_COUNT_S(setsize, cpusetp);
            maxCpuCnt = sysconf(_SC_NPROCESSORS_ONLN);
            dtTerm::Printf("Max CPU Core Count: %d\n", maxCpuCnt);
            CPU_FREE(cpusetp);
            break;
        }
        else if (errno != EINVAL)
        {
            // failure other than max_count being too small, just return 1
            maxCpuCnt = 1;
            CPU_FREE(cpusetp);
            break;
        }
    }

    return maxCpuCnt;
}

int CreateRtThread(ThreadInfo &thread)
{
    cpu_set_t cpuset;
    pthread_attr_t taskAttr;
    struct sched_param taskParam = {.sched_priority = thread.priority};
    if (maxCpuCnt == 0) GetCpuCount();

    dtTerm::PrintTitle(" Create Thread ");
    dtTerm::Printf("Thread Name: %s\n", thread.name);

    /* Step 1. Check CPU assign */
    if (thread.cpuIdx < 0 || thread.cpuIdx >= maxCpuCnt)
    {
        dtTerm::Printf("CPU Index: %d ... user error: Check the CPU Index\n", thread.cpuIdx);
        thread.cpuIdx = 0;
        dtTerm::Printf("CPU Index: default(%d) ... ok\n", thread.cpuIdx);
    }
    else
        dtTerm::Printf("CPU Index: %d ... ok\n", thread.cpuIdx);
    CPU_ZERO(&cpuset);               // removes all CPUs from cpuset
    CPU_SET(thread.cpuIdx, &cpuset); // add CPU idx to the cpuset

    /* Step 2. Thread attribute setting */
    dtTerm::Print("Set pthread attribute ... ");
    if (pthread_attr_init(&taskAttr)) goto error;
    if (pthread_attr_setinheritsched(&taskAttr, PTHREAD_EXPLICIT_SCHED)) goto error;
    if (pthread_attr_setschedpolicy(&taskAttr, SCHED_FIFO)) goto error;
    if (pthread_attr_setaffinity_np(&taskAttr, CPU_SETSIZE, &cpuset)) goto error;
    if (pthread_attr_setdetachstate(&taskAttr, PTHREAD_CREATE_JOINABLE)) goto error;
    if (pthread_attr_setschedparam(&taskAttr, &taskParam)) goto error;
    if (thread.stackSz > 0)
    {
        if (pthread_attr_setstacksize(&taskAttr, thread.stackSz)) goto error;
    }
    dtTerm::Print("ok\n");

    /* Step 3. Thread Create */
    dtTerm::Print("Create RT Thread ... ");
    if (pthread_create(&thread.id, &taskAttr, thread.procFunc, thread.procFuncArg)) goto error;
#if defined(__APPLE__)
    if (pthread_setaffinity_np(thread.id, CPU_SETSIZE, &cpuset)) goto error;
#endif
    dtTerm::Print("ok\n");

    /* Step 4. Check and Destroy the Attribute */
    PrintThreadAttr(&taskAttr);
    if (pthread_attr_destroy(&taskAttr)) goto error;
    dtTerm::Print("Complete\n");
    threadList[threadNum] = &thread.id;
    thread.listIdx = threadNum;
    threadNum++;
    dtTerm::PrintEndLine();

    return 0;

error:
    fprintf(stderr, "!Error! CreateRtThread() : %s(%d)\n", strerror(errno), errno);
    dtTerm::PrintEndLine();
    return -1;
}

int CreateNonRtThread(ThreadInfo &thread)
{
    cpu_set_t cpuset;
    pthread_attr_t taskAttr;
    if (maxCpuCnt == 0) GetCpuCount();

    dtTerm::PrintTitle(" Create Thread ");
    dtTerm::Printf("Thread Name: %s\n", thread.name);

    /* Step 1. Check CPU assign */
    if (thread.cpuIdx < 0 || thread.cpuIdx >= maxCpuCnt)
    {
        dtTerm::Printf("CPU Index: %d ... user error: Check the CPU Index\n", thread.cpuIdx);
        thread.cpuIdx = 0;
        dtTerm::Printf("CPU Index: default(%d) ... ok\n", thread.cpuIdx);
    }
    else
        dtTerm::Printf("CPU Index: %d ... ok\n", thread.cpuIdx);
    CPU_ZERO(&cpuset);               // removes all CPUs from cpuset
    CPU_SET(thread.cpuIdx, &cpuset); // add CPU idx to the cpuset

    /* Step 2. Thread attribute setting */
    dtTerm::Printf("Set pthread attribute ... ");
    if (pthread_attr_init(&taskAttr)) goto error;
    if (pthread_attr_setinheritsched(&taskAttr, PTHREAD_EXPLICIT_SCHED)) goto error;
    if (pthread_attr_setschedpolicy(&taskAttr, SCHED_OTHER)) goto error;
    if (pthread_attr_setaffinity_np(&taskAttr, sizeof(cpuset), &cpuset)) goto error;
    if (pthread_attr_setdetachstate(&taskAttr, PTHREAD_CREATE_JOINABLE)) goto error;

    if (thread.stackSz > 0)
    {
        if (pthread_attr_setstacksize(&taskAttr, thread.stackSz)) goto error;
    }

    dtTerm::Print("ok\n");

    /* Step 3. Ctrate Thread */
    dtTerm::Print("Create Non-RT Thread ... ");
    if (pthread_create(&thread.id, &taskAttr, thread.procFunc, thread.procFuncArg)) goto error;
#if defined(__APPLE__)
    if (pthread_setaffinity_np(thread.id, CPU_SETSIZE, &cpuset)) goto error;
#endif
    dtTerm::Print("ok\n");

    /* Step 4. Check and Destroy the Attribute */
    PrintThreadAttr(&taskAttr);
    if (pthread_attr_destroy(&taskAttr)) goto error;

    dtTerm::Print("Complete\n");
    threadList[threadNum] = &thread.id;
    thread.listIdx = threadNum;
    threadNum++;
    dtTerm::PrintEndLine();

    return 0;

error:
    fprintf(stderr, "!Error! CreateNonRtThread() : %s(%d)\n", strerror(errno), errno);
    dtTerm::PrintEndLine();
    return -1;
}

int DeleteThread(ThreadInfo &thread)
{
    dtTerm::PrintTitle(" Delete Thread ");
    dtTerm::Printf("Delete %s ... ", thread.name);

    if (pthread_join(thread.id, NULL)) goto error;
    dtTerm::Print("ok\n");
    dtTerm::Print("Complete\n");

    threadList[thread.listIdx] = nullptr;
    dtTerm::PrintEndLine();

    return 0;

error:
    fprintf(stderr, "!Error! DeleteThread() : %s(%d)\n", strerror(errno), errno);
    dtTerm::PrintEndLine();
    return -1;
}

int DeleteAllThread()
{
    int num = 0;

    for (int idx = threadNum - 1; idx >= 0; idx--)
    {
        if (threadList[idx] == nullptr) continue;
        if (pthread_join(*threadList[idx], NULL)) goto error;
        num++;
    }
    dtTerm::Printf(43, 1, "--------------------------------");
    dtTerm::PrintTitle(" Delete All Thread ");
    dtTerm::Printf("Delete %d thread ... ok\n", num);
    dtTerm::Print("Complete\n");
    dtTerm::PrintEndLine();

    return 0;

error:
    dtTerm::PrintTitle(" Delete All Thread ");
    fprintf(stderr, "!Error! DeleteAllThread() : %s(%d)\n", strerror(errno), errno);
    dtTerm::PrintEndLine();
    return -1;
}


int CreateSemaphore(SemInfo &semInfo, unsigned int initValue)
{
    dtTerm::PrintTitle(" Ctreate Semaphore ");
    dtTerm::Print("Initialize Semaphore ... ");

#if defined(__APPLE__)
    semInfo.sem = dispatch_semaphore_create(initValue);
#else
    if (sem_init(&semInfo.sem, 0, initValue)) goto error; // 0 means semaphore may only be used by threads in the same process
#endif
    dtTerm::Print("ok\n");
    dtTerm::Print("Complete\n");
    semList[semNum] = &semInfo.sem;
    semInfo.listIdx = semNum;
    semNum++;
    dtTerm::PrintEndLine();

    return 0;

error:
    fprintf(stderr, "!Error! CreateSemaphore() : %s(%d)\n", strerror(errno), errno);
    dtTerm::PrintEndLine();
    return -1;
}

void PostAllSemaphore()
{
    for (int idx = semNum - 1; idx >= 0; idx--)
    {
        if (semList[idx] == nullptr) continue;
#if defined(__APPLE__)
        dispatch_semaphore_signal(*semList[idx]);
#else
        sem_post(semList[idx]);
#endif
    }
}

int DeleteSemaphore(SemInfo &semInfo)
{
    dtTerm::PrintTitle(" Delete Semaphore ");
    dtTerm::Print("Destroy Semaphore ... ");
#if defined(__APPLE__)
    dispatch_release(semInfo.sem);
#else
    if (sem_destroy(&semInfo.sem)) goto error;
#endif
    dtTerm::Print("ok\n");
    dtTerm::Print("Complete\n");
    semList[semInfo.listIdx] = nullptr;
    dtTerm::PrintEndLine();

    return 0;

error:
    fprintf(stderr, "!Error! DeleteSemaphore() : %s(%d)\n", strerror(errno), errno);
    dtTerm::PrintEndLine();
    return -1;
}

int DeleteAllSemaphore()
{
    int num = 0;

    dtTerm::PrintTitle(" Delete All Semaphore ");
    for (int idx = semNum - 1; idx >= 0; idx--)
    {
        if (semList[idx] == nullptr) continue;
#if defined(__APPLE__)
        dispatch_release(*semList[idx]);
#else
        if (sem_destroy(semList[idx])) goto error;
#endif
        num++;
    }
    dtTerm::Printf("Delete %d semaphore ... ok\n", num);
    dtTerm::Print("Complete\n");
    dtTerm::PrintEndLine();

    return 0;

error:
    fprintf(stderr, "!Error! DeleteAllSemaphore() : %s(%d)\n", strerror(errno), errno);
    dtTerm::PrintEndLine();
    return -1;
}

int CreateMutex(MtxInfo &mtxInfo)
{
    dtTerm::PrintTitle(" Ctreate Mutex ");
    dtTerm::Print("Initialize Mutex ... ");
    if (pthread_mutex_init(&mtxInfo.mutex, NULL)) goto error;
    dtTerm::Print("ok\n");
    dtTerm::Print("Complete\n");
    mtxList[mtxNum] = &mtxInfo.mutex;
    mtxInfo.listIdx = mtxNum;
    mtxNum++;
    dtTerm::PrintEndLine();

    return 0;

error:
    fprintf(stderr, "!Error! CreateMutex() : %s(%d)\n", strerror(errno), errno);
    dtTerm::PrintEndLine();
    return -1;
}

int DeleteMutex(MtxInfo &mtxInfo)
{
    dtTerm::PrintTitle(" Delete Mutex ");
    dtTerm::Print("Destroy Mutex ... ");
    if (pthread_mutex_destroy(&mtxInfo.mutex)) goto error;
    dtTerm::Print("ok\n");
    dtTerm::Print("Complete\n");
    mtxList[mtxInfo.listIdx] = nullptr;
    dtTerm::PrintEndLine();

    return 0;

error:
    fprintf(stderr, "!Error! DeleteMutex() : %s(%d)\n", strerror(errno), errno);
    dtTerm::PrintEndLine();
    return -1;
}

int DeleteAllMutex()
{
    int num = 0;

    dtTerm::PrintTitle(" Delete All Mutex ");
    for (int idx = mtxNum - 1; idx >= 0; idx--)
    {
        if (mtxList[idx] == nullptr) continue;
        if (pthread_mutex_destroy(mtxList[idx])) goto error;
        num++;
    }
    dtTerm::Printf("Delete %d mutex ... ok\n", num);
    dtTerm::Print("Complete\n");
    dtTerm::PrintEndLine();

    return 0;

error:
    fprintf(stderr, "!Error! DeleteAllMutex() : %s(%d)\n", strerror(errno), errno);
    dtTerm::PrintEndLine();
    return -1;
}
#endif

} // namespace Thread
} // namespace dt