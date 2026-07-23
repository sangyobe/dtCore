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
#include <dtCore/dtLog>

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
        LOG_CONT(info).printf("error while get core count %d\n", ret);
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
    LOG_CONT(info).printf("binding to core %d\n", core);
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
    LOG_CONT(info).printf("  Stack size: %zd\n", memSize);

    if (pthread_attr_getguardsize(attr, &memSize)) goto error;
    LOG_CONT(info).printf("  Guard size: %zd\n", memSize);

    if (pthread_attr_getschedpolicy(attr, &rtnParam)) goto error;
    if (rtnParam == SCHED_FIFO)
        LOG_CONT(info).printf("  Scheduling policy: SCHED_FIFO\n");
    else if (rtnParam == SCHED_RR)
        LOG_CONT(info).printf("  Scheduling policy: SCHED_RR\n");
    else if (rtnParam == SCHED_OTHER)
        LOG_CONT(info).printf("  Scheduling policy: SCHED_OTHER\n");
    else
        LOG_CONT(info).printf("  Scheduling policy: [unknown]\n");

    if (pthread_attr_getschedparam(attr, &schedparam)) goto error;
    LOG_CONT(info).printf("  Scheduling priority: %d\n", schedparam.sched_priority);

    if (pthread_attr_getdetachstate(attr, &rtnParam)) goto error;
    if (rtnParam == PTHREAD_CREATE_DETACHED)
        LOG_CONT(info).printf("  Detach state: DETACHED\n");
    else if (rtnParam == PTHREAD_CREATE_JOINABLE)
        LOG_CONT(info).printf("  Detach state: JOINABLE\n");
    else
        LOG_CONT(info).printf("  Detach state: [unknown]\n");

    if (pthread_attr_getinheritsched(attr, &rtnParam)) goto error;
    if (rtnParam == PTHREAD_INHERIT_SCHED)
        LOG_CONT(info).printf("  Inherit scheduler: INHERIT\n");
    else if (rtnParam == PTHREAD_EXPLICIT_SCHED)
        LOG_CONT(info).printf("  Inherit scheduler: EXPLICIT\n");
    else
        LOG_CONT(info).printf("  Inherit scheduler: [unknown]\n");

    return 0;

error:
    LOG(err).printf("!Error! PrintThreadAttr() : %s(%d)\n", strerror(errno), errno);
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
            LOG_CONT(info).printf("Max CPU Core Count: %d\n", maxCpuCnt);
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

int CreateThread(ThreadInfo &thread, bool realtime, bool addList)
{
    cpu_set_t cpuset;
    pthread_attr_t taskAttr;
    struct sched_param taskParam = {.sched_priority = thread.priority};
    LOG(info).printf("========= %s =========", realtime ? "CreateRtThread()": "CreateNonRtThread()");
    if (maxCpuCnt == 0) GetCpuCount();
    LOG_CONT(info).printf("Thread Name: %s\n", thread.name);

    /* Step 1. Check CPU assign */
    if (thread.cpuIdx < 0 || thread.cpuIdx >= maxCpuCnt)
    {
        LOG_CONT(info).printf("CPU Index: %d ... user error: Check the CPU Index\n", thread.cpuIdx);
        thread.cpuIdx = 0;
        LOG_CONT(info).printf("CPU Index: default(%d) ... ok\n", thread.cpuIdx);
    }
    else
        LOG_CONT(info).printf("CPU Index: %d ... ok\n", thread.cpuIdx);
    CPU_ZERO(&cpuset);               // removes all CPUs from cpuset
    CPU_SET(thread.cpuIdx, &cpuset); // add CPU idx to the cpuset

    /* Step 2. Thread attribute setting */
    LOG_CONT(info).printf("Set pthread attribute ... ");
    if (pthread_attr_init(&taskAttr)) goto error;
    if (pthread_attr_setinheritsched(&taskAttr, PTHREAD_EXPLICIT_SCHED)) goto error;
    if (realtime)
    {
        if (pthread_attr_setschedpolicy(&taskAttr, SCHED_FIFO)) goto error;
        if (pthread_attr_setschedparam(&taskAttr, &taskParam)) goto error;
    }
    else
    {
        if (pthread_attr_setschedpolicy(&taskAttr, SCHED_OTHER)) goto error;
    }
    if (pthread_attr_setaffinity_np(&taskAttr, sizeof(cpuset), &cpuset)) goto error;
    if (pthread_attr_setdetachstate(&taskAttr, PTHREAD_CREATE_JOINABLE)) goto error;
    
    if (thread.stackSz > 0)
    {
        if (pthread_attr_setstacksize(&taskAttr, thread.stackSz)) goto error;
    }
    LOG_CONT(info).printf("ok\n");

    /* Step 3. Thread Create */
    LOG_CONT(info).printf("Create %s Thread ... ", realtime ? "RT" : "Non-RT");
    if (pthread_create(&thread.id, &taskAttr, thread.procFunc, thread.procFuncArg)) goto error;
#if defined(__APPLE__)
    if (pthread_setaffinity_np(thread.id, CPU_SETSIZE, &cpuset)) goto error;
#endif
    pthread_setname_np(thread.id, thread.name);
    LOG_CONT(info).printf("ok\n");

    /* Step 4. Check and Destroy the Attribute */
    PrintThreadAttr(&taskAttr);
    if (pthread_attr_destroy(&taskAttr)) goto error_no_destroy;
    LOG_CONT(info).printf("Complete\n");
    
    if (addList)
    {
        threadList[threadNum] = &thread.id;
        thread.listIdx = threadNum;
        threadNum++;
    }
    else
    {
        thread.listIdx = (-1);
    }
    LOG_CONT(info).printf("------------------------------------");

    return 0;

error:
    pthread_attr_destroy(&taskAttr);
error_no_destroy:
    LOG_CONT(err).printf("!Error! %s : %s(%d)", realtime ? "CreateRtThread()": "CreateNonRtThread()", strerror(errno), errno);
    LOG_CONT(info).printf("------------------------------------\n");
    return (-1);
}

int CreateRtThread(ThreadInfo &thread)
{
    return (CreateThread(thread, true, true));
}

int CreateNonRtThread(ThreadInfo &thread)
{
    return (CreateThread(thread, false, true));
}

int DeleteThread(ThreadInfo &thread)
{
    LOG(info).printf("Delete Thread ");
    LOG_CONT(info).printf("  Delete %s ... ", thread.name);

    if (pthread_join(thread.id, NULL)) goto error;
    LOG_CONT(info).printf("  ok\n");
    LOG_CONT(info).printf("  Complete\n");

    if (thread.listIdx >= 0)
    {
        threadList[thread.listIdx] = nullptr;
    }

    return 0;

error:
    LOG(err).printf("!Error! DeleteThread() : %s(%d)\n", strerror(errno), errno);
    return -1;
}

int DeleteAllThread()
{
    int num = 0;

    LOG(info).printf("Delete All Thread ");
    for (int idx = threadNum - 1; idx >= 0; idx--)
    {
        if (threadList[idx] == nullptr) continue;
        if (pthread_join(*threadList[idx], NULL)) goto error;
        num++;
    }
    LOG_CONT(info).printf("  Delete %d thread ... ok\n", num);
    LOG_CONT(info).printf("Complete\n");

    return 0;

error:
    LOG(err).printf("!Error! DeleteAllThread() : %s(%d)\n", strerror(errno), errno);
    return -1;
}


int CreateSemaphore(SemInfo &semInfo, unsigned int initValue)
{
    LOG(info).printf("Create Semaphore ");
    LOG_CONT(info).printf("  Initialize Semaphore ... ");

#if defined(__APPLE__)
    semInfo.sem = dispatch_semaphore_create(initValue);
#else
    if (sem_init(&semInfo.sem, 0, initValue)) goto error; // 0 means semaphore may only be used by threads in the same process
#endif
    LOG_CONT(info).printf("ok\n");
    LOG_CONT(info).printf("Complete\n");
    semList[semNum] = &semInfo.sem;
    semInfo.listIdx = semNum;
    semNum++;

    return 0;

error:
    LOG(err).printf("!Error! CreateSemaphore() : %s(%d)\n", strerror(errno), errno);
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
    LOG(info).printf("Delete Semaphore ");
    LOG_CONT(info).printf("  Destroy Semaphore ... ");
#if defined(__APPLE__)
    dispatch_release(semInfo.sem);
#else
    if (sem_destroy(&semInfo.sem)) goto error;
#endif
    LOG_CONT(info).printf("ok\n");
    LOG_CONT(info).printf("Complete\n");
    semList[semInfo.listIdx] = nullptr;
    // LOG_CONT(info).printfEndLine();

    return 0;

error:
    LOG(err).printf("!Error! DeleteSemaphore() : %s(%d)\n", strerror(errno), errno);
    return -1;
}

int DeleteAllSemaphore()
{
    int num = 0;

    LOG(info).printf("Delete All Semaphore ");
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
    LOG_CONT(info).printf("  Delete %d semaphore ... ok\n", num);
    LOG_CONT(info).printf("Complete\n");

    return 0;

error:
    LOG(err).printf("!Error! DeleteAllSemaphore() : %s(%d)\n", strerror(errno), errno);
    return -1;
}

int CreateMutex(MtxInfo &mtxInfo)
{
    LOG(info).printf("Create Mutex ");
    LOG_CONT(info).printf("  Initialize Mutex ... ");
    if (pthread_mutex_init(&mtxInfo.mutex, NULL)) goto error;
    LOG_CONT(info).printf("ok\n");
    LOG_CONT(info).printf("Complete\n");
    mtxList[mtxNum] = &mtxInfo.mutex;
    mtxInfo.listIdx = mtxNum;
    mtxNum++;

    return 0;

error:
    LOG(err).printf("!Error! CreateMutex() : %s(%d)\n", strerror(errno), errno);
    return -1;
}

int DeleteMutex(MtxInfo &mtxInfo)
{
    LOG(info).printf("Delete Mutex ");
    LOG_CONT(info).printf("  Destroy Mutex ... ");
    if (pthread_mutex_destroy(&mtxInfo.mutex)) goto error;
    LOG_CONT(info).printf("ok\n");
    LOG_CONT(info).printf("Complete\n");
    mtxList[mtxInfo.listIdx] = nullptr;

    return 0;

error:
    LOG(err).printf("!Error! DeleteMutex() : %s(%d)\n", strerror(errno), errno);
    return -1;
}

int DeleteAllMutex()
{
    int num = 0;

    LOG(info).printf("Delete All Mutex ");
    for (int idx = mtxNum - 1; idx >= 0; idx--)
    {
        if (mtxList[idx] == nullptr) continue;
        if (pthread_mutex_destroy(mtxList[idx])) goto error;
        num++;
    }
    LOG_CONT(info).printf("  Delete %d mutex ... ok\n", num);
    LOG_CONT(info).printf("Complete\n");

    return 0;

error:
    LOG(err).printf("!Error! DeleteAllMutex() : %s(%d)\n", strerror(errno), errno);
    return -1;
}
#endif

} // namespace Thread
} // namespace dt
