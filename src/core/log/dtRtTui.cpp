#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <sys/ioctl.h>
#include <sys/wait.h>
#include <poll.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <atomic>
#include <algorithm>
#include <string>
#include <thread>

#include "dtCore/src/dtLog/dtRtTui.hpp"
#include "dtCore/src/dtLog/dtRtLog.hpp"

namespace dt
{
namespace Log 
{

// ═══════════════════════════════════════════════
// ANSI escape helpers
// ═══════════════════════════════════════════════
namespace ansi 
{
    static constexpr const char* RESET    = "\x1b[0m";
    static constexpr const char* BOLD     = "\x1b[1m";
    static constexpr const char* FG_CYAN  = "\x1b[36m";
    static constexpr const char* FG_YELLOW= "\x1b[33m";
    static constexpr const char* FG_GREEN = "\x1b[32m";
    static constexpr const char* FG_RED   = "\x1b[31m";
    static constexpr const char* FG_GRAY  = "\x1b[90m";
    static constexpr const char* FG_WHITE = "\x1b[97m";
    static constexpr const char* FG_MAGENTA = "\x1b[35m";
    static constexpr const char* CLEAR_SCREEN = "\x1b[2J";
    static constexpr const char* HIDE_CURSOR  = "\x1b[?25l";
    static constexpr const char* SHOW_CURSOR  = "\x1b[?25h";
}

// ═══════════════════════════════════════════════
// RtTui implementation
// ═══════════════════════════════════════════════
RtTui::RtTui()
{
    memset(m_outBuf, 0, sizeof(m_outBuf));
    m_oldTermios = std::make_unique<struct termios>();

    // Assign default layout names
    for (int i = 0; i < MAX_LAYOUTS; ++i)
    {
        snprintf(m_layouts[i].name, sizeof(m_layouts[i].name), "Layout %d", i + 1);
    }

    // Initialize clipboard mutex with PTHREAD_PRIO_NONE so the SCHED_OTHER clipboard
    // worker thread is accepted by Xenomai Cobalt. Default protocol (PTHREAD_PRIO_INHERIT)
    // causes cobalt_mutex_lock() to return EPERM for SCHED_OTHER (XNWEAK) threads.
    pthread_mutexattr_t mattr;
    pthread_mutexattr_init(&mattr);
    pthread_mutexattr_setprotocol(&mattr, PTHREAD_PRIO_NONE);
    pthread_mutex_init(&m_clipMtx, &mattr);
    pthread_mutexattr_destroy(&mattr);
    pthread_cond_init(&m_clipCv, nullptr);
}

RtTui::~RtTui()
{
    Stop();
    pthread_mutex_destroy(&m_clipMtx);
    pthread_cond_destroy(&m_clipCv);
}

// ───────────────────────────────────────────────
// Init / shutdown
// ───────────────────────────────────────────────
bool RtTui::Init()
{
    // already initialized
    if (m_termActive.load(std::memory_order_acquire))
        return true;

    m_clipThreadRun.store(true, std::memory_order_release);
    m_clipThread = std::thread(&RtTui::ClipboardWorkerThreadFunc, this);

    TermInit();

    // Detect clipboard method.
    // TMUX is checked first: it is set on the server side (not the client terminal),
    // so it is always present over SSH. tmux forwards OSC 52 to the outer terminal.
    // For everything else, probe the terminal directly (no env-var forwarding needed).
    if (getenv("TMUX") || ProbeOsc52())
    {
        m_clipMethod = ClipboardMethod::Osc52;
    }
    else if (getenv("WAYLAND_DISPLAY") &&
            (access("/usr/bin/wl-copy",       X_OK) == 0 ||
             access("/usr/local/bin/wl-copy", X_OK) == 0)) 
    {
        m_clipMethod = ClipboardMethod::WlCopy;
    } 
    else if (getenv("DISPLAY") && access("/usr/bin/xclip", X_OK) == 0) 
    {
        m_clipMethod = ClipboardMethod::XClip;
    } 
    else if (getenv("DISPLAY") && access("/usr/bin/xsel",  X_OK) == 0) 
    {
        m_clipMethod = ClipboardMethod::XSel;
    } 
    else 
    {
        m_clipMethod = ClipboardMethod::None;
    }

    m_running.store(true, std::memory_order_release);
    return true;
}

void RtTui::Stop()
{
    m_running.store(false, std::memory_order_release);

    // Join clipboard thread BEFORE TermRestore() closes m_stdout_fd.
    // DoClipboardWrite() (OSC 52 path) writes directly to m_stdout_fd; joining here
    // ensures no concurrent write races against the close inside TermRestore().
    if (m_clipThread.joinable()) {
        m_clipThreadRun.store(false, std::memory_order_release);
        pthread_mutex_lock(&m_clipMtx);
        pthread_cond_signal(&m_clipCv);
        pthread_mutex_unlock(&m_clipMtx);
        m_clipThread.join();
    }

    // Drain any remaining entries pushed by DrainAll() after the last Tick().
    // The drain thread has already stopped, so TryPop() is safe here.
    {
        TuiLogEntry entry;
        while (m_logQueue.TryPop(entry)) {
            if (m_logCount < LOG_KEEP) {
                m_logHistory[(m_logHead + m_logCount) % LOG_KEEP] = entry;
                m_logCount++;
            } else {
                m_logHistory[m_logHead] = entry;
                m_logHead = (m_logHead + 1) % LOG_KEEP;
            }
        }
    }

    // Capture before TermRestore() clears it: true only on the first Stop() call.
    bool const firstStop = m_termActive.load(std::memory_order_acquire);

    TermRestore();  // clear TUI, restore terminal to normal mode

    // Print the most recent log lines as plain text after terminal restore.
    // Guard with firstStop so the destructor's implicit Stop() call doesn't repeat the output.
    if (firstStop && m_logCount > 0) {
        size_t n     = std::min(m_logCount, (size_t)m_termRows);
        size_t start = m_logCount - n;
        for (size_t i = start; i < m_logCount; ++i) {
            const TuiLogEntry &e = m_logHistory[(m_logHead + i) % LOG_KEEP];
            size_t len = strnlen(e.msg, QUEUE_MSG_LEN);
            ::write(STDOUT_FILENO, e.msg, len);
            ::write(STDOUT_FILENO, "\n", 1);
        }
    }
}

// ───────────────────────────────────────────────
// Terminal control
// ───────────────────────────────────────────────
void RtTui::TermInit() 
{
    m_termActive.store(true, std::memory_order_release);
    tcgetattr(STDIN_FILENO, m_oldTermios.get());

    struct termios raw = *m_oldTermios;
    raw.c_lflag &= ~(ECHO | ICANON);  // raw mode; keep ISIG so Ctrl+C sends SIGINT
    raw.c_cc[VMIN]  = 0;
    raw.c_cc[VTIME] = 0;  // non-blocking read
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);

    // stdin non-blocking
    int flags = fcntl(STDIN_FILENO, F_GETFL, 0);
    fcntl(STDIN_FILENO, F_SETFL, flags | O_NONBLOCK);

    // Open a private stdout fd with O_NONBLOCK.
    // STDOUT_FILENO's file description is left unmodified, so printf/write(1,...)
    // from other threads never see EAGAIN during TUI operation.
    // O_APPEND prevents offset corruption when stdout is redirected to a regular file.
    m_stdout_fd = ::open("/proc/self/fd/1", O_WRONLY | O_APPEND | O_NONBLOCK | O_CLOEXEC);
    if (m_stdout_fd < 0)
    {
        // /proc unavailable: fall back to setting O_NONBLOCK on STDOUT_FILENO directly
        m_stdout_fd = STDOUT_FILENO;
        int out_flags = fcntl(STDOUT_FILENO, F_GETFL, 0);
        if (out_flags != -1)
        {
            fcntl(STDOUT_FILENO, F_SETFL, out_flags | O_NONBLOCK);
        }
    }

    AppendData_str(ansi::HIDE_CURSOR);
    AppendData_str(ansi::CLEAR_SCREEN);
    // Enable SGR mouse: button-event tracking (motion only while button held) + extended coords
    // ?1002h sends motion only when a button is pressed — avoids hover noise.
    // ?1006h enables SGR extended coordinates for terminals wider than 223 columns.
    AppendData_str("\x1b[?1002h\x1b[?1006h");
    FlushOutput();
}

void RtTui::TermRestore() 
{
    if (!m_termActive.exchange(false, std::memory_order_acq_rel))
    {
        return;
    }

    // Switch the private fd to blocking so the final flush always completes.
    // In fallback mode (m_stdout_fd == STDOUT_FILENO), also restores STDOUT_FILENO.
    if (m_stdout_fd >= 0)
    {
        int out_flags = fcntl(m_stdout_fd, F_GETFL, 0);
        if (out_flags != -1)
        {
            fcntl(m_stdout_fd, F_SETFL, out_flags & ~O_NONBLOCK);
        }
    }

    m_outPos = 0;
    // Disable SGR mouse tracking before restoring terminal
    AppendData_str("\x1b[?1002l\x1b[?1006l");
    AppendData_str("\x1b[0m");
    AppendData_str(ansi::SHOW_CURSOR);
    AppendData_str(ansi::CLEAR_SCREEN);
    AppendData_str("\x1b[1;1H");
    FlushOutput();

    if (m_stdout_fd >= 0 && m_stdout_fd != STDOUT_FILENO)
    {
        ::close(m_stdout_fd);
    }
    m_stdout_fd = -1;

    int flags = fcntl(STDIN_FILENO, F_GETFL, 0);
    if (flags != -1)
    {
        fcntl(STDIN_FILENO, F_SETFL, flags & ~O_NONBLOCK);
    }

    tcsetattr(STDIN_FILENO, TCSAFLUSH, m_oldTermios.get());
}

void RtTui::TermGetSize(int& rows, int& cols) 
{
    struct winsize ws{};
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == 0 && ws.ws_row > 0) 
    {
        rows = ws.ws_row;
        cols = ws.ws_col;
    }
    else {
        rows = 24;
        cols = 80;
    }
}

// ───────────────────────────────────────────────
// Layout management
// ───────────────────────────────────────────────
void RtTui::SetLayoutName(int layoutIdx, const char *name) 
{
    if (layoutIdx < 0 || layoutIdx >= MAX_LAYOUTS || !name)
    {
        return;
    }

    strncpy(m_layouts[layoutIdx].name, name, sizeof(m_layouts[layoutIdx].name) - 1);
    m_layouts[layoutIdx].name[sizeof(m_layouts[layoutIdx].name) - 1] = '\0';
    m_layouts[layoutIdx].defined.store(true, std::memory_order_relaxed);
}

// ───────────────────────────────────────────────
// Area 1 group header setup (once at startup)
// ───────────────────────────────────────────────
void RtTui::SetGroup(int layoutIdx, int groupIdx, const char *labelHdr, const char *colHdrs[], int ncols) 
{
    if (layoutIdx < 0 || layoutIdx >= MAX_LAYOUTS)
    {
        return;
    }

    if (groupIdx < 0 || groupIdx >= (int)TUI_MAX_GROUPS)
    {
        return;
    }

    TuiGroupHeader &gh = m_layouts[layoutIdx].groupHeaders[groupIdx];

    strncpy(gh.label, labelHdr, TUI_DATA_COL_LEN - 1);
    gh.label[TUI_DATA_COL_LEN - 1] = '\0';

    if (ncols > TUI_MAX_COLS)
    {
        ncols = TUI_MAX_COLS;
    }
    gh.ncols = ncols;

    for (int i = 0; i < ncols; ++i) 
    {
        if (colHdrs[i]) 
        {
            strncpy(gh.cols[i], colHdrs[i], TUI_DATA_COL_LEN - 1);
            gh.cols[i][TUI_DATA_COL_LEN - 1] = '\0';
        }
        else 
        {
            gh.cols[i][0] = '\0';
        }
    }

    gh.active = true;
    m_layouts[layoutIdx].defined.store(true, std::memory_order_relaxed);
}

void RtTui::SetGroupNoHeader(int layoutIdx, int groupIdx) 
{
    if (layoutIdx < 0 || layoutIdx >= MAX_LAYOUTS)
    {
        return;
    }

    if (groupIdx < 0 || groupIdx >= (int)TUI_MAX_GROUPS)
    {
        return;
    }

    TuiGroupHeader& gh = m_layouts[layoutIdx].groupHeaders[groupIdx];
    gh.label[0]   = '\0';
    gh.ncols      = 0;
    gh.hideHeader = true;
    gh.active     = true;
    m_layouts[layoutIdx].defined.store(true, std::memory_order_relaxed);
}

// ───────────────────────────────────────────────
// Area 1 data update (RT-safe)
// ───────────────────────────────────────────────
void RtTui::SetRowData(int layoutIdx, int groupIdx, int row_idx, const char *label, const char *cols[], int ncols) 
{
    if (layoutIdx < 0 || layoutIdx >= MAX_LAYOUTS)
    {
        return;
    }

    if (groupIdx < 0 || groupIdx >= (int)TUI_MAX_GROUPS)
    {
        return;
    }

    if (row_idx < 0 || row_idx >= (int)TUI_MAX_ROWS_PER_GROUP)
    {
        return;
    }

    if (ncols > TUI_MAX_COLS)
    {
        ncols = TUI_MAX_COLS;
    }

    TuiLayoutData   &layout = m_layouts[layoutIdx];
    int             widx    = layout.dataWriteIdx.load(std::memory_order_relaxed);
    TuiDataBuffer   &dbuf   = layout.dataBuf[widx];
    TuiGroupRowData &gdata  = dbuf.groups[groupIdx];
    TuiDataRow      &row    = gdata.rows[row_idx];

    strncpy(row.label, label, TUI_DATA_COL_LEN - 1);
    row.label[TUI_DATA_COL_LEN - 1] = '\0';

    row.ncols     = ncols;
    row.textMode = false;
    for (int i = 0; i < ncols; ++i) 
    {
        if (cols[i]) 
        {
            strncpy(row.col[i], cols[i], TUI_DATA_COL_LEN - 1);
            row.col[i][TUI_DATA_COL_LEN - 1] = '\0';
        }
        else 
        {
            row.col[i][0] = '\0';
        }
    }
    
    for (int i = ncols; i < TUI_MAX_COLS; ++i)
    {
        row.col[i][0] = '\0';
    }

    gdata.nrows = std::max(gdata.nrows, row_idx + 1);
    dbuf.dirty.store(true, std::memory_order_release);
    layout.defined.store(true, std::memory_order_relaxed);
}

void RtTui::SetTextRow(int layoutIdx, int groupIdx, int row_idx, const char *label, const char *text) noexcept
{
    if (layoutIdx < 0 || layoutIdx >= MAX_LAYOUTS)
    {
        return;
    }

    if (groupIdx < 0 || groupIdx >= (int)TUI_MAX_GROUPS)
    {
        return;
    }

    if (row_idx < 0 || row_idx >= (int)TUI_MAX_ROWS_PER_GROUP)
    {
        return;
    }

    TuiLayoutData   &layout = m_layouts[layoutIdx];
    int             widx    = layout.dataWriteIdx.load(std::memory_order_relaxed);
    TuiDataBuffer   &dbuf   = layout.dataBuf[widx];
    TuiGroupRowData &gdata  = dbuf.groups[groupIdx];
    TuiDataRow      &row    = gdata.rows[row_idx];

    strncpy(row.label, label, TUI_DATA_COL_LEN - 1);
    row.label[TUI_DATA_COL_LEN - 1] = '\0';

    row.textMode = true;
    strncpy(row.text, text, TUI_TEXT_ROW_LEN - 1);
    row.text[TUI_TEXT_ROW_LEN - 1] = '\0';
    row.ncols = 0;

    gdata.nrows = std::max(gdata.nrows, row_idx + 1);
    dbuf.dirty.store(true, std::memory_order_release);
    layout.defined.store(true, std::memory_order_relaxed);
}

void RtTui::SetTextRowFmt(int layoutIdx, int groupIdx, int row_idx, const char* label, const char* fmt, ...) noexcept 
{
    char buf[TUI_TEXT_ROW_LEN];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);
    SetTextRow(layoutIdx, groupIdx, row_idx, label, buf);
}

int RtTui::CalcArea1Height() const noexcept 
{
    int layoutIdx = m_currentLayout.load(std::memory_order_relaxed);
    const TuiLayoutData& layout = m_layouts[layoutIdx];

    // Use the read buffer (opposite of current write index)
    int widx = layout.dataWriteIdx.load(std::memory_order_relaxed);
    int ridx = 1 - widx;
    const TuiDataBuffer& buf = layout.dataBuf[ridx];

    int content = 0;
    bool first = true;
    for (int g = 0; g < (int)TUI_MAX_GROUPS; ++g) 
    {
        if (!layout.groupHeaders[g].active)
        {
            continue;
        }

        if (!first)
        {
            content += 1;  // group separator line
        }

        first = false;
        if (!layout.groupHeaders[g].hideHeader)
        {
            content += 2;  // header row + underline separator
        }

        content += buf.groups[g].nrows;
    }

    return (2 + content);  // +2 for top and bottom border
}

// ───────────────────────────────────────────────
// Area 2 log API (RT-safe, uses MpscLogQueue)
// ───────────────────────────────────────────────
void RtTui::Log(spdlog::level::level_enum level, const char *fmt, ...) noexcept
{
    va_list ap;
    va_start(ap, fmt);
    LogV(level, fmt, ap);
    va_end(ap);
}

void RtTui::LogV(spdlog::level::level_enum level, const char *fmt, va_list args) noexcept {
    TuiLogEntry entry;
    entry.SetV(level, 0, fmt, args);
    m_logQueue.TryPush(entry);
}

// ───────────────────────────────────────────────
// tick(): called at 25 Hz by the RtLog drain thread
// ───────────────────────────────────────────────
void RtTui::Tick() 
{
    if (!m_running.load(std::memory_order_acquire))
    {
        return;
    }

    // 1) drain log queue into circular history
    TuiLogEntry entry;
    while (m_logQueue.TryPop(entry)) 
    {
        if (m_logCount < LOG_KEEP) 
        {
            m_logHistory[(m_logHead + m_logCount) % LOG_KEEP] = entry;
            m_logCount++;
        }
        else 
        {
            m_logHistory[m_logHead] = entry;
            m_logHead = (m_logHead + 1) % LOG_KEEP;
        }

        if (m_autoScroll) 
        {
            m_scrollOffset = 0;
        }
        else 
        {
            m_scrollOffset++;
        }
    }

    // 2) update Area 2 geometry for mouse coordinate mapping (uses last frame's termRows)
    {
        int needed  = CalcArea1Height();
        int area1_h = std::max(4, std::min(needed, m_termRows - 1 - AREA2_MIN_ROWS));
        m_area2StartRow = area1_h + 1;
        m_area2Height   = m_termRows - area1_h - 1;
    }

    // 3) boundary-scroll when dragging at Area 2 edges (runs before rendering)
    if (m_mouseDragging) {
        int logAreaTop    = m_area2StartRow + 1;
        int logAreaBottom = m_area2StartRow + m_area2Height - 2;
        if (m_dragCurrentTermRow < logAreaTop) {
            m_scrollOffset++;
            if (m_scrollOffset > m_logCount)
                m_scrollOffset = m_logCount;
        } else if (m_dragCurrentTermRow > logAreaBottom) {
            if (m_scrollOffset > 0)
                m_scrollOffset--;
        }
        // Update selection end to match the (possibly scrolled) current drag position
        m_selEndLogIdx = TermRowToLogIdx(m_dragCurrentTermRow);
    }

    // 4) process key/mouse input
    // 256 bytes: fits ~15 SGR mouse events per read, reducing partial-sequence splits
    char kbuf[256]{};
    ssize_t kr = read(STDIN_FILENO, kbuf, sizeof(kbuf));
    if (kr > 0)
    {
        ssize_t pos = 0;
        while (pos < kr)
        {
            ssize_t remaining = kr - pos;

            // Try SGR mouse sequence first: \x1b[<Pb;Px;Py[Mm]
            int mBtn, mCol, mRow;
            bool mRelease;
            ssize_t mLen = ParseMouseSgr(kbuf + pos, remaining, mBtn, mCol, mRow, mRelease);
            if (mLen > 0) {
                HandleMouseEvent(mBtn, mCol, mRow, mRelease);
                pos += mLen;
                continue;
            }

            // If the sequence starts with \x1b[< it is a mouse event that failed to
            // parse (e.g. truncated at the read boundary). Skip to 'M'/'m' terminator
            // so the leftover digits/semicolons do not leak into m_lastKey.
            if (remaining >= 3 &&
                kbuf[pos] == '\x1b' && kbuf[pos + 1] == '[' && kbuf[pos + 2] == '<')
            {
                ssize_t skip = 3;
                while (pos + skip < kr &&
                       kbuf[pos + skip] != 'M' && kbuf[pos + skip] != 'm') {
                    ++skip;
                }
                if (pos + skip < kr) ++skip;  // consume terminator
                pos += skip;
                continue;
            }

            if (kbuf[pos] == '\x1b' && remaining >= 3 && kbuf[pos + 1] == '[')
            {
                ssize_t seq_len = (remaining >= 4 && kbuf[pos + 3] == '~') ? 4 : 3;
                HandleKey(kbuf + pos, seq_len);
                pos += seq_len;
            }
            else if (kbuf[pos] == '\x1b')
            {
                HandleKey(kbuf + pos, remaining);
                break;
            }
            else
            {
                HandleKey(kbuf + pos, 1);
                pos += 1;
            }
        }
    }

    // 5) update terminal size and compute layout
    TermGetSize(m_termRows, m_termCols);
    int rows    = m_termRows;
    int cols    = m_termCols;
    int needed  = CalcArea1Height();
    int area1_h = std::max(4, std::min(needed, rows - 1 - AREA2_MIN_ROWS));
    int area2_h = rows - area1_h - 1;

    // 6) clear screen on terminal resize or layout switch
    bool force_clear = m_layoutChanged.exchange(false, std::memory_order_relaxed);
    if (rows != m_prevTermRows || cols != m_prevTermCols || force_clear)
    {
        AppendData_str(ansi::CLEAR_SCREEN);
        FlushOutput();
        m_prevTermRows = rows;
        m_prevTermCols = cols;
    }

    // 7) render: pack Area1 + Area2 + cmd_line into one buffer → single write()
    FlushOutput();
    m_outPos = 0;
    RenderArea1(1, area1_h, cols);
    RenderArea2(area1_h + 1, area2_h, cols);
    RenderCmdLine(rows, cols);

    FlushOutput();
}

// ───────────────────────────────────────────────
// Area 1: multi-group data monitor
// ───────────────────────────────────────────────
void RtTui::RenderArea1(int startRow, int height, int width) 
{
    if (height < 4)
    {
        return;
    }

    int layoutIdx = m_currentLayout.load(std::memory_order_relaxed);
    TuiLayoutData& layout = m_layouts[layoutIdx];

    // Swap double buffer if dirty
    int widx = layout.dataWriteIdx.load(std::memory_order_acquire);
    int ridx = 1 - widx;

    if (layout.dataBuf[widx].dirty.load(std::memory_order_acquire))
    {
        layout.dataWriteIdx.store(1 - widx, std::memory_order_release);
        layout.dataBuf[widx].dirty.store(false, std::memory_order_release);
        ridx = widx;
    }

    const TuiDataBuffer& buf = layout.dataBuf[ridx];

    // Column width: label 21 chars + 2 border + 12 per data column
    int maxColsFit = (width - 23) / 12;
    if (maxColsFit > TUI_MAX_COLS)
    {
        maxColsFit = TUI_MAX_COLS;
    }

    if (maxColsFit < 0)
    {
        maxColsFit = 0;
    }

    int cur = startRow;
    const int bot = startRow + height - 1;

    // ── layout name in top border title ──────────────────
    char title[48];
    snprintf(title, sizeof(title), "[ %s ]", layout.name);

    int titleLen = Utf8CodePointCount(title);
    SafeSnprintf("\x1b[%d;1H%s%s┌─%s%s%s─",  cur++, ansi::BOLD, ansi::FG_CYAN, ansi::FG_YELLOW, title, ansi::FG_CYAN);
    int remain_top = width - 2 - titleLen;
    AppendData_hbar(remain_top > 0 ? remain_top : 0);
    AppendData_str("┐");
    AppendData_str(ansi::RESET);

    bool firstGroup = true;
    for (int g = 0; g < (int)TUI_MAX_GROUPS && cur < bot; ++g) 
    {
        const TuiGroupHeader& gh = layout.groupHeaders[g];
        if (!gh.active)
        {
            continue;
        }

        // ── group separator ──
        if (!firstGroup && cur < bot) 
        {
            SafeSnprintf("\x1b[%d;1H%s├", cur++, ansi::FG_CYAN);
            AppendData_hbar(width - 2);
            AppendData_str("┤");
            AppendData_str(ansi::RESET);
        }

        firstGroup = false;

        // ── group header row + underline (skipped for headerless groups) ──
        if (!gh.hideHeader) 
        {
            if (cur < bot) 
            {
                SafeSnprintf("\x1b[%d;1H%s│%s %s%-20s%s",
                               cur, ansi::FG_CYAN, ansi::BOLD, ansi::FG_YELLOW, gh.label, ansi::FG_YELLOW);

                for (int c = 0; c < maxColsFit; ++c) 
                {
                    char autoHdr[16];
                    const char* hdr;
                    if (c < gh.ncols) 
                    {
                        hdr = gh.cols[c];
                    }
                    else 
                    {
                        snprintf(autoHdr, sizeof(autoHdr), ".");
                        hdr = autoHdr;
                    }
                    SafeSnprintf(" %11s", hdr);
                }

                AppendData_str(ansi::RESET);
                AppendData_str("\x1b[K");
                SafeSnprintf("\x1b[%d;%dH%s│%s", cur, width, ansi::FG_CYAN, ansi::RESET);
                cur++;
            }

            if (cur < bot) {
                SafeSnprintf("\x1b[%d;1H%s├", cur++, ansi::FG_CYAN);
                AppendData_hbar(width - 2);
                AppendData_str("┤");
                AppendData_str(ansi::RESET);
            }
        }

        // ── data rows ──
        const TuiGroupRowData& gdata = buf.groups[g];
        for (int r = 0; r < gdata.nrows && cur < bot; ++r) 
        {
            const TuiDataRow& dr = gdata.rows[r];
            SafeSnprintf("\x1b[%d;1H%s│%s", cur, ansi::FG_CYAN, ansi::RESET);
            SafeSnprintf(" %s%-20s%s", ansi::FG_WHITE, dr.label, ansi::RESET);
            AppendData_str("\x1b[K");  // erase from end-of-label to EOL before writing content

            if (dr.textMode)
            {
                int textAvail = width - 24;
                if (textAvail > 0)
                {
                    SafeSnprintf(" %s%-*.*s%s", ansi::FG_WHITE, textAvail, textAvail, dr.text, ansi::RESET);
                }
            }
            else
            {
                for (int ci = 0; ci < maxColsFit; ++ci)
                {
                    if (ci < dr.ncols)
                    {
                        SafeSnprintf(" %s%11s%s", ansi::FG_WHITE, dr.col[ci], ansi::RESET);
                    }
                    else
                    {
                        SafeSnprintf("            ");
                    }
                }
            }

            SafeSnprintf("\x1b[%d;%dH%s│%s", cur, width, ansi::FG_CYAN, ansi::RESET);
            cur++;
        }
    }

    // ── clear empty rows between data and bottom border ──
    while (cur < bot)
    {
        SafeSnprintf("\x1b[%d;1H%s│%s\x1b[K\x1b[%d;%dH%s│%s",
                      cur, ansi::FG_CYAN, ansi::RESET,
                      cur, width, ansi::FG_CYAN, ansi::RESET);
        cur++;
    }

    // ── bottom border ──
    SafeSnprintf("\x1b[%d;1H%s└", bot, ansi::FG_CYAN);
    AppendData_hbar(width - 2);
    AppendData_str("┘");
    AppendData_str(ansi::RESET);
}

// ───────────────────────────────────────────────
// Area 2: log scroll view
// ───────────────────────────────────────────────
void RtTui::RenderArea2(int startRow, int height, int width) 
{
    if (height < 3)
    {
        return;
    }

    size_t visibleH = (size_t)(height - 2);
    size_t total     = m_logCount;

    // Maximum valid offset: oldest available message sits at the top of the viewport.
    // Setting offset beyond this would make endIdx go to 0 and show nothing.
    size_t maxOffset = (total > visibleH) ? (total - visibleH) : 0;
    size_t offset     = (m_scrollOffset > maxOffset) ? maxOffset : m_scrollOffset;

    size_t endIdx      = total - offset;
    size_t startIdx    = (endIdx > visibleH) ? (endIdx - visibleH) : 0;
    size_t linesToShow = endIdx - startIdx;

    // Store for mouse coordinate mapping (TermRowToLogIdx uses this next Tick)
    m_lastRenderStartIdx = startIdx;

    // Selection bounds (valid when dragging or selection is finalized)
    bool   doHighlight = (m_hasSelection || (m_mouseDragging && m_dragHadMotion)) && m_logCount > 0;
    size_t selMin = 0, selMax = 0;
    if (doHighlight) {
        selMin = std::min(m_selStartLogIdx, m_selEndLogIdx);
        selMax = std::max(m_selStartLogIdx, m_selEndLogIdx);
        if (selMin >= m_logCount) doHighlight = false;
        if (doHighlight && selMax >= m_logCount) selMax = m_logCount - 1;
    }

    // ── top border ──
    const char* titleColor = m_autoScroll ? ansi::FG_CYAN : ansi::FG_YELLOW;
    const char* titleText  = m_autoScroll ? "─[ LOG : AUTO SCROLL ]─" : "─[ LOG : PAUSED      ]─";

    SafeSnprintf("\x1b[%d;1H%s%s┌%s%s%s",
                  startRow, ansi::BOLD, ansi::FG_CYAN,
                  titleColor, titleText, ansi::FG_CYAN);
    int remain = width - 2 - Utf8CodePointCount(titleText);

    AppendData_hbar(remain);

    SafeSnprintf("\x1b[%d;%dH┐%s", startRow, width, ansi::RESET);

    // ── log lines ──
    for (size_t r = 0; r < visibleH; ++r)
    {
        int    screen_row = startRow + 1 + (int)r;
        size_t absIdx     = startIdx + r;
        bool   inSel      = doHighlight && (r < linesToShow) &&
                            absIdx >= selMin && absIdx <= selMax;

        // Left border (always cyan, no highlight)
        SafeSnprintf("\x1b[%d;1H%s│%s", screen_row, ansi::FG_CYAN, ansi::RESET);

        // cursor is now at col 2; apply highlight and erase to EOL
        if (inSel) {
            AppendData_str("\x1b[7m");   // reverse video ON
            AppendData_str("\x1b[K");    // fill rest of line with reversed bg
        } else {
            AppendData_str("\x1b[K");    // erase normally
        }

        if (r < linesToShow)
        {
            const TuiLogEntry& e = m_logHistory[(m_logHead + absIdx) % LOG_KEEP];
            int maxMsg = width - 4;
            if (inSel) {
                char stripped[QUEUE_MSG_LEN];
                StripAnsi(e.msg, stripped, sizeof(stripped));
                SafeSnprintf(" %.*s", maxMsg, stripped);
            } else {
                SafeSnprintf(" %.*s", maxMsg, e.msg);
            }
        }

        if (inSel) {
            AppendData_str("\x1b[27m");  // reverse video OFF
        }
        AppendData_str(ansi::RESET);
        SafeSnprintf("\x1b[%d;%dH%s│%s", screen_row, width, ansi::FG_CYAN, ansi::RESET);
    }

    // ── scrollbar ── (pass clamped offset so the thumb tracks the actual visible position)
    RenderScrollbar(startRow + 1, (int)visibleH, width - 1, total, visibleH, offset);

    // ── bottom border ──
    SafeSnprintf("\x1b[%d;1H%s└", startRow + height - 1, ansi::FG_CYAN);
    AppendData_hbar(width - 2);

    const char* hint = " ↑↓:Line  PgUp/PgDn:Page ";
    int hint_cols = Utf8CodePointCount(hint);
    int hint_col = std::max(2, width - 1 - hint_cols);
    SafeSnprintf("\x1b[%d;%dH%s%s", startRow + height - 1, hint_col, ansi::FG_GRAY, hint);
    SafeSnprintf("\x1b[%d;%dH%s─┘%s", startRow + height - 1, width - 1, ansi::FG_CYAN, ansi::RESET);
}

// ───────────────────────────────────────────────
// Scrollbar rendering
// ───────────────────────────────────────────────
void RtTui::RenderScrollbar(int startRow, int height, int col, size_t total, size_t visible, size_t offset) 
{
    if (total <= visible)
    {
        return;
    }

    size_t scrollable = total - visible;
    size_t clamped    = (offset < scrollable) ? offset : scrollable;
    int thumb_from_bottom = (int)((double)clamped / scrollable * (height - 1));
    int thumb_row = height - 1 - thumb_from_bottom;

    for (int r = 0; r < height; ++r) 
    {
        const char* sym = (r == thumb_row) ? "█" : " ";
        SafeSnprintf("\x1b[%d;%dH%s%s%s", startRow + r, col, ansi::FG_GRAY, sym, ansi::RESET);
    }
}

// ───────────────────────────────────────────────
// Key input handler
// ───────────────────────────────────────────────
void RtTui::HandleKey(const char* buf, ssize_t len) 
{
    if (len <= 0)
    {
        return;
    }

    size_t page = (size_t)(m_termRows * 6 / 10);
    if (page < 1)
    {
        page = 1;
    }

    char c = buf[0];

    // ESC sequence
    if (c == '\x1b' && len >= 3 && buf[1] == '[') 
    {
        char b2 = buf[2];
        switch (b2) 
        {
        case 'A':  // ↑ arrow
            m_scrollOffset++;
            m_autoScroll = false;
            break;

        case 'B':  // ↓ arrow
            if (m_scrollOffset > 0) 
            {
                m_scrollOffset--;
            }
            if (m_scrollOffset == 0) 
            {
                m_autoScroll = true;
            }
            break;

        case '5':  // PgUp
            if (len >= 4 && buf[3] == '~') 
            {
                m_scrollOffset += page;
                m_autoScroll = false;
            }
            break;

        case '6':  // PgDn
            if (len >= 4 && buf[3] == '~') 
            {
                if (m_scrollOffset > page)
                {
                    m_scrollOffset -= page;
                }
                else
                {
                    m_scrollOffset = 0;
                }

                if (m_scrollOffset == 0)
                {
                    m_autoScroll = true;
                }
            }
            break;

        case 'F':  // End
            m_scrollOffset = 0;
            m_autoScroll   = true;
            break;

        case 'H':  // Home
            m_scrollOffset = m_logCount;
            m_autoScroll   = false;
            break;
        }

        // Coarse upper-bound clamp (fine clamp against visible height is done in render_area2)
        if (m_scrollOffset > m_logCount)
        {
            m_scrollOffset = m_logCount;
        }
        return;
    }

    // Layout switching: keys '[', ']'
    int curr_layout = m_currentLayout.load(std::memory_order_relaxed);
    int new_layout  = curr_layout;

    if (c == '[') {
        // direction: <<
        if (curr_layout == 0) {
            new_layout = MAX_LAYOUTS - 1;
            for (int i = 0; i < MAX_LAYOUTS; i++) {
                if (!m_layouts[i].defined.load(std::memory_order_relaxed)) {
                    new_layout = i;
                    break;
                }
            }
        }
        else {
            new_layout = curr_layout - 1;
        }
    }
    else if (c == ']') {
        // direction: >>
        if (curr_layout == (MAX_LAYOUTS - 1) || !m_layouts[curr_layout].defined.load(std::memory_order_relaxed)) {
            new_layout = 0;
        }
        else {
            new_layout = curr_layout + 1;
        }
    }
    else {
        // TUI-internal keys: consumed here, not relayed to main loop via m_lastKey
        if (c == '\x1b') {  // bare ESC: clear selection
            m_hasSelection  = false;
            m_mouseDragging = false;
            return;
        }

        // All other single-char keys: relay to main loop
        m_lastKey.store(c, std::memory_order_relaxed);
        return;
    }

    if (m_currentLayout.exchange(new_layout, std::memory_order_relaxed) != new_layout)
    {
        m_layoutChanged.store(true, std::memory_order_relaxed);
    }
}

// ───────────────────────────────────────────────
// SGR mouse sequence parser
// Format: \x1b[<Pb;Px;PyM (press/motion) or \x1b[<Pb;Px;Pym (release)
// Returns byte count consumed, or 0 if not a valid SGR mouse sequence.
// ───────────────────────────────────────────────
ssize_t RtTui::ParseMouseSgr(const char *buf, ssize_t len,
                               int &btn, int &col, int &row,
                               bool &isRelease) noexcept
{
    // Minimum: \x1b[<0;1;1M = 9 bytes
    if (len < 9) return 0;
    if (buf[0] != '\x1b' || buf[1] != '[' || buf[2] != '<') return 0;

    int vals[3] = {0, 0, 0};
    int vi = 0;
    ssize_t i = 3;
    for (; i < len && vi < 3; ++i) {
        char c = buf[i];
        if (c >= '0' && c <= '9') {
            vals[vi] = vals[vi] * 10 + (c - '0');
        } else if (c == ';') {
            if (++vi >= 3) return 0;  // too many fields
        } else if (c == 'M' || c == 'm') {
            if (vi != 2) return 0;  // need exactly 3 fields
            btn       = vals[0];
            col       = vals[1];
            row       = vals[2];
            isRelease = (c == 'm');
            return i + 1;
        } else {
            return 0;
        }
    }
    return 0;  // incomplete sequence
}

// ───────────────────────────────────────────────
// Mouse event handler
// ───────────────────────────────────────────────
void RtTui::HandleMouseEvent(int button, int col, int row, bool isRelease) noexcept
{
    (void)col;

    // Scroll wheel (button 64=up, 65=down) — always active
    if (button == 64) {
        m_scrollOffset++;
        if (m_scrollOffset > m_logCount) m_scrollOffset = m_logCount;
        m_autoScroll = false;
        return;
    }
    if (button == 65) {
        if (m_scrollOffset > 0) m_scrollOffset--;
        if (m_scrollOffset == 0) m_autoScroll = true;
        return;
    }

    // Only handle left button (0=press/release, 32=motion+left held)
    bool isLeftPress   = (button == 0 && !isRelease);
    bool isLeftMotion  = (button == 32);
    bool isLeftRelease = (button == 0 && isRelease);

    int logAreaTop    = m_area2StartRow + 1;
    int logAreaBottom = m_area2StartRow + m_area2Height - 2;

    if (isLeftPress) {
        if (row >= logAreaTop && row <= logAreaBottom) {
            // Save prior selection state — checked at release to enable click-to-copy
            m_hadSelectionOnPress = m_hasSelection;
            m_savedSelStart       = m_selStartLogIdx;
            m_savedSelEnd         = m_selEndLogIdx;

            m_mouseDragging      = true;
            m_dragHadMotion      = false;
            m_hasSelection       = false;
            m_autoScroll         = false;
            m_dragCurrentTermRow = row;
            m_selStartLogIdx     = TermRowToLogIdx(row);
            m_selEndLogIdx       = m_selStartLogIdx;
        } else {
            // Clicked outside Area 2: clear selection
            m_hadSelectionOnPress = false;
            m_mouseDragging       = false;
            m_dragHadMotion       = false;
            m_hasSelection        = false;
        }
        return;
    }

    if (isLeftMotion && m_mouseDragging) {
        m_dragHadMotion      = true;
        m_dragCurrentTermRow = row;
        m_selEndLogIdx       = TermRowToLogIdx(row);
        return;
    }

    if (isLeftRelease && m_mouseDragging) {
        m_mouseDragging      = false;
        m_dragCurrentTermRow = row;
        m_selEndLogIdx       = TermRowToLogIdx(row);

        if (m_logCount == 0) return;

        if (!m_dragHadMotion) {
            // True single click (no motion events): copy PRIOR selection if one existed
            if (m_hadSelectionOnPress) {
                m_selStartLogIdx = m_savedSelStart;
                m_selEndLogIdx   = m_savedSelEnd;
                m_hasSelection   = true;
                CopySelectionToClipboard();
                m_hasSelection = false;
            }
            // No prior selection → nothing happens
        } else {
            // Drag (including 1-line same-row drag): show selection, Ctrl+C to copy
            if (std::min(m_selStartLogIdx, m_selEndLogIdx) < m_logCount)
                m_hasSelection = true;
        }
        return;
    }
}

// ───────────────────────────────────────────────
// Map a terminal row number → log history offset from m_logHead
// ───────────────────────────────────────────────
size_t RtTui::TermRowToLogIdx(int termRow) const noexcept
{
    int logAreaTop = m_area2StartRow + 1;
    int visIdx = termRow - logAreaTop;
    if (visIdx < 0) visIdx = 0;

    size_t idx = m_lastRenderStartIdx + (size_t)visIdx;
    if (m_logCount == 0) return 0;
    if (idx >= m_logCount) idx = m_logCount - 1;
    return idx;
}

// ───────────────────────────────────────────────
// Command status line (bottom row)
// ───────────────────────────────────────────────
void RtTui::RenderCmdLine(int row, int width) 
{
    int curLayout = m_currentLayout.load(std::memory_order_relaxed);

    // Count active (defined) layouts to build the switcher hint
    // Format: ▶1:Name  2:Name  3:Name  (▶ marks active)
    char switcher[256]{};
    int  sw_pos = 0;
    for (int i = 0; i < MAX_LAYOUTS && sw_pos < (int)sizeof(switcher) - 20; ++i) 
    {
        if (!m_layouts[i].defined.load(std::memory_order_relaxed))
        {
            continue;
        }

        // Truncate layout name to 8 chars for compact display
        char short_name[9]{};
        strncpy(short_name, m_layouts[i].name, 8);
        short_name[8] = '\0';

        if (i == curLayout)
        {
            sw_pos += snprintf(switcher + sw_pos, sizeof(switcher) - sw_pos, "[%d:%s] ", i + 1, short_name);
        }
        else
        {
            sw_pos += snprintf(switcher + sw_pos, sizeof(switcher) - sw_pos, " %d:%-8s ", i + 1, short_name);
        }
    }

    // Last key display
    char keyStr[16];
    char lastKey = m_lastKey.load(std::memory_order_relaxed);

    if (lastKey == 0) 
    {
        snprintf(keyStr, sizeof(keyStr), "(none)");
    }
    else if (lastKey >= 0x20 && lastKey < 0x7f) 
    {
        snprintf(keyStr, sizeof(keyStr), "'%c'", lastKey);
    }
    else 
    {
        snprintf(keyStr, sizeof(keyStr), "0x%02X", (unsigned char)lastKey);
    }

    SafeSnprintf("\x1b[%d;1H%s LAYOUTS: %s%s%s  CMD: %s%-10s%s",
                  row,
                  ansi::FG_GRAY,
                  ansi::FG_MAGENTA, switcher, ansi::RESET,
                  ansi::FG_WHITE, keyStr, ansi::RESET);

    // Selection / copy feedback
    if (m_copyFeedbackTicks > 0) {
        --m_copyFeedbackTicks;
        if (m_clipUnsupported)
            AppendData_str("  \x1b[31m[Clipboard not supported]\x1b[m");
        else
            SafeSnprintf("  \x1b[32m[Copied: %zu lines]\x1b[m", m_copiedLineCount);
    }

    AppendData_str("\x1b[K");  // clear remainder of line
}

// ───────────────────────────────────────────────
// Output buffer helpers
// ───────────────────────────────────────────────
void RtTui::AppendData(const char* s, size_t len) 
{
    if (m_outPos + len >= OUT_BUF_SIZE)
    {
        FlushOutput();
    }

    size_t n = std::min(len, (OUT_BUF_SIZE - m_outPos));
    memcpy(m_outBuf + m_outPos, s, n);
    m_outPos += n;
}

void RtTui::AppendData_str(const char* s) 
{
    AppendData(s, strlen(s));
}

void RtTui::AppendData_hbar(int n) 
{
    static constexpr char HBAR[3] = {'\xe2', '\x94', '\x80'};
    for (int i = 0; i < n; ++i)
    {
        AppendData(HBAR, 3);
    }
}

// ───────────────────────────────────────────────
// ANSI code stripper
// ───────────────────────────────────────────────
void RtTui::StripAnsi(const char *src, char *dst, size_t dstLen) noexcept
{
    size_t di = 0;
    for (size_t si = 0; src[si] && di + 1 < dstLen; ++si) {
        if (src[si] != '\x1b') {
            dst[di++] = src[si];
            continue;
        }
        // ESC sequence
        ++si;
        if (!src[si]) break;
        if (src[si] == '[') {
            // CSI: skip until a letter in @-~ range terminates it
            ++si;
            while (src[si] && !(src[si] >= '@' && src[si] <= '~')) ++si;
        } else if (src[si] == ']') {
            // OSC: skip until BEL or ESC
            ++si;
            while (src[si] && src[si] != '\x07' && src[si] != '\x1b') ++si;
        }
        // si will be incremented by the outer for loop
    }
    dst[di] = '\0';
}

// ───────────────────────────────────────────────
// Extract selected lines as plain text
// ───────────────────────────────────────────────
std::string RtTui::ExtractSelectedText() const
{
    if (m_logCount == 0) return {};

    size_t selMin = std::min(m_selStartLogIdx, m_selEndLogIdx);
    size_t selMax = std::max(m_selStartLogIdx, m_selEndLogIdx);
    if (selMin >= m_logCount) return {};
    if (selMax >= m_logCount) selMax = m_logCount - 1;

    std::string result;
    char stripped[QUEUE_MSG_LEN];
    for (size_t i = selMin; i <= selMax; ++i) {
        const TuiLogEntry &e = m_logHistory[(m_logHead + i) % LOG_KEEP];
        StripAnsi(e.msg, stripped, sizeof(stripped));
        result += stripped;
        result += '\n';
    }
    return result;
}

// ───────────────────────────────────────────────
// Signal clipboard worker thread
// ───────────────────────────────────────────────
void RtTui::CopySelectionToClipboard()
{
    if (m_clipMethod == ClipboardMethod::None) {
        m_clipUnsupported   = true;
        m_copyFeedbackTicks = 25;
        return;
    }

    std::string text = ExtractSelectedText();
    if (text.empty()) return;

    size_t nLines = (m_selEndLogIdx >= m_selStartLogIdx)
                    ? (m_selEndLogIdx - m_selStartLogIdx + 1)
                    : (m_selStartLogIdx - m_selEndLogIdx + 1);

    m_clipUnsupported = false;

    if (m_clipMethod == ClipboardMethod::Osc52) 
    {
        // Write OSC 52 directly into m_outBuf (we are in drain-thread / Tick() context).
        // FlushOutput() at step 7 of Tick() sends it atomically with the render frame,
        // avoiding concurrent writes to the PTY from the clipboard worker thread.
        std::string encoded = Base64Encode(text);
        std::string seq = "\x1b]52;c;" + encoded + "\x07";
        AppendData(seq.c_str(), seq.size());
    } 
    else 
    {
        // wl-copy / xclip / xsel: delegate to worker thread (requires fork/exec)
        pthread_mutex_lock(&m_clipMtx);
        m_clipText    = std::move(text);
        m_clipPending = true;
        pthread_mutex_unlock(&m_clipMtx);
        pthread_cond_signal(&m_clipCv);
    }

    // Feedback display: ~1 second at 25 Hz
    m_copiedLineCount   = nLines;
    m_copyFeedbackTicks = 25;
}

// ───────────────────────────────────────────────
// Base64 encoder (for OSC 52)
// ───────────────────────────────────────────────
std::string RtTui::Base64Encode(const std::string &input)
{
    static const char kTable[] =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::string out;
    out.reserve((input.size() + 2) / 3 * 4);
    for (size_t i = 0; i < input.size(); i += 3) {
        unsigned char b0 = (unsigned char)input[i];
        unsigned char b1 = (i + 1 < input.size()) ? (unsigned char)input[i + 1] : 0;
        unsigned char b2 = (i + 2 < input.size()) ? (unsigned char)input[i + 2] : 0;
        out += kTable[b0 >> 2];
        out += kTable[((b0 & 3) << 4) | (b1 >> 4)];
        out += (i + 1 < input.size()) ? kTable[((b1 & 0xf) << 2) | (b2 >> 6)] : '=';
        out += (i + 2 < input.size()) ? kTable[b2 & 0x3f] : '=';
    }
    return out;
}

// ───────────────────────────────────────────────
// Fork/exec helper — pipe text to command's stdin
// Blocking: must be called from the clipboard worker thread only.
// ───────────────────────────────────────────────
void RtTui::RunPipe(const char *cmd, const std::string &text) noexcept
{
    int pipefd[2];
    if (pipe(pipefd) != 0) return;

    pid_t pid = fork();
    if (pid == 0) {
        // Child: redirect stdin from pipe and exec
        if (dup2(pipefd[0], STDIN_FILENO) < 0) _exit(1);
        close(pipefd[0]);
        close(pipefd[1]);
        execl("/bin/sh", "sh", "-c", cmd, nullptr);
        _exit(1);
    } else if (pid > 0) {
        // Parent: write text then close; blocking wait is OK in worker thread
        close(pipefd[0]);
        const char *p   = text.c_str();
        size_t      rem = text.size();
        while (rem > 0) {
            ssize_t n = write(pipefd[1], p, rem);
            if (n <= 0) break;
            p   += n;
            rem -= (size_t)n;
        }
        close(pipefd[1]);
        waitpid(pid, nullptr, 0);
    } else {
        close(pipefd[0]);
        close(pipefd[1]);
    }
}


// Probe for OSC 52 support. Two-stage detection:
//   1. XTVERSION (\x1b[>q) — identifies terminal by name.
//      Windows Terminal supports OSC 52 write but NOT the read query (\x1b]52;c;?\x07),
//      so it must be detected via name, not via read response.
//   2. OSC 52 read query — for terminals that respond to the read probe.
// Must be called after TermInit() (stdin in raw + O_NONBLOCK mode).
bool RtTui::ProbeOsc52() noexcept
{
    // ── Stage 1: XTVERSION (\x1b[>q) ────────────────────────────────────────
    // Terminals that support OSC 52 write typically respond to XTVERSION with
    // their name and version: \x1bP>|<Name>(<Version>)\x1b\\
    { char d[64]; while (::read(STDIN_FILENO, d, sizeof(d)) > 0) {} }  // drain

    if (::write(STDOUT_FILENO, "\x1b[>q", 4) == 4) {
        struct pollfd pfd = {STDIN_FILENO, POLLIN, 0};
        if (poll(&pfd, 1, 200) > 0 && (pfd.revents & POLLIN)) {
            char buf[512]{};
            ssize_t n = ::read(STDIN_FILENO, buf, sizeof(buf) - 1);
            if (n > 0) {
                buf[n] = '\0';
                // Terminals confirmed to support OSC 52 write via SSH.
                // Windows Terminal is intentionally excluded: it responds to XTVERSION
                // but does NOT support OSC 52 write through SSH sessions.
                if (strstr(buf, "WezTerm") ||
                    strstr(buf, "kitty")   ||
                    strstr(buf, "iTerm2"))
                    return true;
                // VTE: OSC 52 write added in version 0.70
                const char *vte = strstr(buf, "VTE(");
                if (vte) {
                    int major = 0, minor = 0;
                    if (sscanf(vte + 4, "%d.%d", &major, &minor) == 2 &&
                        (major > 0 || minor >= 70))
                        return true;
                }
            }
        }
    }

    // ── Stage 2: OSC 52 read query (\x1b]52;c;?\x07) ────────────────────────
    // For terminals not identified above that respond to the clipboard read request.
    { char d[64]; while (::read(STDIN_FILENO, d, sizeof(d)) > 0) {} }  // drain

    if (::write(STDOUT_FILENO, "\x1b]52;c;?\x07", 9) <= 0) return false;

    struct pollfd pfd2 = {STDIN_FILENO, POLLIN, 0};
    if (poll(&pfd2, 1, 150) <= 0 || !(pfd2.revents & POLLIN)) return false;

    char buf2[256]{};
    ssize_t n2 = ::read(STDIN_FILENO, buf2, sizeof(buf2) - 1);
    if (n2 <= 0) return false;

    // Response: \x1b]52;c;<base64_data>\x07 or \x1b]52;c;\x07 (empty clipboard)
    for (ssize_t i = 0; i + 4 < n2; ++i) {
        if (buf2[i]   == '\x1b' && buf2[i+1] == ']' &&
            buf2[i+2] == '5'   && buf2[i+3] == '2'  && buf2[i+4] == ';')
            return true;
    }
    return false;
}

// ───────────────────────────────────────────────
// Clipboard worker thread — waits for copy requests, never blocks drain thread
// ───────────────────────────────────────────────
void RtTui::ClipboardWorkerThreadFunc()
{
    while (true) {
        std::string text;
        pthread_mutex_lock(&m_clipMtx);
        while (!m_clipPending && m_clipThreadRun.load(std::memory_order_relaxed)) {
            pthread_cond_wait(&m_clipCv, &m_clipMtx);
        }
        bool const shouldExit = !m_clipThreadRun.load(std::memory_order_relaxed);
        if (m_clipPending) {
            text          = std::move(m_clipText);
            m_clipPending = false;
        }
        pthread_mutex_unlock(&m_clipMtx);

        if (!text.empty()) DoClipboardWrite(text);
        if (shouldExit) break;
    }
}

// ───────────────────────────────────────────────
// Actual clipboard write (runs in worker thread)
// ───────────────────────────────────────────────
void RtTui::DoClipboardWrite(const std::string &text) noexcept
{
    if (text.empty()) return;

    switch (m_clipMethod) {
    case ClipboardMethod::Osc52: {
        std::string encoded = Base64Encode(text);
        std::string seq = "\x1b]52;c;" + encoded + "\x07";
        // Use blocking STDOUT_FILENO (not m_stdout_fd which is O_NONBLOCK) so the
        // full sequence is always written. The clipboard worker thread may block here.
        const char *p   = seq.c_str();
        size_t      rem = seq.size();
        while (rem > 0) {
            ssize_t n = ::write(STDOUT_FILENO, p, rem);
            if (n > 0) { p += (size_t)n; rem -= (size_t)n; }
            else break;
        }
        break;
    }
    case ClipboardMethod::WlCopy:
        RunPipe("wl-copy", text);
        break;
    case ClipboardMethod::XClip:
        RunPipe("xclip -selection clipboard", text);
        break;
    case ClipboardMethod::XSel:
        RunPipe("xsel --clipboard --input", text);
        break;
    case ClipboardMethod::None:
        break;
    }
}

void RtTui::FlushOutput()
{
    if (m_outPos == 0)
    {
        return;
    }

    size_t written = 0;
    while (written < m_outPos) 
    {
        ssize_t n = write(m_stdout_fd, m_outBuf + written, m_outPos - written);
        if (n < 0) 
        {
            if (errno == EINTR)
            {
                continue;
            }

            if (errno == EAGAIN || errno == EWOULDBLOCK) 
            {
                // pty buffer is full (expected: stdout is O_NONBLOCK).
                // Wait up to 10 ms for the terminal to drain before giving up.
                // A timeout is normal backpressure — not an error.
                struct pollfd pfd = {m_stdout_fd, POLLOUT, 0};
                if (poll(&pfd, 1, 3) > 0 && (pfd.revents & POLLOUT))
                {
                    continue;
                }
                // Still not writable: retain unwritten bytes, retry next frame.
                break;
            }

            // Real write error (EPIPE, EBADF, …): log once and discard remaining
            // bytes to prevent the error from being logged again every frame.
            static std::chrono::steady_clock::time_point last_times;
            auto now = std::chrono::steady_clock::now();
            auto interval = std::chrono::duration_cast<std::chrono::milliseconds>(now - last_times).count();
            if (interval >= 1000) {
                last_times = now;
                char errbuf[64];
                const char* errstr = strerror_r(errno, errbuf, sizeof(errbuf));
                LOG(err).printf("[TUI] write failed (%s): dropped %zu bytes", errstr, m_outPos - written);
            }
            m_outPos = written;

            return;
        }

        if (n == 0)
        {
            break;
        }

        written += (size_t)n;
    }

    size_t remaining = m_outPos - written;
    if (remaining > 0 && written > 0)
    {
        std::memmove(m_outBuf, m_outBuf + written, remaining);
    }

    m_outPos = remaining;
}

}  // namespace Log
}  // namespace dt
