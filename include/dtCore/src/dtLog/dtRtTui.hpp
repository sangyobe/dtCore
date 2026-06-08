#pragma once

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
class RtTui {
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
        bool hide_header{false};  // skip label + column-header + underline rows
    };

    // Per-layout state: name + group headers + double-buffered data rows
    struct TuiLayoutData {
        char             name[32]{};
        TuiGroupHeader   group_headers[TUI_MAX_GROUPS]{};
        TuiDataBuffer    data_buf[2]{};
        std::atomic<int> data_write_idx{0};
        bool             defined{false};  // true once any group/row is configured
    };

    // Per-column format+value pair for set_row_cols().
    // Each column carries its own pre-formatted string so that different columns
    // can have different types and format specifiers in a single row update.
    //
    // Construct as:  TuiCol("%+8.3f", velocity)
    //                TuiCol("0x%04X", statusWord)
    //                TuiCol("%d",     count)
    struct TuiCol {
        char buf[TUI_DATA_COL_LEN]{};

        template<typename T>
        TuiCol(const char* fmt, T value) noexcept {
            snprintf(buf, sizeof(buf), fmt, value);
        }
    };

public:
    RtTui();
    ~RtTui();

    // ── init / stop (called from NRT context) ──────────────
    bool init();   // enter terminal raw mode
    void stop();   // restore terminal
    void tick();   // drain queue, handle keys, render (called from RtLog drain thread)

    // ── Layout management ──────────────────────────────────
    // layout_idx: 0 ~ MAX_LAYOUTS-1 (user switches via keys '1'~'9')
    void set_layout_name(int layout_idx, const char* name);

    // ── Area 1 group header setup (call once at startup) ──
    // layout_idx: 0 ~ MAX_LAYOUTS-1
    // group_idx : 0 ~ TUI_MAX_GROUPS-1
    void set_group(int layout_idx, int group_idx, const char* label_hdr, const char* col_hdrs[], int ncols);

    // Variadic convenience — all col_hdrs args must be const char*
    template<typename... Args>
    void set_group_v(int layout_idx, int group_idx, const char* label_hdr, Args... col_hdrs);

    // Headerless group — activates the group slot but renders no label row,
    // no column-header row, and no underline. Only the data rows are shown.
    // A thin separator (├───┤) is still drawn between adjacent groups.
    void set_group_no_header(int layout_idx, int group_idx);

    // ── Area 1 data update API (RT-safe) ──────────────────
    // layout_idx: 0 ~ MAX_LAYOUTS-1
    // group_idx : 0 ~ TUI_MAX_GROUPS-1
    // row_idx   : 0 ~ TUI_MAX_ROWS_PER_GROUP-1
    template<typename... Args>
    void set_row_fmt(int layout_idx, int group_idx, int row_idx, const char* label, const char* format, Args... args);

    // Per-column format: each TuiCol carries its own format string and pre-formatted value.
    // Allows mixed types and custom format specifiers (width, sign, precision) per column.
    //
    // Example:
    //   set_row_cols(0, grp, row, "R1",
    //       TuiCol("0x%04X", statusWord),   // hex int
    //       TuiCol("%+8.1f", desPos),        // signed float
    //       TuiCol("%+8d",   tgtTPU));       // signed int
    template<typename... Cols>
    void set_row_cols(int layout_idx, int group_idx, int row_idx, const char* label, Cols&&... cols);

    // Full-width text row: displays label + pre-formatted string (ignores column layout)
    void set_text_row(int layout_idx, int group_idx, int row_idx, const char* label, const char* text);

    // printf-style text row: formats args into a buffer, then calls set_text_row.
    // Equivalent to: snprintf(buf, ...); set_text_row(..., buf);
    // Example: set_text_row_fmt(0, 2, 0, "Right",
    //              "Pos:%+8.3f,%+8.3f,%+8.3f   Vel:%+8.3f,%+8.3f,%+8.3f",
    //              px, py, pz, vx, vy, vz);
    void set_text_row_fmt(int layout_idx, int group_idx, int row_idx, const char* label, const char* fmt, ...) noexcept
        __attribute__((format(printf, 6, 7)));

    // ── Area 2 log API (RT-safe) ─────────────────
    void log(spdlog::level::level_enum level, const char* fmt, ...);
    void log_v(spdlog::level::level_enum level, const char* fmt, va_list args);

    char pop_pendingkey() {
        char key = m_last_key.exchange(0, std::memory_order_relaxed);
        return key;
    }

    int get_current_layout() const noexcept {
        return m_current_layout.load(std::memory_order_relaxed);
    }

private:
    // Helper for formatting individual columns
    template<typename T>
    void _format_column(char* buf, size_t buf_size, const char* format, T value);

    // Helper for setting row data (layout- and group-index based)
    void _set_row_data(int layout_idx, int group_idx, int row_idx, const char* label, const char* cols[], int ncols);

    // Compute content-driven Area 1 height using the current layout
    int calc_area1_height() const noexcept;

    // Recursive variadic helpers for format-based columns
    template<typename T, typename... Args>
    void _format_columns_recursive(int col_idx, char cols[][TUI_DATA_COL_LEN], const char* format, T value, Args... rest);

    void _format_columns_recursive(int col_idx, char cols[][TUI_DATA_COL_LEN], const char* format);

    // Terminal control (direct ANSI escape codes, no ncurses)
    void term_init();
    void term_restore();
    void term_get_size(int& rows, int& cols);

    // Rendering
    void render_area1(int start_row, int height, int width);
    void render_area2(int start_row, int height, int width);
    void render_scrollbar(int start_row, int height, int col, size_t total, size_t visible, size_t offset);
    void render_cmd_line(int row, int width);

    // Key input handler (buf: byte array read, len: byte count)
    void handle_key(const char* buf, ssize_t len);

    // Output buffer (single write() syscall per frame)
    void flush_output();
    void buf_append(const char* s, size_t len);
    void buf_append_str(const char* s);
    void buf_append_hbar(int n);  // append n ─ (U+2500) characters (3 bytes each)

    template<typename... Args>
    inline void safe_snprintf(const char* fmt, Args... args) {
        if (m_out_pos >= OUT_BUF_SIZE)
            return;

        int n = snprintf(m_out_buf + m_out_pos, OUT_BUF_SIZE - m_out_pos, fmt, args...);
        if (n > 0)
            m_out_pos += std::min((size_t)n, OUT_BUF_SIZE - m_out_pos - 1);
    }

    // ── member variables ─────────────────────────────────
    TuiLogQueue         m_log_queue;

    // Area 1: per-layout state (name + group headers + double-buffered data)
    TuiLayoutData       m_layouts[MAX_LAYOUTS]{};
    std::atomic<int>    m_current_layout{0};     // 0-based; key '1' → layout 0
    std::atomic<bool>   m_layout_changed{false}; // triggers screen clear on next tick

    // Area 2: log history (NRT only, no lock needed) — circular buffer
    TuiLogEntry         m_log_history[LOG_KEEP]{};
    size_t              m_log_count{0};
    size_t              m_log_head{0};
    size_t              m_scroll_offset{0};  // 0 = bottom (newest)
    bool                m_auto_scroll{true};
    std::atomic<char>   m_last_key{0};  // TUI→main key relay

    std::atomic<bool>   m_running{false};
    std::atomic<bool>   m_term_active{false};

    // Terminal state
    struct termios*     m_old_termios{nullptr};
    int                 m_term_rows{24};
    int                 m_term_cols{80};
    int                 m_prev_term_rows{0};
    int                 m_prev_term_cols{0};

    // Output double-buffer (minimizes write() syscalls)
    char                m_out_buf[OUT_BUF_SIZE];
    size_t              m_out_pos{0};
};

};  // namespace Utils
};  // namespace dt

// ═══════════════════════════════════════════════════════════════════════════
// Template Implementation (must be in header)
// ═══════════════════════════════════════════════════════════════════════════

template<typename... Args>
void dt::Utils::RtTui::set_group_v(int layout_idx, int group_idx, const char* label_hdr, Args... col_hdrs) {
    const char* cols[] = { static_cast<const char*>(col_hdrs)... };
    set_group(layout_idx, group_idx, label_hdr, cols, (int)(sizeof...(col_hdrs)));
}

template<typename... Args>
void dt::Utils::RtTui::set_row_fmt(int layout_idx, int group_idx, int row_idx,
                                    const char* label, const char* format, Args... args) {
    if (layout_idx < 0 || layout_idx >= MAX_LAYOUTS)
        return;

    if (group_idx < 0 || group_idx >= (int)TUI_MAX_GROUPS)
        return;

    if (row_idx < 0 || row_idx >= (int)TUI_MAX_ROWS_PER_GROUP)
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

    _set_row_data(layout_idx, group_idx, row_idx, label, col_ptrs, ncols);
}

template<typename... Cols>
void dt::Utils::RtTui::set_row_cols(int layout_idx, int group_idx, int row_idx, const char* label, Cols&&... cols) {
    if (layout_idx < 0 || layout_idx >= MAX_LAYOUTS)
        return;

    if (group_idx < 0 || group_idx >= (int)TUI_MAX_GROUPS)
        return;

    if (row_idx < 0 || row_idx >= (int)TUI_MAX_ROWS_PER_GROUP)
        return;

    TuiCol col_arr[] = {std::forward<Cols>(cols)...};
    int ncols = (int)sizeof...(cols);
    if (ncols > TUI_MAX_COLS) 
        ncols = TUI_MAX_COLS;

    const char* ptrs[TUI_MAX_COLS];
    for (int i = 0; i < ncols; ++i)
        ptrs[i] = col_arr[i].buf;

    _set_row_data(layout_idx, group_idx, row_idx, label, ptrs, ncols);
}

template<typename T, typename... Args>
void dt::Utils::RtTui::_format_columns_recursive(int col_idx, char cols[][TUI_DATA_COL_LEN], const char* format, T value, Args... rest) {
    if (col_idx >= TUI_MAX_COLS)
        return;

    _format_column(cols[col_idx], TUI_DATA_COL_LEN, format, value);
    _format_columns_recursive(col_idx + 1, cols, format, rest...);
}

inline void dt::Utils::RtTui::_format_columns_recursive(int col_idx, char cols[][TUI_DATA_COL_LEN], const char* format) {
    (void)col_idx;
    (void)cols;
    (void)format;
}

template<typename T>
void dt::Utils::RtTui::_format_column(char* buf, size_t buf_size, const char* format, T value) {
    if constexpr (std::is_same_v<T, bool>) {
        snprintf(buf, buf_size, "%s", value ? "true" : "false");
    }
    else if constexpr (std::is_floating_point_v<T>) {
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
