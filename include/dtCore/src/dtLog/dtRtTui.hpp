#pragma once

#include <stdint.h>
#include <stddef.h>
#include <stdarg.h>
#include <atomic>
#include <spdlog/spdlog.h>

#include "dtLogQueue.hpp"

// ───────────────────────────────────────────────
// Constants
// ───────────────────────────────────────────────
static constexpr size_t TUI_MAX_GROUPS         = 4;   // max groups in Area 1
static constexpr size_t TUI_MAX_ROWS_PER_GROUP = 20;  // max data rows per group
static constexpr size_t TUI_DATA_COL_LEN       = 24;  // max column string length
static constexpr int    TUI_MAX_COLS           = 8;   // max columns per row
static constexpr size_t TUI_TEXT_ROW_LEN       = 200; // max text length for text-mode rows
static constexpr int    AREA2_MIN_ROWS         = 5;   // minimum Area 2 height (rows)

// TUI uses MpscLogQueue with 512 capacity and 256-byte messages
using TuiLogQueue = LogQueue<512, 256>;
using TuiLogEntry = TuiLogQueue::Entry;

// ───────────────────────────────────────────────
// Area 1 data structures (double-buffered, RT-safe)
// ───────────────────────────────────────────────
struct TuiDataRow {
    char label[TUI_DATA_COL_LEN]{};
    char col[TUI_MAX_COLS][TUI_DATA_COL_LEN]{};
    int  ncols{0};
    bool text_mode{false};
    char text[TUI_TEXT_ROW_LEN]{};
};

struct TuiGroupRowData {
    TuiDataRow rows[TUI_MAX_ROWS_PER_GROUP]{};
    int        nrows{0};
};

struct TuiDataBuffer {
    TuiGroupRowData   groups[TUI_MAX_GROUPS]{};
    std::atomic<bool> dirty{false};
};

// Group header (set once at startup via set_group, no RT safety required)
struct TuiGroupHeader {
    char label[TUI_DATA_COL_LEN]{};
    char cols[TUI_MAX_COLS][TUI_DATA_COL_LEN]{};
    int  ncols{0};
    bool active{false};
};

// ───────────────────────────────────────────────
// RtTui : main class
// ───────────────────────────────────────────────
class RtTui {
public:
    RtTui();
    ~RtTui();

    // ── init / stop (called from NRT context) ──────────────
    bool init();   // enter terminal raw mode
    void stop();   // restore terminal
    void tick();   // drain queue, handle keys, render (called from RtLog drain thread)

    // ── Area 1 group header setup (call once at startup) ──
    // group_idx : 0 ~ TUI_MAX_GROUPS-1
    // label_hdr : header name for the left label column
    // col_hdrs  : array of data column header names
    // Example: set_group(0, "Joint", cols, 6);
    void set_group(int group_idx, const char* label_hdr, const char* col_hdrs[], int ncols);

    // Variadic convenience — all args must be const char*
    // Example: set_group_v(0, "Joint", "J1","J2","J3","J4","J5","J6")
    //          set_group_v(1, "Task",  "X", "Y", "Z")
    template<typename... Args>
    void set_group_v(int group_idx, const char* label_hdr, Args... col_hdrs);

    // ── Area 1 data update API (RT-safe) ──────────
    // group_idx : 0 ~ TUI_MAX_GROUPS-1
    // row_idx   : 0 ~ TUI_MAX_ROWS_PER_GROUP-1
    // Example: set_row_fmt(0, 2, "ABS_Enc", "%.2f", j1, j2, j3, j4, j5, j6)
    template<typename... Args>
    void set_row_fmt(int group_idx, int row_idx, const char* label, const char* format, Args... args);

    // Example: set_row_v(0, 0, "ABS_Enc", j1, j2, j3, j4, j5, j6)
    template<typename... Args>
    void set_row_v(int group_idx, int row_idx, const char* label, Args... args);

    // Full-width text row: displays label + free-form string (ignores column layout)
    // Example: set_text_row(1, 1, "cmd", "movej(posd=[...], time=5.0)")
    void set_text_row(int group_idx, int row_idx, const char* label, const char* text);

    // ── Area 2 log API (RT-safe) ─────────────────
    void log(spdlog::level::level_enum level, const char* fmt, ...);
    void log_v(spdlog::level::level_enum level, const char* fmt, va_list args);

    // convenience wrappers for macros
    void log_trace(const char* fmt, ...);
    void log_debug(const char* fmt, ...);
    void log_info (const char* fmt, ...);
    void log_warn (const char* fmt, ...);
    void log_error(const char* fmt, ...);
    void log_critical(const char* fmt, ...);

    char pop_pendingkey() {
        char key = m_last_key.exchange(0, std::memory_order_relaxed);
        return key;
    }

private:
    // Helper for formatting individual columns
    template<typename T>
    void _format_column(char* buf, size_t buf_size, const char* format, T value);

    // Helper for setting row data (group_idx, row_idx based)
    void _set_row_data(int group_idx, int row_idx,
                       const char* label, const char* cols[], int ncols);

    // Compute content-driven Area 1 height (used by tick())
    int calc_area1_height() const noexcept;

    // Recursive variadic helpers
    template<typename T, typename... Args>
    void _format_columns_recursive(int col_idx, char cols[][TUI_DATA_COL_LEN],
                                   const char* format, T value, Args... rest);

    void _format_columns_recursive(int col_idx, char cols[][TUI_DATA_COL_LEN],
                                   const char* format);

    // Auto-format helpers
    template<typename T, typename... Args>
    void _auto_format_recursive(int col_idx, char cols[][TUI_DATA_COL_LEN],
                               T value, Args... rest);

    void _auto_format_recursive(int col_idx, char cols[][TUI_DATA_COL_LEN]);

    // terminal control (direct ANSI escape codes, no ncurses)
    void term_init();
    void term_restore();
    void term_get_size(int& rows, int& cols);

    // rendering
    void render_area1(int start_row, int height, int width);
    void render_area2(int start_row, int height, int width);
    void render_scrollbar(int start_row, int height, int col,
                          size_t total, size_t visible, size_t offset);
    void render_cmd_line(int row, int width);

    // key input handler (buf: byte array read, len: byte count)
    void handle_key(const char* buf, ssize_t len);

    // output buffer flush (single write() syscall)
    void flush_output();
    void buf_append(const char* s, size_t len);
    void buf_append_str(const char* s);
    void buf_append_hbar(int n);  // append n ─ (U+2500) characters (3 bytes each)

    // ── member variables ─────────────────────────────────
    TuiLogQueue   m_log_queue;

    // Area 1: group headers (set once at startup via set_group)
    TuiGroupHeader m_group_headers[TUI_MAX_GROUPS]{};

    // Area 1: double buffer (write=RT, read=render)
    TuiDataBuffer    m_data_buf[2]{};
    std::atomic<int> m_data_write_idx{0};

    // Area 2: log history (NRT only, no lock needed) — circular buffer
    static constexpr size_t LOG_KEEP = 2000;
    TuiLogEntry   m_log_history[LOG_KEEP]{};
    size_t        m_log_count{0};
    size_t        m_log_head{0};
    size_t        m_scroll_offset{0};  // 0 = bottom
    bool          m_auto_scroll{true};
    std::atomic<char> m_last_key{0};  // TUI→main key relay (RT writes, main reads)

    std::atomic<bool> m_running{false};
    std::atomic<bool> m_term_active{false};

    // terminal state
    struct termios*   m_old_termios{nullptr};
    int               m_term_rows{24};
    int               m_term_cols{80};
    int               m_prev_term_rows{0};
    int               m_prev_term_cols{0};

    // output double-buffer (minimizes write() syscalls)
    // 256 KB ensures a full frame fits even on very wide terminals (500+ cols) without mid-frame flush
    static constexpr size_t OUT_BUF_SIZE = 262144;
    char   m_out_buf[OUT_BUF_SIZE];
    size_t m_out_pos{0};
};

// ───────────────────────────────────────────────
// Convenience macros (use spdlog level enum)
// ───────────────────────────────────────────────
#define TUI_TRACE(tui, fmt, ...)    (tui).log(spdlog::level::trace,    fmt, ##__VA_ARGS__)
#define TUI_DEBUG(tui, fmt, ...)    (tui).log(spdlog::level::debug,    fmt, ##__VA_ARGS__)
#define TUI_INFO(tui, fmt, ...)     (tui).log(spdlog::level::info,     fmt, ##__VA_ARGS__)
#define TUI_WARN(tui, fmt, ...)     (tui).log(spdlog::level::warn,     fmt, ##__VA_ARGS__)
#define TUI_ERROR(tui, fmt, ...)    (tui).log(spdlog::level::err,      fmt, ##__VA_ARGS__)
#define TUI_CRITICAL(tui, fmt, ...) (tui).log(spdlog::level::critical, fmt, ##__VA_ARGS__)

// ═══════════════════════════════════════════════════════════════════════════
// Template Implementation (must be in header)
// ═══════════════════════════════════════════════════════════════════════════

// set_group_v: variadic convenience (all args must be const char*)
// Example: tui.set_group_v(0, "Joint", "J1","J2","J3")
template<typename... Args>
void RtTui::set_group_v(int group_idx, const char* label_hdr, Args... col_hdrs) {
    const char* cols[] = { static_cast<const char*>(col_hdrs)... };
    set_group(group_idx, label_hdr, cols, (int)(sizeof...(col_hdrs)));
}

// Format-based set_row (single format applied to all columns)
template<typename... Args>
void RtTui::set_row_fmt(int group_idx, int row_idx, const char* label, const char* format, Args... args) {
    if (group_idx < 0 || group_idx >= (int)TUI_MAX_GROUPS)
        return;

    if (row_idx  < 0 || row_idx  >= (int)TUI_MAX_ROWS_PER_GROUP)
        return;

    char cols[TUI_MAX_COLS][TUI_DATA_COL_LEN];
    memset(cols, 0, sizeof(cols));
    _format_columns_recursive(0, cols, format, args...);

    int ncols = sizeof...(args);
    if (ncols > TUI_MAX_COLS)
        ncols = TUI_MAX_COLS;

    const char* col_ptrs[TUI_MAX_COLS];
    for (int i = 0; i < ncols; ++i)
        col_ptrs[i] = cols[i];

    _set_row_data(group_idx, row_idx, label, col_ptrs, ncols);
}

// Auto-format variadic set_row (detects types automatically)
template<typename... Args>
void RtTui::set_row_v(int group_idx, int row_idx, const char* label, Args... args) {
    if (group_idx < 0 || group_idx >= (int)TUI_MAX_GROUPS)
        return;

    if (row_idx  < 0 || row_idx  >= (int)TUI_MAX_ROWS_PER_GROUP)
        return;

    char cols[TUI_MAX_COLS][TUI_DATA_COL_LEN];
    memset(cols, 0, sizeof(cols));
    _auto_format_recursive(0, cols, args...);

    int ncols = sizeof...(args);
    if (ncols > TUI_MAX_COLS)
        ncols = TUI_MAX_COLS;

    const char* col_ptrs[TUI_MAX_COLS];
    for (int i = 0; i < ncols; ++i)
        col_ptrs[i] = cols[i];

    _set_row_data(group_idx, row_idx, label, col_ptrs, ncols);
}

// Recursive helper for format-based columns
template<typename T, typename... Args>
void RtTui::_format_columns_recursive(int col_idx, char cols[][TUI_DATA_COL_LEN],
                                     const char* format, T value, Args... rest) {
    if (col_idx >= TUI_MAX_COLS)
        return;
    _format_column(cols[col_idx], TUI_DATA_COL_LEN, format, value);
    _format_columns_recursive(col_idx + 1, cols, format, rest...);
}

// Base case for format-based recursion
inline void RtTui::_format_columns_recursive(int col_idx, char cols[][TUI_DATA_COL_LEN],
                                            const char* format) {
    (void)col_idx;
    (void)cols;
    (void)format;
}

// Recursive helper for auto-format columns
template<typename T, typename... Args>
void RtTui::_auto_format_recursive(int col_idx, char cols[][TUI_DATA_COL_LEN], T value, Args... rest) {
    if (col_idx >= TUI_MAX_COLS)
        return;

    if constexpr (std::is_same_v<T, const char*> || std::is_same_v<T, char*>) {
        snprintf(cols[col_idx], TUI_DATA_COL_LEN, "%s", value);
    }
    else if constexpr (std::is_floating_point_v<T>) {
        snprintf(cols[col_idx], TUI_DATA_COL_LEN, "%.3f", static_cast<double>(value));
    }
    else if constexpr (std::is_signed_v<T>) {
        snprintf(cols[col_idx], TUI_DATA_COL_LEN, "%lld", static_cast<long long>(value));
    }
    else if constexpr (std::is_unsigned_v<T>) {
        snprintf(cols[col_idx], TUI_DATA_COL_LEN, "%llu", static_cast<unsigned long long>(value));
    }
    else {
        snprintf(cols[col_idx], TUI_DATA_COL_LEN, "?");
    }

    _auto_format_recursive(col_idx + 1, cols, rest...);
}

// Base case for auto-format recursion
inline void RtTui::_auto_format_recursive(int col_idx, char cols[][TUI_DATA_COL_LEN]) {
    (void)col_idx;
    (void)cols;
}

// Format single column helper
template<typename T>
void RtTui::_format_column(char* buf, size_t buf_size, const char* format, T value) {
    if constexpr (std::is_floating_point_v<T>) {
        snprintf(buf, buf_size, format, static_cast<double>(value));
    }
    else if constexpr (std::is_signed_v<T>) {
        snprintf(buf, buf_size, format, static_cast<long long>(value));
    }
    else if constexpr (std::is_unsigned_v<T>) {
        snprintf(buf, buf_size, format, static_cast<unsigned long long>(value));
    }
    else if constexpr (std::is_same_v<T, const char*> || std::is_same_v<T, char*>) {
        snprintf(buf, buf_size, format, value);
    }
    else {
        snprintf(buf, buf_size, "?");
    }
}
