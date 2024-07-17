/*!
\file       dtTerm.cpp
\brief      Configure Terminal(Console) Attribute
\author     Dong-hyun Lee, phenom8305@gamil.com
\date       2021. 02. 05
\version    1.0.0
*/

#ifdef _WIN32
#define _CRT_SECURE_NO_WARNINGS 1
#include <conio.h>
#include <windows.h>
#else
#include <fcntl.h>
#include <stdio.h>
#include <termios.h>
#include <unistd.h>
#endif

#include <stdio.h>
#include <stdlib.h>

#include "dtCore/src/dtUtils/dtTerminal.h"

namespace dt
{
namespace Utils
{

#ifdef _WIN32
// Some old MinGW/CYGWIN distributions don't define this:
#ifndef ENABLE_VIRTUAL_TERMINAL_PROCESSING
#define ENABLE_VIRTUAL_TERMINAL_PROCESSING 0x0004
#endif

static HANDLE stdoutHandle, stdinHandle;
static DWORD oriOutMode, oriInMode;
static CONSOLE_CURSOR_INFO oriCurInfo;

void Term::SetupTerminal(bool showCursor)
{
    DWORD outMode = 0, inMode = 0;
    CONSOLE_CURSOR_INFO curInfo;

    stdoutHandle = GetStdHandle(STD_OUTPUT_HANDLE);
    stdinHandle = GetStdHandle(STD_INPUT_HANDLE);

    if (stdoutHandle == INVALID_HANDLE_VALUE || stdinHandle == INVALID_HANDLE_VALUE)
    {
        exit(GetLastError());
    }

    if (!GetConsoleMode(stdoutHandle, &outMode) || !GetConsoleMode(stdinHandle, &inMode) || !GetConsoleCursorInfo(stdoutHandle, &curInfo))
    {
        exit(GetLastError());
    }

    oriOutMode = outMode;
    oriInMode = inMode;
    oriCurInfo = curInfo;

    // Enable ANSI escape codes
    outMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;

    // Set stdin as no echo and unbuffered
    inMode &= ~(ENABLE_ECHO_INPUT | ENABLE_LINE_INPUT);

    // Set Cursor visible
    curInfo.bVisible = showCursor;

    if (!SetConsoleMode(stdoutHandle, outMode) || !SetConsoleMode(stdinHandle, inMode) || !SetConsoleCursorInfo(stdoutHandle, &curInfo))
    {
        exit(GetLastError());
    }
}

void dhTerm::RestoreTerminal(void)
{
    // Reset colors
    printf("\x1b[0m");

    // Reset console mode
    if (!SetConsoleMode(stdoutHandle, oriOutMode) || !SetConsoleMode(stdinHandle, oriInMode) || !SetConsoleCursorInfo(stdoutHandle, &oriCurInfo))
    {
        exit(GetLastError());
    }
}

#else

static struct termios oriTerm;
static struct termios newTerm;

void Term::SetupTerminal(bool showCursor)
{
    tcgetattr(STDIN_FILENO, &oriTerm);
    newTerm = oriTerm;
    newTerm.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newTerm);
    int res = 0;
    res = showCursor ? system("setterm -cursor on") : system("setterm -cursor off");
}

void Term::RestoreTerminal(void)
{
    // Reset colors
    printf("\x1b[0m");

    // Reset console mode
    tcsetattr(STDIN_FILENO, TCSANOW, &oriTerm);

    int res = 0;
    res = system("setterm -cursor on");
}

int Term::kbhit(void)
{
    int ch;
    int oldFd;

    oldFd = fcntl(STDIN_FILENO, F_GETFL, 0);
    fcntl(STDIN_FILENO, F_SETFL, oldFd | O_NONBLOCK);
    ch = getchar();
    fcntl(STDIN_FILENO, F_SETFL, oldFd);

    if (ch != EOF)
    {
        ungetc(ch, stdin);
        return 1;
    }

    return 0;
}

#endif

void Term::GetCurPos(int *row, int *col)
{
    char buff[128];
    int indx = 0;

    printf("\x1b[6n");

    for (;;)
    {
        int cc = getchar();
        buff[indx] = (char)cc;
        indx++;
        if (cc == 'R')
        {
            buff[indx + 1] = '\0';
            break;
        }
    }

    sscanf(buff, "\x1b[%d;%dR", row, col);
    fseek(stdin, 0, SEEK_END);
}

} // namespace Utils
} // namespace dt