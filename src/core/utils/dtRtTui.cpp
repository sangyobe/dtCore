#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <sys/ioctl.h>
#include <poll.h>
#include <string.h>
#include <stdio.h>
#include <atomic>
#include <algorithm>
#include <iostream>

#include "dtCore/src/dtLog/dtRtTui.hpp"
#include "dtCore/src/dtLog/dtRtLog.hpp"

// ═══════════════════════════════════════════════
// ANSI escape helpers
// ═══════════════════════════════════════════════
namespace ansi {
    static constexpr const char* RESET    = "\x1b[0m";
    static constexpr const char* BOLD     = "\x1b[1m";
    static constexpr const char* FG_CYAN  = "\x1b[36m";
    static constexpr const char* FG_YELLOW= "\x1b[33m";
    static constexpr const char* FG_GREEN = "\x1b[32m";
    static constexpr const char* FG_RED   = "\x1b[31m";
    static constexpr const char* FG_GRAY  = "\x1b[90m";
    static constexpr const char* FG_WHITE = "\x1b[97m";
    static constexpr const char* CLEAR_SCREEN = "\x1b[2J";
    static constexpr const char* HIDE_CURSOR  = "\x1b[?25l";
    static constexpr const char* SHOW_CURSOR  = "\x1b[?25h";
}


// ═══════════════════════════════════════════════
// RtTui implementation (uses MpscLogQueue from rtLogQueue.hpp)
// ═══════════════════════════════════════════════
RtTui::RtTui() {
    memset(m_out_buf, 0, sizeof(m_out_buf));
    m_old_termios = new struct termios();
}

RtTui::~RtTui() {
    stop();
    delete m_old_termios;
}

// ───────────────────────────────────────────────
// Init / shutdown
// ───────────────────────────────────────────────
bool RtTui::init() {
    term_init();
    m_running.store(true, std::memory_order_release);
    return true;
}

void RtTui::stop() {
    m_running.store(false, std::memory_order_release);
    term_restore();  // m_term_active guards against double-call
}

// ───────────────────────────────────────────────
// Terminal control
// ───────────────────────────────────────────────
void RtTui::term_init() {
    m_term_active.store(true, std::memory_order_release);
    tcgetattr(STDIN_FILENO, m_old_termios);

    struct termios raw = *m_old_termios;
    raw.c_lflag &= ~(ECHO | ICANON | ISIG);  // raw mode
    raw.c_cc[VMIN]  = 0;
    raw.c_cc[VTIME] = 0;  // non-blocking read
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);

    // stdin non-blocking
    int flags = fcntl(STDIN_FILENO, F_GETFL, 0);
    fcntl(STDIN_FILENO, F_SETFL, flags | O_NONBLOCK);

    // stdout non-blocking: prevents drain thread from blocking on terminal writes
    // flush_output() skips the frame on EAGAIN to prioritize RT queue draining
    int out_flags = fcntl(STDOUT_FILENO, F_GETFL, 0);
    if (out_flags != -1)
        fcntl(STDOUT_FILENO, F_SETFL, out_flags | O_NONBLOCK);

    buf_append_str(ansi::HIDE_CURSOR);
    buf_append_str(ansi::CLEAR_SCREEN);
    flush_output();
}

void RtTui::term_restore() {
    // exchange prevents double-restore: return immediately if already restored
    if (!m_term_active.exchange(false, std::memory_order_acq_rel))
        return;

    // Restore stdout to blocking so the restore sequence (cursor/screen) is fully sent
    int out_flags = fcntl(STDOUT_FILENO, F_GETFL, 0);
    if (out_flags != -1)
        fcntl(STDOUT_FILENO, F_SETFL, out_flags & ~O_NONBLOCK);

    m_out_pos = 0;
    buf_append_str("\x1b[0m");       // reset ANSI attributes
    buf_append_str(ansi::SHOW_CURSOR);
    buf_append_str(ansi::CLEAR_SCREEN);
    buf_append_str("\x1b[1;1H");     // move cursor to (1,1)
    flush_output();

    // restore stdin to blocking mode
    int flags = fcntl(STDIN_FILENO, F_GETFL, 0);
    if (flags != -1)
        fcntl(STDIN_FILENO, F_SETFL, flags & ~O_NONBLOCK);

    tcsetattr(STDIN_FILENO, TCSAFLUSH, m_old_termios);
}

void RtTui::term_get_size(int& rows, int& cols) {
    struct winsize ws{};
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == 0 && ws.ws_row > 0) {
        rows = ws.ws_row;
        cols = ws.ws_col;
    } 
    else {
        rows = 24;
        cols = 80;
    }
}

// ───────────────────────────────────────────────
// Area 1 group header setup (once at startup)
// ───────────────────────────────────────────────
void RtTui::set_group(int group_idx, const char* label_hdr, const char* col_hdrs[], int ncols) {
    if (group_idx < 0 || group_idx >= (int)TUI_MAX_GROUPS)
        return;

    TuiGroupHeader& gh = m_group_headers[group_idx];

    strncpy(gh.label, label_hdr, TUI_DATA_COL_LEN - 1);
    gh.label[TUI_DATA_COL_LEN - 1] = '\0';

    if (ncols > TUI_MAX_COLS)
        ncols = TUI_MAX_COLS;
    gh.ncols = ncols;

    for (int i = 0; i < ncols; ++i) {
        if (col_hdrs[i]) {
            strncpy(gh.cols[i], col_hdrs[i], TUI_DATA_COL_LEN - 1);
            gh.cols[i][TUI_DATA_COL_LEN - 1] = '\0';
        }
        else {
            gh.cols[i][0] = '\0';
        }
    }
    gh.active = true;
}

// ───────────────────────────────────────────────
// Area 1 data update (RT-safe)
// ───────────────────────────────────────────────
void RtTui::_set_row_data(int group_idx, int row_idx,
                          const char* label, const char* cols[], int ncols) {
    if (group_idx < 0 || group_idx >= (int)TUI_MAX_GROUPS)
        return;

    if (row_idx < 0 || row_idx >= (int)TUI_MAX_ROWS_PER_GROUP)
        return;

    if (ncols > TUI_MAX_COLS)
        ncols = TUI_MAX_COLS;

    int widx = m_data_write_idx.load(std::memory_order_relaxed);
    TuiDataBuffer&    dbuf  = m_data_buf[widx];
    TuiGroupRowData&  gdata = dbuf.groups[group_idx];
    TuiDataRow&       row   = gdata.rows[row_idx];

    strncpy(row.label, label, TUI_DATA_COL_LEN - 1);
    row.label[TUI_DATA_COL_LEN - 1] = '\0';

    row.ncols = ncols;
    for (int i = 0; i < ncols; ++i) {
        if (cols[i]) {
            strncpy(row.col[i], cols[i], TUI_DATA_COL_LEN - 1);
            row.col[i][TUI_DATA_COL_LEN - 1] = '\0';
        }
        else {
            row.col[i][0] = '\0';
        }
    }
    for (int i = ncols; i < TUI_MAX_COLS; ++i)
        row.col[i][0] = '\0';

    gdata.nrows = std::max(gdata.nrows, row_idx + 1);
    dbuf.dirty.store(true, std::memory_order_release);
}

void RtTui::set_text_row(int group_idx, int row_idx, const char* label, const char* text) {
    if (group_idx < 0 || group_idx >= (int)TUI_MAX_GROUPS)
        return;
    if (row_idx < 0 || row_idx >= (int)TUI_MAX_ROWS_PER_GROUP)
        return;

    int widx = m_data_write_idx.load(std::memory_order_relaxed);
    TuiDataBuffer&   dbuf  = m_data_buf[widx];
    TuiGroupRowData& gdata = dbuf.groups[group_idx];
    TuiDataRow&      row   = gdata.rows[row_idx];

    strncpy(row.label, label, TUI_DATA_COL_LEN - 1);
    row.label[TUI_DATA_COL_LEN - 1] = '\0';

    row.text_mode = true;
    strncpy(row.text, text, TUI_TEXT_ROW_LEN - 1);
    row.text[TUI_TEXT_ROW_LEN - 1] = '\0';
    row.ncols = 0;

    gdata.nrows = std::max(gdata.nrows, row_idx + 1);
    dbuf.dirty.store(true, std::memory_order_release);
}

int RtTui::calc_area1_height() const noexcept {
    int widx = m_data_write_idx.load(std::memory_order_relaxed);
    int ridx = 1 - widx;
    const TuiDataBuffer& buf = m_data_buf[ridx];

    int content = 0;
    bool first = true;
    for (int g = 0; g < (int)TUI_MAX_GROUPS; ++g) {
        if (!m_group_headers[g].active)
            continue;

        if (!first)
            content += 1;  // group separator line

        first = false;
        content += 2;  // header row + underline separator
        content += buf.groups[g].nrows;
    }
    return 2 + content;  // +2 for top and bottom border
}

// ───────────────────────────────────────────────
// Area 2 log API (RT-safe, uses MpscLogQueue)
// ───────────────────────────────────────────────
void RtTui::log(spdlog::level::level_enum level, const char* fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    log_v(level, fmt, ap);
    va_end(ap);
}

void RtTui::log_v(spdlog::level::level_enum level, const char* fmt, va_list args) {
    TuiLogEntry entry;
    entry.set_v(level, 0, fmt, args);  // timestamp_ns = 0 (not used in TUI)
    m_log_queue.try_push(entry);
}

void RtTui::log_trace(const char* fmt, ...) {
    va_list ap; va_start(ap, fmt);
    log_v(spdlog::level::trace, fmt, ap);
    va_end(ap);
}

void RtTui::log_debug(const char* fmt, ...) {
    va_list ap; va_start(ap, fmt);
    log_v(spdlog::level::debug, fmt, ap);
    va_end(ap);
}

void RtTui::log_info(const char* fmt, ...) {
    va_list ap; va_start(ap, fmt);
    log_v(spdlog::level::info, fmt, ap);
    va_end(ap);
}

void RtTui::log_warn(const char* fmt, ...) {
    va_list ap; va_start(ap, fmt);
    log_v(spdlog::level::warn, fmt, ap);
    va_end(ap);
}

void RtTui::log_error(const char* fmt, ...) {
    va_list ap; va_start(ap, fmt);
    log_v(spdlog::level::err, fmt, ap);
    va_end(ap);
}

void RtTui::log_critical(const char* fmt, ...) {
    va_list ap; va_start(ap, fmt);
    log_v(spdlog::level::critical, fmt, ap);
    va_end(ap);
}

// ───────────────────────────────────────────────
// tick(): called at 25 Hz by the RtLog drain thread
// ───────────────────────────────────────────────
void RtTui::tick() {
    if (!m_running.load(std::memory_order_acquire)) 
        return;

    // 1) drain log queue
    TuiLogEntry entry;
    while (m_log_queue.try_pop(entry)) {
        if (m_log_count < LOG_KEEP) {
            m_log_history[(m_log_head + m_log_count) % LOG_KEEP] = entry;
            m_log_count++;
        }
        else {
            // Circular buffer: overwrite oldest slot without memmove
            m_log_history[m_log_head] = entry;
            m_log_head = (m_log_head + 1) % LOG_KEEP;
        }

        if (m_auto_scroll) {
            m_scroll_offset = 0;
        }
        else {
            // PAUSED: keep the visible window anchored when new messages arrive
            m_scroll_offset++;
        }
    }

    // 2) process all key input from a single read() call
    char kbuf[64]{};
    ssize_t kr = read(STDIN_FILENO, kbuf, sizeof(kbuf));
    bool key_pressed = (kr > 0);
    if (key_pressed) {
        ssize_t pos = 0;
        while (pos < kr) {
            ssize_t remaining = kr - pos;
            // ESC sequence: at least 3 bytes (ESC [ X)
            if (kbuf[pos] == '\x1b' && remaining >= 3 && kbuf[pos + 1] == '[') {
                ssize_t seq_len = (remaining >= 4 && kbuf[pos + 3] == '~') ? 4 : 3;
                handle_key(kbuf + pos, seq_len);
                pos += seq_len;
            }
            else if (kbuf[pos] == '\x1b') {
                handle_key(kbuf + pos, remaining);
                break;  // ESC alone or incomplete sequence
            }
            else {
                handle_key(kbuf + pos, 1);
                pos += 1;
            }
        }
    }

    // 3) update terminal size and compute layout
    term_get_size(m_term_rows, m_term_cols);
    int rows     = m_term_rows;
    int cols     = m_term_cols;
    int needed   = calc_area1_height();
    int area1_h  = std::max(4, std::min(needed, rows - 1 - AREA2_MIN_ROWS));
    int area2_h  = rows - area1_h - 1;  // bottom row reserved for command status line

    // 4) clear screen on terminal resize
    if (rows != m_prev_term_rows || cols != m_prev_term_cols) {
        buf_append_str(ansi::CLEAR_SCREEN);
        flush_output();
        m_prev_term_rows = rows;
        m_prev_term_cols = cols;
    }

    // 5) render: pack Area1 + Area2 into one buffer for a single write()
    //    prevents split-frame artifacts (partial flush → ghost images / red background bleed)
    // Flush bytes retained from the previous EAGAIN before overwriting the buffer
    flush_output();
    m_out_pos = 0;  // discard any still-retained bytes; re-render a fresh frame
    render_area1(1, area1_h, cols);
    render_area2(area1_h + 1, area2_h, cols);
    render_cmd_line(rows, cols);

    flush_output();
}

// ───────────────────────────────────────────────
// Area 1: multi-group data monitor
// ───────────────────────────────────────────────
void RtTui::render_area1(int start_row, int height, int width) {
    if (height < 4)
        return;

    // Fix write_idx with a single load to prevent TOCTOU races
    int widx = m_data_write_idx.load(std::memory_order_acquire);
    int ridx = 1 - widx;

    if (m_data_buf[widx].dirty.load(std::memory_order_acquire)) {
        m_data_buf[widx].dirty.store(false, std::memory_order_release);
        m_data_write_idx.store(1 - widx, std::memory_order_release);
        ridx = widx;
    }

    const TuiDataBuffer& buf = m_data_buf[ridx];

    // Column width: label 21 chars (1+20) + 2 border chars + 12 per column (1 space + 11)
    int max_cols_fit = (width - 23) / 12;
    if (max_cols_fit > TUI_MAX_COLS)
        max_cols_fit = TUI_MAX_COLS;

    if (max_cols_fit < 0)
        max_cols_fit = 0;

    int cur = start_row;
    const int bot = start_row + height - 1;  // reserved for bottom border

    // ── top border ──
    m_out_pos += snprintf(m_out_buf + m_out_pos, sizeof(m_out_buf) - m_out_pos,
        "\x1b[%d;1H%s%s┌", cur++, ansi::BOLD, ansi::FG_CYAN);
    buf_append_hbar(width - 2);
    buf_append_str("┐");
    buf_append_str(ansi::RESET);

    bool first_group = true;
    for (int g = 0; g < (int)TUI_MAX_GROUPS && cur < bot; ++g) {
        const TuiGroupHeader& gh = m_group_headers[g];
        if (!gh.active)
            continue;

        // ── group separator (skipped for first group) ──
        if (!first_group && cur < bot) {
            m_out_pos += snprintf(m_out_buf + m_out_pos, sizeof(m_out_buf) - m_out_pos,
                "\x1b[%d;1H%s├", cur++, ansi::FG_CYAN);
            buf_append_hbar(width - 2);
            buf_append_str("┤");
            buf_append_str(ansi::RESET);
        }
        
        first_group = false;

        // ── group header ──
        if (cur < bot) {
            m_out_pos += snprintf(m_out_buf + m_out_pos, sizeof(m_out_buf) - m_out_pos,
                "\x1b[%d;1H%s│%s %s%-20s%s",
                cur, ansi::FG_CYAN, ansi::BOLD, ansi::FG_YELLOW, gh.label, ansi::FG_YELLOW);

            for (int c = 0; c < max_cols_fit; ++c) {
                char auto_hdr[16];
                const char* hdr;
                if (c < gh.ncols) {
                    hdr = gh.cols[c];
                }
                else {
                    // snprintf(auto_hdr, sizeof(auto_hdr), "COL-%d", c);
                    snprintf(auto_hdr, sizeof(auto_hdr), ".");
                    hdr = auto_hdr;
                }
                m_out_pos += snprintf(m_out_buf + m_out_pos, sizeof(m_out_buf) - m_out_pos,
                    " %11s", hdr);
            }

            buf_append_str(ansi::RESET);
            buf_append_str("\x1b[K");
            m_out_pos += snprintf(m_out_buf + m_out_pos, sizeof(m_out_buf) - m_out_pos,
                "\x1b[%d;%dH%s│%s", cur, width, ansi::FG_CYAN, ansi::RESET);
            cur++;
        }

        // ── header underline ──
        if (cur < bot) {
            m_out_pos += snprintf(m_out_buf + m_out_pos, sizeof(m_out_buf) - m_out_pos,
                "\x1b[%d;1H%s├", cur++, ansi::FG_CYAN);
            buf_append_hbar(width - 2);
            buf_append_str("┤");
            buf_append_str(ansi::RESET);
        }

        // ── data rows ──
        const TuiGroupRowData& gdata = buf.groups[g];
        for (int r = 0; r < gdata.nrows && cur < bot; ++r) {
            const TuiDataRow& dr = gdata.rows[r];
            m_out_pos += snprintf(m_out_buf + m_out_pos, sizeof(m_out_buf) - m_out_pos,
                "\x1b[%d;1H%s│%s", cur, ansi::FG_CYAN, ansi::RESET);
            m_out_pos += snprintf(m_out_buf + m_out_pos, sizeof(m_out_buf) - m_out_pos,
                " %s%-20s%s", ansi::FG_WHITE, dr.label, ansi::RESET);

            if (dr.text_mode) {
                // Full-width text: 1(│)+1(sp)+20(label)+1(sp)+text+1(│) = 24 + text + 1
                int text_avail = width - 24;
                if (text_avail > 0) {
                    m_out_pos += snprintf(m_out_buf + m_out_pos, sizeof(m_out_buf) - m_out_pos,
                        " %s%-*.*s%s", ansi::FG_WHITE, text_avail, text_avail, dr.text, ansi::RESET);
                }
            }
            else {
                for (int ci = 0; ci < max_cols_fit; ++ci) {
                    if (ci < dr.ncols) {
                        m_out_pos += snprintf(m_out_buf + m_out_pos, sizeof(m_out_buf) - m_out_pos,
                            " %s%11s%s", ansi::FG_WHITE, dr.col[ci], ansi::RESET);
                    }
                    else {
                        m_out_pos += snprintf(m_out_buf + m_out_pos, sizeof(m_out_buf) - m_out_pos,
                            "            ");
                    }
                }
            }

            buf_append_str("\x1b[K");
            m_out_pos += snprintf(m_out_buf + m_out_pos, sizeof(m_out_buf) - m_out_pos,
                "\x1b[%d;%dH%s│%s", cur, width, ansi::FG_CYAN, ansi::RESET);
            cur++;
        }
    }

    // ── clear empty rows between data and bottom border ──
    while (cur < bot) {
        m_out_pos += snprintf(m_out_buf + m_out_pos, sizeof(m_out_buf) - m_out_pos,
            "\x1b[%d;1H%s│%s\x1b[K\x1b[%d;%dH%s│%s",
            cur, ansi::FG_CYAN, ansi::RESET,
            cur, width, ansi::FG_CYAN, ansi::RESET);
        cur++;
    }

    // ── bottom border ──
    m_out_pos += snprintf(m_out_buf + m_out_pos, sizeof(m_out_buf) - m_out_pos,
        "\x1b[%d;1H%s└", bot, ansi::FG_CYAN);
    buf_append_hbar(width - 2);
    buf_append_str("┘");
    buf_append_str(ansi::RESET);
}

// ───────────────────────────────────────────────
// Area 2: log scroll view
// ───────────────────────────────────────────────
void RtTui::render_area2(int start_row, int height, int width) {
    if (height < 3) 
        return;

    size_t visible_h  = (size_t)(height - 2);  // excluding borders
    size_t total      = m_log_count;
    size_t offset     = m_scroll_offset;

    // Count display columns for UTF-8 string (skip multi-byte continuation bytes 0x80–0xBF)
    auto utf8_cols = [](const char* s) -> int {
        int n = 0;
        while (*s) { if ((*s & 0xC0) != 0x80) ++n; ++s; }
        return n;
    };

    // Compute visible range (anchored to bottom)
    size_t end_idx  = (offset < total) ? (total - offset) : 0;
    size_t start_idx = (end_idx > visible_h) ? (end_idx - visible_h) : 0;
    size_t lines_to_show = end_idx - start_idx;

    // ── top border ──
    const char* title_color = m_auto_scroll ? ansi::FG_CYAN : ansi::FG_YELLOW;
    const char* title_text  = m_auto_scroll
        ? "─[ LOG : AUTO SCROLL ]─"
        : "─[ LOG : PAUSED      ]─";

    // ┌ + title (title_color) + remaining ─ (FG_CYAN) + ┐
    m_out_pos += snprintf(m_out_buf + m_out_pos, sizeof(m_out_buf) - m_out_pos,
        "\x1b[%d;1H%s%s┌%s%s%s",
        start_row, ansi::BOLD, ansi::FG_CYAN,
        title_color, title_text, ansi::FG_CYAN);
    int remain = width - 2 - utf8_cols(title_text);

    buf_append_hbar(remain);

    // ┐ placed at absolute column
    m_out_pos += snprintf(m_out_buf + m_out_pos, sizeof(m_out_buf) - m_out_pos,
        "\x1b[%d;%dH┐%s", start_row, width, ansi::RESET);

    // ── log lines ──
    int scrollbar_col = width;  // rightmost column
    for (size_t r = 0; r < visible_h; ++r) {
        int screen_row = start_row + 1 + (int)r;
        m_out_pos += snprintf(m_out_buf + m_out_pos, sizeof(m_out_buf) - m_out_pos,
            "\x1b[%d;1H%s│%s", screen_row, ansi::FG_CYAN, ansi::RESET);

        if (r < lines_to_show) {
            const TuiLogEntry& e = m_log_history[(m_log_head + start_idx + r) % LOG_KEEP];
            int max_msg = width - 4;
            m_out_pos += snprintf(m_out_buf + m_out_pos, sizeof(m_out_buf) - m_out_pos,
                " %.*s", max_msg, e.msg);
        }

        // RESET first: \x1b[K after a critical/background-color message would erase
        // with the background still active, corrupting the scrollbar area
        buf_append_str(ansi::RESET);
        buf_append_str("\x1b[K");
        m_out_pos += snprintf(m_out_buf + m_out_pos, sizeof(m_out_buf) - m_out_pos,
            "\x1b[%d;%dH%s│%s", screen_row, scrollbar_col, ansi::FG_CYAN, ansi::RESET);
    }

    // ── scrollbar (overlaid on second-to-last column) ──
    render_scrollbar(start_row + 1, (int)visible_h, width - 1,
                     total, visible_h, offset);

    // ── bottom border ──
    m_out_pos += snprintf(m_out_buf + m_out_pos, sizeof(m_out_buf) - m_out_pos,
        "\x1b[%d;1H%s└", start_row + height - 1, ansi::FG_CYAN);

    buf_append_hbar(width - 2);

    // bottom key hint
    const char* hint = " ↑↓:Line  PgUp/PgDn:Page ";
    int hint_cols = utf8_cols(hint);

    // hint sits just left of ┘; ┘ is placed at absolute column width
    int hint_col = std::max(2, width - 1 - hint_cols);
    m_out_pos += snprintf(m_out_buf + m_out_pos, sizeof(m_out_buf) - m_out_pos,
        "\x1b[%d;%dH%s%s", start_row + height - 1, hint_col, ansi::FG_GRAY, hint);
    m_out_pos += snprintf(m_out_buf + m_out_pos, sizeof(m_out_buf) - m_out_pos,
        "\x1b[%d;%dH%s─┘%s", start_row + height - 1, width - 1, ansi::FG_CYAN, ansi::RESET);
}

// ───────────────────────────────────────────────
// Scrollbar rendering
// ───────────────────────────────────────────────
void RtTui::render_scrollbar(int start_row, int height, int col,
                              size_t total, size_t visible, size_t offset) {
    if (total <= visible) 
        return;  // no scrollbar needed

    // thumb position (0 = bottom, height-1 = top)
    size_t scrollable = total - visible;
    size_t clamped    = (offset < scrollable) ? offset : scrollable;
    int thumb_from_bottom = (int)((double)clamped / scrollable * (height - 1));
    int thumb_row = height - 1 - thumb_from_bottom;  // screen row (0-based)

    for (int r = 0; r < height; ++r) {
        const char* sym = (r == thumb_row) ? "█" : " ";
        m_out_pos += snprintf(m_out_buf + m_out_pos, sizeof(m_out_buf) - m_out_pos,
            "\x1b[%d;%dH%s%s%s",
            start_row + r, col,
            ansi::FG_GRAY, sym, ansi::RESET);
    }
}

// ───────────────────────────────────────────────
// Key input handler
// ───────────────────────────────────────────────
void RtTui::handle_key(const char* buf, ssize_t len) {
    if (len <= 0) 
        return;

    size_t page = (size_t)(m_term_rows * 6 / 10);
    if (page < 1) 
        page = 1;

    char c = buf[0];

    // ESC sequence: tick() already read the full sequence, so buf[1..] can be accessed directly
    if (c == '\x1b' && len >= 3 && buf[1] == '[') {
        char b2 = buf[2];
        switch (b2) {
        case 'A':  // ↑ arrow
            m_scroll_offset++;
            m_auto_scroll = false;
            break;
        case 'B':  // ↓ arrow
            if (m_scroll_offset > 0) m_scroll_offset--;
            if (m_scroll_offset == 0) m_auto_scroll = true;
            break;
        case '5':  // PgUp: ESC[5~
            if (len >= 4 && buf[3] == '~') {
                m_scroll_offset += page;
                m_auto_scroll = false;
            }
            break;
        case '6':  // PgDn: ESC[6~
            if (len >= 4 && buf[3] == '~') {
                if (m_scroll_offset > page) 
                    m_scroll_offset -= page;
                else
                    m_scroll_offset = 0;

                if (m_scroll_offset == 0)
                    m_auto_scroll = true;
            }
            break;
        case 'F':  // End
            m_scroll_offset = 0;
            m_auto_scroll   = true;
            break;
        case 'H':  // Home
            m_scroll_offset = m_log_count;
            m_auto_scroll   = false;
            break;
        }

        // clamp scroll_offset to upper bound
        if (m_scroll_offset > m_log_count)
            m_scroll_offset = m_log_count;
        return;
    }

    // single-char key: relay to main loop for non-blocking handling
    m_last_key.store(c, std::memory_order_relaxed);
    // static_cast<SysData*>(m_sysData)->tuiPendingKey.store(c, std::memory_order_relaxed);
}

// ───────────────────────────────────────────────
// Command status line (bottom row)
// ───────────────────────────────────────────────
void RtTui::render_cmd_line(int row, int width) {
    char key_str[16];
    char _key = m_last_key.load(std::memory_order_relaxed);

    if (_key == 0) {
        snprintf(key_str, sizeof(key_str), "(none)");
    }
    else if (_key >= 0x20 && _key < 0x7f) {
        snprintf(key_str, sizeof(key_str), "'%c'", _key);
    }
    else {
        snprintf(key_str, sizeof(key_str), "0x%02X", (unsigned char)_key);
    }

    m_out_pos += snprintf(m_out_buf + m_out_pos, sizeof(m_out_buf) - m_out_pos,
        "\x1b[%d;1H%s CMD: %s%-10s%s\x1b[K",
        row, ansi::FG_GRAY, ansi::FG_WHITE, key_str, ansi::RESET);
}

// ───────────────────────────────────────────────
// Output buffer helpers
// ───────────────────────────────────────────────
void RtTui::buf_append(const char* s, size_t len) {
    if (m_out_pos + len >= OUT_BUF_SIZE) 
        flush_output();
    
    memcpy(m_out_buf + m_out_pos, s, len);
    m_out_pos += len;
}

void RtTui::buf_append_str(const char* s) {
    buf_append(s, strlen(s));
}

void RtTui::buf_append_hbar(int n) {
    // ─ (U+2500) = UTF-8 3 bytes: E2 94 80
    // Fixed 3-byte copy instead of strlen to minimize loop overhead
    static constexpr char HBAR[3] = {'\xe2', '\x94', '\x80'};
    for (int i = 0; i < n; ++i)
        buf_append(HBAR, 3);
}

void RtTui::flush_output() {
    if (m_out_pos == 0)
        return;

    // snprintf "would-have-written" accumulation may push m_out_pos past OUT_BUF_SIZE
    if (m_out_pos > OUT_BUF_SIZE)
        m_out_pos = OUT_BUF_SIZE;

    // O_NONBLOCK: write() returns EAGAIN when pty buffer is full (e.g. terminal maximize/resize).
    // On EAGAIN, poll(POLLOUT) waits until the pty drains (event-driven, not a fixed sleep).
    // Max wait: 3 ms per EAGAIN event — enough for the terminal emulator to redraw and drain,
    // without blocking the drain thread longer than necessary.
    // If still full after 3 ms, retain unwritten bytes for the next tick.
    size_t written = 0;
    while (written < m_out_pos) {
        ssize_t n = write(STDOUT_FILENO, m_out_buf + written, m_out_pos - written);
        if (n < 0) {
            if (errno == EINTR)
                continue;

            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                struct pollfd pfd = {STDOUT_FILENO, POLLOUT, 0};
                if (poll(&pfd, 1, 3) > 0 && (pfd.revents & POLLOUT))
                    continue;  // pty drained: retry write immediately
            }

            LOG_RT_RAW(err, "[ERROR] poll timeout or error: retain remaining bytes(%d)", m_out_pos - written);
            break;  // poll timeout or error: retain remaining bytes
        }

        if (n == 0)
            break;

        written += (size_t)n;
    }

    // Slide unwritten bytes to buffer front; next tick() flushes them before rendering
    size_t remaining = m_out_pos - written;
    if (remaining > 0 && written > 0)
        std::memmove(m_out_buf, m_out_buf + written, remaining);

    m_out_pos = remaining;
}
