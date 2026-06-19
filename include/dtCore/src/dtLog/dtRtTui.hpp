#ifndef _DT_RTTUI_H_
#define _DT_RTTUI_H_

#include <stdint.h>
#include <stddef.h>
#include <stdarg.h>
#include <atomic>
#include <spdlog/spdlog.h>

#include "dtLogQueue.hpp"

namespace dt
{
namespace Utils
{

// ───────────────────────────────────────────────
// RtTui : main class
// ───────────────────────────────────────────────
class RtTui 
{
public:
    // ───────────────────────────────────────────────
    // Constants
    // ───────────────────────────────────────────────
    static constexpr size_t TUI_MAX_GROUPS         = 10;   // max groups in Area 1 per layout
    static constexpr size_t TUI_MAX_ROWS_PER_GROUP = 20;   // max data rows per group
    static constexpr size_t TUI_DATA_COL_LEN       = 24;   // max column string length
    static constexpr int    TUI_MAX_COLS           = 10;   // max columns per row
    static constexpr size_t TUI_TEXT_ROW_LEN       = 200;  // max text length for text-mode rows
    static constexpr int    AREA2_MIN_ROWS         = 5;    // minimum Area 2 height (rows)
    static constexpr size_t LOG_KEEP               = 2000; // log history circular buffer capacity
    static constexpr size_t OUT_BUF_SIZE           = 262144; // 256 KB output buffer
    static constexpr int    MAX_LAYOUTS            = 9;    // layouts switchable via keys '1'–'9'
    static constexpr size_t QUEUE_CAPACITY         = 1024;
    static constexpr size_t QUEUE_MSG_LEN          = 1024;

    // TUI uses MpscLogQueue with QUEUE_CAPACITY capacity and QUEUE_MSG_LEN-byte messages
    using TuiLogQueue = LogQueue<QUEUE_CAPACITY, QUEUE_MSG_LEN>;
    using TuiLogEntry = TuiLogQueue::Entry;

    // ───────────────────────────────────────────────
    // Area 1 data structures (double-buffered, RT-safe)
    // ───────────────────────────────────────────────
    struct TuiDataRow 
    {
        char label[TUI_DATA_COL_LEN]{};
        char col[TUI_MAX_COLS][TUI_DATA_COL_LEN]{};
        int  ncols{0};
        bool textMode{false};
        char text[TUI_TEXT_ROW_LEN]{};
    };

    struct TuiGroupRowData 
    {
        TuiDataRow rows[TUI_MAX_ROWS_PER_GROUP]{};
        int        nrows{0};
    };

    struct TuiDataBuffer 
    {
        TuiGroupRowData   groups[TUI_MAX_GROUPS]{};
        std::atomic<bool> dirty{false};
    };

    // Group header (set once at startup via set_group, no RT safety required)
    struct TuiGroupHeader 
    {
        char label[TUI_DATA_COL_LEN]{};
        char cols[TUI_MAX_COLS][TUI_DATA_COL_LEN]{};
        int  ncols{0};
        bool active{false};
        bool hideHeader{false};  // skip label + column-header + underline rows
    };

    // Per-layout state: name + group headers + double-buffered data rows
    struct TuiLayoutData 
    {
        char             name[32]{};
        TuiGroupHeader   groupHeaders[TUI_MAX_GROUPS]{};
        TuiDataBuffer    dataBuf[2]{};
        std::atomic<int> dataWriteIdx{0};
        bool             defined{false};  // true once any group/row is configured
    };

    // Per-column format+value pair for set_row_cols().
    // Each column carries its own pre-formatted string so that different columns
    // can have different types and format specifiers in a single row update.
    //
    // Construct as:  TuiCol("%+8.3f", velocity)
    //                TuiCol("0x%04X", statusWord)
    //                TuiCol("%d",     count)
    struct TuiCol 
    {
        char buf[TUI_DATA_COL_LEN]{};

        template<typename T>
        TuiCol(const char *fmt, T value) noexcept 
        {
            snprintf(buf, sizeof(buf), fmt, value);
        }
    };

public:
    RtTui();
    ~RtTui();

    // ── init / stop (called from NRT context) ──────────────
    bool Init();   // enter terminal raw mode
    void Stop();   // restore terminal
    void Tick();   // drain queue, handle keys, render (called from RtLog drain thread)

    // ── Layout management ──────────────────────────────────
    // layoutIdx: 0 ~ MAX_LAYOUTS-1 (user switches via keys '1'~'9')
    void SetLayoutName(int layoutIdx, const char *name);

    // ── Area 1 group header setup (call once at startup) ──
    // layoutIdx: 0 ~ MAX_LAYOUTS-1
    // groupIdx : 0 ~ TUI_MAX_GROUPS-1
    void SetGroup(int layoutIdx, int groupIdx, const char *labelHdr, const char *colHdrs[], int ncols);

    // Variadic convenience — all colHdrs args must be const char*
    template<typename... Args>
    void SetGroupV(int layoutIdx, int groupIdx, const char *labelHdr, Args... colHdrs);

    // Headerless group — activates the group slot but renders no label row,
    // no column-header row, and no underline. Only the data rows are shown.
    // A thin separator (├───┤) is still drawn between adjacent groups.
    void SetGroupNoHeader(int layoutIdx, int groupIdx);

    // ── Area 1 data update API (RT-safe) ──────────────────
    // layoutIdx: 0 ~ MAX_LAYOUTS-1
    // groupIdx : 0 ~ TUI_MAX_GROUPS-1
    // rowIdx   : 0 ~ TUI_MAX_ROWS_PER_GROUP-1
    template<typename... Args>
    void SetRowFmt(int layoutIdx, int groupIdx, int rowIdx, const char *label, const char *format, Args... args);

    // Per-column format: each TuiCol carries its own format string and pre-formatted value.
    // Allows mixed types and custom format specifiers (width, sign, precision) per column.
    //
    // Example:
    //   SetRowCols(0, grp, row, "R1",
    //       TuiCol("0x%04X", statusWord),   // hex int
    //       TuiCol("%+8.1f", desPos),        // signed float
    //       TuiCol("%+8d",   tgtTPU));       // signed int
    template<typename... Cols>
    void SetRowCols(int layoutIdx, int groupIdx, int rowIdx, const char *label, Cols &&...cols);

    // Full-width text row: displays label + pre-formatted string (ignores column layout)
    void SetTextRow(int layoutIdx, int groupIdx, int rowIdx, const char *label, const char *text) noexcept;

    // printf-style text row: formats args into a buffer, then calls set_text_row.
    // Equivalent to: snprintf(buf, ...); set_text_row(..., buf);
    // Example: SetTextRowFmt(0, 2, 0, "Right",
    //              "Pos:%+8.3f,%+8.3f,%+8.3f   Vel:%+8.3f,%+8.3f,%+8.3f",
    //              px, py, pz, vx, vy, vz);
    void SetTextRowFmt(int layoutIdx, int groupIdx, int rowIdx, const char *label, const char *fmt, ...) noexcept
        __attribute__((format(printf, 6, 7)));

    // ── Area 2 log API (RT-safe) ─────────────────
    void Log(spdlog::level::level_enum level, const char *fmt, ...);
    void LogV(spdlog::level::level_enum level, const char *fmt, va_list args);

    char PopPendingKey() 
    {
        char key = m_lastKey.exchange(0, std::memory_order_relaxed);
        return key;
    }

    int GetCurrentLayout() const noexcept 
    {
        return m_currentLayout.load(std::memory_order_relaxed);
    }

private:
    // ── member variables ─────────────────────────────────
    TuiLogQueue         m_logQueue;

    // Area 1: per-layout state (name + group headers + double-buffered data)
    TuiLayoutData       m_layouts[MAX_LAYOUTS]{};
    std::atomic<int>    m_currentLayout{0};     // 0-based; key '1' → layout 0
    std::atomic<bool>   m_layoutChanged{false}; // triggers screen clear on next tick

    // Area 2: log history (NRT only, no lock needed) — circular buffer
    TuiLogEntry         m_logHistory[LOG_KEEP]{};
    size_t              m_logCount{0};
    size_t              m_logHead{0};
    size_t              m_scrollOffset{0};  // 0 = bottom (newest)
    bool                m_autoScroll{true};
    std::atomic<char>   m_lastKey{0};  // TUI→main key relay

    std::atomic<bool>   m_running{false};
    std::atomic<bool>   m_termActive{false};

    // Terminal state
    std::unique_ptr<struct termios> m_oldTermios;
    int                 m_termRows{24};
    int                 m_termCols{80};
    int                 m_prevTermRows{0};
    int                 m_prevTermCols{0};
    int                 m_stdout_fd{-1};    // private O_NONBLOCK fd; STDOUT_FILENO as fallback

    // Output double-buffer (minimizes write() syscalls)
    char                m_outBuf[OUT_BUF_SIZE];
    size_t              m_outPos{0};

private:
    // Helper for formatting individual columns
    template<typename T>
    void FormatColumn(char *buf, size_t buf_size, const char *format, T value);

    // Helper for setting row data (layout- and group-index based)
    void SetRowData(int layoutIdx, int groupIdx, int rowIdx, const char *label, const char *cols[], int ncols);

    // Compute content-driven Area 1 height using the current layout
    int CalcArea1Height() const noexcept;

    // Recursive variadic helpers for format-based columns
    template<typename T, typename... Args>
    void FormatColumnsRecursive(int colIdx, char cols[][TUI_DATA_COL_LEN], const char *format, T value, Args... rest);

    void FormatColumnsRecursive(int colIdx, char cols[][TUI_DATA_COL_LEN], const char *format);

    // Terminal control (direct ANSI escape codes, no ncurses)
    void TermInit();
    void TermRestore();
    void TermGetSize(int &rows, int &cols);

    // Rendering
    void RenderArea1(int startRow, int height, int width);
    void RenderArea2(int startRow, int height, int width);
    void RenderScrollbar(int startRow, int height, int col, size_t total, size_t visible, size_t offset);
    void RenderCmdLine(int row, int width);

    // Key input handler (buf: byte array read, len: byte count)
    void HandleKey(const char *buf, ssize_t len);
 
    // Output buffer (single write() syscall per frame)
    void FlushOutput();
    void AppendData(const char *s, size_t len);
    void AppendData_str(const char *s);
    void AppendData_hbar(int n);  // append n ─ (U+2500) characters (3 bytes each)

    int Utf8CodePointCount(const char* s)
    {
        int n = 0;
        while (*s) 
        {
            if ((*s & 0xC0) != 0x80)
            {
                ++n;
            }
            ++s;
        }

        return n;
    };

    template<typename... Args>
    inline void SafeSnprintf(const char *fmt, Args... args) 
    {
        if (m_outPos >= OUT_BUF_SIZE)
        {
            return;
        }

        int n = snprintf(m_outBuf + m_outPos, OUT_BUF_SIZE - m_outPos, fmt, args...);
        if (n > 0)
        {
            m_outPos += std::min((size_t)n, OUT_BUF_SIZE - m_outPos);
        }
    }
};

};  // namespace Utils
};  // namespace dt

// ═══════════════════════════════════════════════════════════════════════════
// Template Implementation (must be in header)
// ═══════════════════════════════════════════════════════════════════════════

template<typename... Args>
void dt::Utils::RtTui::SetGroupV(int layoutIdx, int groupIdx, const char *labelHdr, Args... colHdrs) 
{
    const char *cols[] = { static_cast<const char*>(colHdrs)... };
    SetGroup(layoutIdx, groupIdx, labelHdr, cols, (int)(sizeof...(colHdrs)));
}

template<typename... Args>
void dt::Utils::RtTui::SetRowFmt(int layoutIdx, int groupIdx, int rowIdx, const char *label, const char *format, Args... args) 
{
    if (layoutIdx < 0 || layoutIdx >= MAX_LAYOUTS)
    {
        return;
    }

    if (groupIdx < 0 || groupIdx >= (int)TUI_MAX_GROUPS)
    {
        return;
    }

    if (rowIdx < 0 || rowIdx >= (int)TUI_MAX_ROWS_PER_GROUP)
    {
        return;
    }

    char cols[TUI_MAX_COLS][TUI_DATA_COL_LEN];
    memset(cols, 0, sizeof(cols));
    FormatColumnsRecursive(0, cols, format, args...);

    int ncols = sizeof...(args);
    if (ncols > TUI_MAX_COLS)
    {
        ncols = TUI_MAX_COLS;
    }

    const char* colPtrs[TUI_MAX_COLS];
    for (int i = 0; i < ncols; ++i)
    {
        colPtrs[i] = cols[i];
    }

    SetRowData(layoutIdx, groupIdx, rowIdx, label, colPtrs, ncols);
}

template<typename... Cols>
void dt::Utils::RtTui::SetRowCols(int layoutIdx, int groupIdx, int rowIdx, const char *label, Cols &&...cols) 
{
    if (layoutIdx < 0 || layoutIdx >= MAX_LAYOUTS)
        return;

    if (groupIdx < 0 || groupIdx >= (int)TUI_MAX_GROUPS)
        return;

    if (rowIdx < 0 || rowIdx >= (int)TUI_MAX_ROWS_PER_GROUP)
        return;

    TuiCol colArr[] = {std::forward<Cols>(cols)...};
    int ncols = (int)sizeof...(cols);
    if (ncols > TUI_MAX_COLS) 
    {
        ncols = TUI_MAX_COLS;
    }

    const char *ptrs[TUI_MAX_COLS];
    for (int i = 0; i < ncols; ++i)
    {
        ptrs[i] = colArr[i].buf;
    }

    SetRowData(layoutIdx, groupIdx, rowIdx, label, ptrs, ncols);
}

template<typename T, typename... Args>
void dt::Utils::RtTui::FormatColumnsRecursive(int colIdx, char cols[][TUI_DATA_COL_LEN], const char *format, T value, Args... rest) 
{
    if (colIdx >= TUI_MAX_COLS)
    {
        return;
    }

    FormatColumn(cols[colIdx], TUI_DATA_COL_LEN, format, value);
    FormatColumnsRecursive(colIdx + 1, cols, format, rest...);
}

inline void dt::Utils::RtTui::FormatColumnsRecursive(int colIdx, char cols[][TUI_DATA_COL_LEN], const char *format) 
{
    (void)colIdx;
    (void)cols;
    (void)format;
}

template<typename T>
void dt::Utils::RtTui::FormatColumn(char *buf, size_t buf_size, const char *format, T value) 
{
    if constexpr (std::is_same_v<T, bool>) 
    {
        snprintf(buf, buf_size, format, static_cast<bool>(value));
    }
    else if constexpr (std::is_floating_point_v<T>) 
    {
        snprintf(buf, buf_size, format, static_cast<double>(value));
    }
    else if constexpr (std::is_signed_v<T>) 
    {
        snprintf(buf, buf_size, format, static_cast<long long>(value));
    }
    else if constexpr (std::is_unsigned_v<T>) 
    {
        snprintf(buf, buf_size, format, static_cast<unsigned long long>(value));
    }
    else if constexpr (std::is_same_v<T, const char*> || std::is_same_v<T, char*>) 
    {
        snprintf(buf, buf_size, format, value);
    }
    else 
    {
        snprintf(buf, buf_size, "?");
    }
}

#endif  // _DT_RTTUI_H_
