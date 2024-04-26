#include "dtCore/src/dtUtils/dtTimeCheck.h"

#if defined(_WIN32)
dtTimeCheck::dtTimeCheck()
{
    bStart = false;
    // Gets the frequency of the performance counter.
    // The frequency of the performance counter is fixed at system boot
    QueryPerformanceFrequency(&freq);
}

dtTimeCheck::~dtTimeCheck()
{
}

/*! Start Time Check */
void dtTimeCheck::Start(void)
{
    // Gets start point value of the performance counter
    QueryPerformanceCounter(&startTime);
    bStart = true;
}

/*! Stop Time Check */
int dtTimeCheck::Stop()
{
    // Gets end point value of the performance counter
    QueryPerformanceCounter(&endTime);

    if (!bStart) return -1;

    if (freq.QuadPart != 0)
    {
        elapsedTime_msec = (endTime.QuadPart - startTime.QuadPart) * 1E3 / static_cast<double>(freq.QuadPart);
        elapsedTime_usec = (endTime.QuadPart - startTime.QuadPart) * 1E6 / static_cast<double>(freq.QuadPart);
    }

    bStart = false;
    return 0;
}

void dtTimeCheck::Reset(void)
{
    bStart = false;
}

double dtTimeCheck::GetElapsedTime_sec(void)
{
    return elapsedTime_msec / 1E3;
}

double dtTimeCheck::GetElapsedTime_msec(void)
{
    return elapsedTime_msec;
}

double dtTimeCheck::GetElapsedTime_usec()
{
    return elapsedTime_usec;
}

#elif defined(__linux__)
dtTimeCheck::dtTimeCheck()
{
    bStart = false;
}

dtTimeCheck::~dtTimeCheck()
{
}

/*! Start Time Check */
void dtTimeCheck::Start(void)
{
    clock_gettime(CLOCK_MONOTONIC, &startTime);
    bStart = true;
}

/*! Stop Time Check */
int dtTimeCheck::Stop()
{
    clock_gettime(CLOCK_MONOTONIC, &endTime);

    if (!bStart) return -1;

    elapsedTime_msec = (endTime.tv_sec - startTime.tv_sec) * 1E3 + (endTime.tv_nsec - startTime.tv_nsec) / 1E6;
    elapsedTime_usec = (endTime.tv_sec - startTime.tv_sec) * 1E6 + (endTime.tv_nsec - startTime.tv_nsec) / 1E3;
    elapsedTime_nsec = (endTime.tv_sec - startTime.tv_sec) * 1E9 + (endTime.tv_nsec - startTime.tv_nsec);
    bStart = false;
    return 0;
}

void dtTimeCheck::Reset(void)
{
    bStart = false;
}

double dtTimeCheck::GetElapsedTime_sec(void)
{
    return elapsedTime_msec / 1E3;
}

double dtTimeCheck::GetElapsedTime_msec(void)
{
    return elapsedTime_msec;
}

double dtTimeCheck::GetElapsedTime_usec()
{
    return elapsedTime_usec;
}

long dtTimeCheck::GetElapsedTime_nsec()
{
    return elapsedTime_nsec;
}

#elif defined(ARDUINO)

dtTimeCheck::dtTimeCheck()
{
    bStart = false;
}
dtTimeCheck::~dtTimeCheck() {}

/*! Start Time Check */
void dtTimeCheck::Start(void)
{
    startTime = micros();
    bStart = true;
}

/*! Stop Time Check */
int dtTimeCheck::Stop()
{
    endTime = micros();

    if (!bStart)
    {
        elapsedTime_usec = -1;
        bStart = false;
        return -1;
    }

    elapsedTime_usec = (double)(endTime - startTime);

    bStart = false;
    return 0;
}

void dtTimeCheck::Reset(void)
{
    bStart = false;
}

double dtTimeCheck::GetElapsedTime_sec(void)
{
    return (elapsedTime_usec) / 1E6;
}

double dtTimeCheck::GetElapsedTime_msec(void)
{
    return (elapsedTime_usec) / 1E3;
}

double dtTimeCheck::GetElapsedTime_usec()
{
    return elapsedTime_usec;
}

#endif
