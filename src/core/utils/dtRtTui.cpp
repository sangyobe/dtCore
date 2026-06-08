#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <sys/ioctl.h>
#include <poll.h>
#include <string.h>
#include <stdio.h>
#include <atomic>
#include <algorithm>

#include "dtCore/src/dtLog/dtRtTui.hpp"
#include "dtCore/src/dtLog/dtRtLog.hpp"

namespace dt {
namespace Utils {

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
    static constexpr const char* FG_MAGENTA = "\x1b[35m";
    static constexpr const char* CLEAR_SCREEN = "\x1b[2J";
    static constexpr const char* HIDE_CURSOR  = "\x1b[?25l";
    static constexpr const char* SHOW_CURSOR  = "\x1b[?25h";
}

// ═══════════════════════════════════════════════
// RtTui implementation
// ═══════════════════════════════════════════════
RtTui::RtTui() {
    memset(m_out_buf, 0, sizeof(m_out_buf));
    m_old_termios = new struct termios();

    // Assign default layout names
    for (int i = 0; i < MAX_LAYOUTS; ++i) {
        snprintf(m_layouts[i].name, sizeof(m_layouts[i].name), "Layout %d", i + 1);
    }
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
    int out_flags = fcntl(STDOUT_FILENO, F_GETFL, 0);
    if (out_flags != -1)
        fcntl(STDOUT_FILENO, F_SETFL, out_flags | O_NONBLOCK);

    buf_append_str(ansi::HIDE_CURSOR);
    buf_append_str(ansi::CLEAR_SCREEN);
    flush_output();
}

void RtTui::term_restore() {
    if (!m_term_active.exchange(false, std::memory_order_acq_rel))
        return;

    int out_flags = fcntl(STDOUT_FILENO, F_GETFL, 0);
    if (out_flags != -1)
        fcntl(STDOUT_FILENO, F_SETFL, out_flags & ~O_NONBLOCK);

    m_out_pos = 0;
    buf_append_str("\x1b[0m");
    buf_append_str(ansi::SHOW_CURSOR);
    buf_append_str(ansi::CLEAR_SCREEN);
    buf_append_str("\x1b[1;1H");
    flush_output();

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
// Layout management
// ───────────────────────────────────────────────
void RtTui::set_layout_name(int layout_idx, const char* name) {
    if (layout_idx < 0 || layout_idx >= MAX_LAYOUTS || !name)
        return;

    strncpy(m_layouts[layout_idx].name, name, sizeof(m_layouts[layout_idx].name) - 1);
    m_layouts[layout_idx].name[sizeof(m_layouts[layout_idx].name) - 1] = '\0';
    m_layouts[layout_idx].defined = true;
}

// ───────────────────────────────────────────────
// Area 1 group header setup (once at startup)
// ───────────────────────────────────────────────
void RtTui::set_group(int layout_idx, int group_idx, const char* label_hdr,
                      const char* col_hdrs[], int ncols) {
    if (layout_idx < 0 || layout_idx >= MAX_LAYOUTS)
        return;

    if (group_idx < 0 || group_idx >= (int)TUI_MAX_GROUPS)
        return;

    TuiGroupHeader& gh = m_layouts[layout_idx].group_headers[group_idx];

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
    m_layouts[layout_idx].defined = true;
}

void RtTui::set_group_no_header(int layout_idx, int group_idx) {
    if (layout_idx < 0 || layout_idx >= MAX_LAYOUTS)
        return;

    if (group_idx < 0 || group_idx >= (int)TUI_MAX_GROUPS)
        return;

    TuiGroupHeader& gh = m_layouts[layout_idx].group_headers[group_idx];
    gh.label[0]   = '\0';
    gh.ncols      = 0;
    gh.hide_header = true;
    gh.active      = true;
    m_layouts[layout_idx].defined = true;
}

// ───────────────────────────────────────────────
// Area 1 data update (RT-safe)
// ───────────────────────────────────────────────
void RtTui::_set_row_data(int layout_idx, int group_idx, int row_idx,
                           const char* label, const char* cols[], int ncols) {
    if (layout_idx < 0 || layout_idx >= MAX_LAYOUTS)
        return;

    if (group_idx < 0 || group_idx >= (int)TUI_MAX_GROUPS)
        return;

    if (row_idx < 0 || row_idx >= (int)TUI_MAX_ROWS_PER_GROUP)
        return;

    if (ncols > TUI_MAX_COLS)
        ncols = TUI_MAX_COLS;

    TuiLayoutData&   layout = m_layouts[layout_idx];
    int              widx   = layout.data_write_idx.load(std::memory_order_relaxed);
    TuiDataBuffer&   dbuf   = layout.data_buf[widx];
    TuiGroupRowData& gdata  = dbuf.groups[group_idx];
    TuiDataRow&      row    = gdata.rows[row_idx];

    strncpy(row.label, label, TUI_DATA_COL_LEN - 1);
    row.label[TUI_DATA_COL_LEN - 1] = '\0';

    row.ncols     = ncols;
    row.text_mode = false;
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
    layout.defined = true;
}

void RtTui::set_text_row(int layout_idx, int group_idx, int row_idx, const char* label, const char* text) {
    if (layout_idx < 0 || layout_idx >= MAX_LAYOUTS)
        return;

    if (group_idx < 0 || group_idx >= (int)TUI_MAX_GROUPS)
        return;

    if (row_idx < 0 || row_idx >= (int)TUI_MAX_ROWS_PER_GROUP)
        return;

    TuiLayoutData&   layout = m_layouts[layout_idx];
    int              widx   = layout.data_write_idx.load(std::memory_order_relaxed);
    TuiDataBuffer&   dbuf   = layout.data_buf[widx];
    TuiGroupRowData& gdata  = dbuf.groups[group_idx];
    TuiDataRow&      row    = gdata.rows[row_idx];

    strncpy(row.label, label, TUI_DATA_COL_LEN - 1);
    row.label[TUI_DATA_COL_LEN - 1] = '\0';

    row.text_mode = true;
    strncpy(row.text, text, TUI_TEXT_ROW_LEN - 1);
    row.text[TUI_TEXT_ROW_LEN - 1] = '\0';
    row.ncols = 0;

    gdata.nrows = std::max(gdata.nrows, row_idx + 1);
    dbuf.dirty.store(true, std::memory_order_release);
    layout.defined = true;
}

void RtTui::set_text_row_fmt(int layout_idx, int group_idx, int row_idx, const char* label, const char* fmt, ...) noexcept {
    char buf[TUI_TEXT_ROW_LEN];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);
    set_text_row(layout_idx, group_idx, row_idx, label, buf);
}

int RtTui::calc_area1_height() const noexcept {
    int layout_idx = m_current_layout.load(std::memory_order_relaxed);
    const TuiLayoutData& layout = m_layouts[layout_idx];

    // Use the read buffer (opposite of current write index)
    int widx = layout.data_write_idx.load(std::memory_order_relaxed);
    int ridx = 1 - widx;
    const TuiDataBuffer& buf = layout.data_buf[ridx];

    int content = 0;
    bool first = true;
    for (int g = 0; g < (int)TUI_MAX_GROUPS; ++g) {
        if (!layout.group_headers[g].active)
            continue;

        if (!first)
            content += 1;  // group separator line

        first = false;
        if (!layout.group_headers[g].hide_header)
            content += 2;  // header row + underline separator

        content += buf.groups[g].nrows;
    }

    return (2 + content);  // +2 for top and bottom border
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
    entry.set_v(level, 0, fmt, args);
    m_log_queue.try_push(entry);
}

// ───────────────────────────────────────────────
// tick(): called at 25 Hz by the RtLog drain thread
// ───────────────────────────────────────────────
void RtTui::tick() {
    if (!m_running.load(std::memory_order_acquire))
        return;

    // 1) drain log queue into circular history
    TuiLogEntry entry;
    while (m_log_queue.try_pop(entry)) {
        if (m_log_count < LOG_KEEP) {
            m_log_history[(m_log_head + m_log_count) % LOG_KEEP] = entry;
            m_log_count++;
        }
        else {
            m_log_history[m_log_head] = entry;
            m_log_head = (m_log_head + 1) % LOG_KEEP;
        }

        if (m_auto_scroll) {
            m_scroll_offset = 0;
        }
        else {
            m_scroll_offset++;
        }
    }

    // 2) process key input
    char kbuf[64]{};
    ssize_t kr = read(STDIN_FILENO, kbuf, sizeof(kbuf));
    if (kr > 0) {
        ssize_t pos = 0;
        while (pos < kr) {
            ssize_t remaining = kr - pos;
            if (kbuf[pos] == '\x1b' && remaining >= 3 && kbuf[pos + 1] == '[') {
                ssize_t seq_len = (remaining >= 4 && kbuf[pos + 3] == '~') ? 4 : 3;
                handle_key(kbuf + pos, seq_len);
                pos += seq_len;
            }
            else if (kbuf[pos] == '\x1b') {
                handle_key(kbuf + pos, remaining);
                break;
            }
            else {
                handle_key(kbuf + pos, 1);
                pos += 1;
            }
        }
    }

    // 3) update terminal size and compute layout
    term_get_size(m_term_rows, m_term_cols);
    int rows    = m_term_rows;
    int cols    = m_term_cols;
    int needed  = calc_area1_height();
    int area1_h = std::max(4, std::min(needed, rows - 1 - AREA2_MIN_ROWS));
    int area2_h = rows - area1_h - 1;

    // 4) clear screen on terminal resize or layout switch
    bool force_clear = m_layout_changed.exchange(false, std::memory_order_relaxed);
    if (rows != m_prev_term_rows || cols != m_prev_term_cols || force_clear) {
        buf_append_str(ansi::CLEAR_SCREEN);
        flush_output();
        m_prev_term_rows = rows;
        m_prev_term_cols = cols;
    }

    // 5) render: pack Area1 + Area2 + cmd_line into one buffer → single write()
    flush_output();
    m_out_pos = 0;
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

    int layout_idx = m_current_layout.load(std::memory_order_relaxed);
    TuiLayoutData& layout = m_layouts[layout_idx];

    // Swap double buffer if dirty
    int widx = layout.data_write_idx.load(std::memory_order_acquire);
    int ridx = 1 - widx;

    if (layout.data_buf[widx].dirty.load(std::memory_order_acquire)) {
        layout.data_write_idx.store(1 - widx, std::memory_order_release);
        layout.data_buf[widx].dirty.store(false, std::memory_order_release);
        ridx = widx;
    }

    const TuiDataBuffer& buf = layout.data_buf[ridx];

    // Column width: label 21 chars + 2 border + 12 per data column
    int max_cols_fit = (width - 23) / 12;
    if (max_cols_fit > TUI_MAX_COLS)
        max_cols_fit = TUI_MAX_COLS;

    if (max_cols_fit < 0)
        max_cols_fit = 0;

    int cur = start_row;
    const int bot = start_row + height - 1;

    // ── layout name in top border title ──────────────────
    char title[48];
    snprintf(title, sizeof(title), "[ %s ]", layout.name);

    int title_len = 0;
    for (const char* p = title; *p; ++p)
        if ((*p & 0xC0) != 0x80) 
            ++title_len;

    safe_snprintf("\x1b[%d;1H%s%s┌─%s%s%s─",  cur++, ansi::BOLD, ansi::FG_CYAN, ansi::FG_YELLOW, title, ansi::FG_CYAN);
    int remain_top = width - 2 - title_len;
    buf_append_hbar(remain_top > 0 ? remain_top : 0);
    buf_append_str("┐");
    buf_append_str(ansi::RESET);

    bool first_group = true;
    for (int g = 0; g < (int)TUI_MAX_GROUPS && cur < bot; ++g) {
        const TuiGroupHeader& gh = layout.group_headers[g];
        if (!gh.active)
            continue;

        // ── group separator ──
        if (!first_group && cur < bot) {
            safe_snprintf("\x1b[%d;1H%s├", cur++, ansi::FG_CYAN);
            buf_append_hbar(width - 2);
            buf_append_str("┤");
            buf_append_str(ansi::RESET);
        }

        first_group = false;

        // ── group header row + underline (skipped for headerless groups) ──
        if (!gh.hide_header) {
            if (cur < bot) {
                safe_snprintf("\x1b[%d;1H%s│%s %s%-20s%s",
                    cur, ansi::FG_CYAN, ansi::BOLD, ansi::FG_YELLOW, gh.label, ansi::FG_YELLOW);

                for (int c = 0; c < max_cols_fit; ++c) {
                    char auto_hdr[16];
                    const char* hdr;
                    if (c < gh.ncols) {
                        hdr = gh.cols[c];
                    }
                    else {
                        snprintf(auto_hdr, sizeof(auto_hdr), ".");
                        hdr = auto_hdr;
                    }
                    safe_snprintf(" %11s", hdr);
                }

                buf_append_str(ansi::RESET);
                buf_append_str("\x1b[K");
                safe_snprintf("\x1b[%d;%dH%s│%s", cur, width, ansi::FG_CYAN, ansi::RESET);
                cur++;
            }

            if (cur < bot) {
                safe_snprintf("\x1b[%d;1H%s├", cur++, ansi::FG_CYAN);
                buf_append_hbar(width - 2);
                buf_append_str("┤");
                buf_append_str(ansi::RESET);
            }
        }

        // ── data rows ──
        const TuiGroupRowData& gdata = buf.groups[g];
        for (int r = 0; r < gdata.nrows && cur < bot; ++r) {
            const TuiDataRow& dr = gdata.rows[r];
            safe_snprintf("\x1b[%d;1H%s│%s", cur, ansi::FG_CYAN, ansi::RESET);
            safe_snprintf(" %s%-20s%s", ansi::FG_WHITE, dr.label, ansi::RESET);

            if (dr.text_mode) {
                int text_avail = width - 24;
                if (text_avail > 0) {
                    safe_snprintf(" %s%-*.*s%s", ansi::FG_WHITE, text_avail, text_avail, dr.text, ansi::RESET);
                }
            }
            else {
                for (int ci = 0; ci < max_cols_fit; ++ci) {
                    if (ci < dr.ncols) {
                        safe_snprintf(" %s%11s%s", ansi::FG_WHITE, dr.col[ci], ansi::RESET);
                    }
                    else {
                        safe_snprintf("            ");
                    }
                }
            }

            buf_append_str("\x1b[K");
            safe_snprintf("\x1b[%d;%dH%s│%s", cur, width, ansi::FG_CYAN, ansi::RESET);
            cur++;
        }
    }

    // ── clear empty rows between data and bottom border ──
    while (cur < bot) {
        safe_snprintf("\x1b[%d;1H%s│%s\x1b[K\x1b[%d;%dH%s│%s",
            cur, ansi::FG_CYAN, ansi::RESET,
            cur, width, ansi::FG_CYAN, ansi::RESET);
        cur++;
    }

    // ── bottom border ──
    safe_snprintf("\x1b[%d;1H%s└", bot, ansi::FG_CYAN);
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

    size_t visible_h = (size_t)(height - 2);
    size_t total     = m_log_count;

    // Maximum valid offset: oldest available message sits at the top of the viewport.
    // Setting offset beyond this would make end_idx go to 0 and show nothing.
    size_t max_offset = (total > visible_h) ? (total - visible_h) : 0;
    size_t offset     = (m_scroll_offset > max_offset) ? max_offset : m_scroll_offset;

    auto utf8_cols = [](const char* s) -> int {
        int n = 0;
        while (*s) {
            if ((*s & 0xC0) != 0x80)
                ++n;
            ++s;
        }
        return n;
    };

    size_t end_idx       = total - offset;
    size_t start_idx     = (end_idx > visible_h) ? (end_idx - visible_h) : 0;
    size_t lines_to_show = end_idx - start_idx;

    // ── top border ──
    const char* title_color = m_auto_scroll ? ansi::FG_CYAN : ansi::FG_YELLOW;
    const char* title_text  = m_auto_scroll
        ? "─[ LOG : AUTO SCROLL ]─"
        : "─[ LOG : PAUSED      ]─";

    safe_snprintf("\x1b[%d;1H%s%s┌%s%s%s",
        start_row, ansi::BOLD, ansi::FG_CYAN,
        title_color, title_text, ansi::FG_CYAN);
    int remain = width - 2 - utf8_cols(title_text);

    buf_append_hbar(remain);

    safe_snprintf("\x1b[%d;%dH┐%s", start_row, width, ansi::RESET);

    // ── log lines ──
    for (size_t r = 0; r < visible_h; ++r) {
        int screen_row = start_row + 1 + (int)r;
        safe_snprintf("\x1b[%d;1H%s│%s", screen_row, ansi::FG_CYAN, ansi::RESET);

        if (r < lines_to_show) {
            const TuiLogEntry& e = m_log_history[(m_log_head + start_idx + r) % LOG_KEEP];
            int max_msg = width - 4;
            safe_snprintf(" %.*s", max_msg, e.msg);
        }

        buf_append_str(ansi::RESET);
        buf_append_str("\x1b[K");
        safe_snprintf("\x1b[%d;%dH%s│%s", screen_row, width, ansi::FG_CYAN, ansi::RESET);
    }

    // ── scrollbar ── (pass clamped offset so the thumb tracks the actual visible position)
    render_scrollbar(start_row + 1, (int)visible_h, width - 1, total, visible_h, offset);

    // ── bottom border ──
    safe_snprintf("\x1b[%d;1H%s└", start_row + height - 1, ansi::FG_CYAN);
    buf_append_hbar(width - 2);

    const char* hint = " ↑↓:Line  PgUp/PgDn:Page ";
    int hint_cols = utf8_cols(hint);
    int hint_col = std::max(2, width - 1 - hint_cols);
    safe_snprintf("\x1b[%d;%dH%s%s", start_row + height - 1, hint_col, ansi::FG_GRAY, hint);
    safe_snprintf("\x1b[%d;%dH%s─┘%s", start_row + height - 1, width - 1, ansi::FG_CYAN, ansi::RESET);
}

// ───────────────────────────────────────────────
// Scrollbar rendering
// ───────────────────────────────────────────────
void RtTui::render_scrollbar(int start_row, int height, int col,
                              size_t total, size_t visible, size_t offset) {
    if (total <= visible)
        return;

    size_t scrollable = total - visible;
    size_t clamped    = (offset < scrollable) ? offset : scrollable;
    int thumb_from_bottom = (int)((double)clamped / scrollable * (height - 1));
    int thumb_row = height - 1 - thumb_from_bottom;

    for (int r = 0; r < height; ++r) {
        const char* sym = (r == thumb_row) ? "█" : " ";
        safe_snprintf("\x1b[%d;%dH%s%s%s",
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

    // ESC sequence
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

        case '5':  // PgUp
            if (len >= 4 && buf[3] == '~') {
                m_scroll_offset += page;
                m_auto_scroll = false;
            }
            break;

        case '6':  // PgDn
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

        // Coarse upper-bound clamp (fine clamp against visible height is done in render_area2)
        if (m_scroll_offset > m_log_count)
            m_scroll_offset = m_log_count;
        return;
    }

    // Layout switching: keys '1' ~ '9'
    if (c >= '1' && c <= '9') {
        int new_layout = c - '1';  // 0-based
        if (new_layout < MAX_LAYOUTS) {
            int old_layout = m_current_layout.exchange(new_layout, std::memory_order_relaxed);
            if (old_layout != new_layout)
                m_layout_changed.store(true, std::memory_order_relaxed);
        }
        // also relay to main loop (for user-defined key handling)
        m_last_key.store(c, std::memory_order_relaxed);
        return;
    }

    // Single-char key: relay to main loop
    m_last_key.store(c, std::memory_order_relaxed);
}

// ───────────────────────────────────────────────
// Command status line (bottom row)
// ───────────────────────────────────────────────
void RtTui::render_cmd_line(int row, int width) {
    int cur_layout = m_current_layout.load(std::memory_order_relaxed);

    // Count active (defined) layouts to build the switcher hint
    // Format: ▶1:Name  2:Name  3:Name  (▶ marks active)
    char switcher[256]{};
    int  sw_pos = 0;
    for (int i = 0; i < MAX_LAYOUTS && sw_pos < (int)sizeof(switcher) - 20; ++i) {
        if (!m_layouts[i].defined)
            continue;

        // Truncate layout name to 8 chars for compact display
        char short_name[9]{};
        strncpy(short_name, m_layouts[i].name, 8);
        short_name[8] = '\0';

        if (i == cur_layout)
            sw_pos += snprintf(switcher + sw_pos, sizeof(switcher) - sw_pos, "[%d:%s] ", i + 1, short_name);
        else
            sw_pos += snprintf(switcher + sw_pos, sizeof(switcher) - sw_pos, " %d:%-8s ", i + 1, short_name);
    }

    // Last key display
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

    safe_snprintf("\x1b[%d;1H%s LAYOUTS: %s%s%s  CMD: %s%-10s%s\x1b[K",
        row,
        ansi::FG_GRAY,
        ansi::FG_MAGENTA, switcher, ansi::RESET,
        ansi::FG_WHITE, key_str, ansi::RESET);
}

// ───────────────────────────────────────────────
// Output buffer helpers
// ───────────────────────────────────────────────
void RtTui::buf_append(const char* s, size_t len) {
    if (m_out_pos + len >= OUT_BUF_SIZE)
        flush_output();

    size_t n = std::min(len, (OUT_BUF_SIZE - m_out_pos));
    memcpy(m_out_buf + m_out_pos, s, n);
    m_out_pos += n;
}

void RtTui::buf_append_str(const char* s) {
    buf_append(s, strlen(s));
}

void RtTui::buf_append_hbar(int n) {
    static constexpr char HBAR[3] = {'\xe2', '\x94', '\x80'};
    for (int i = 0; i < n; ++i)
        buf_append(HBAR, 3);
}

void RtTui::flush_output() {
    if (m_out_pos == 0)
        return;

    size_t written = 0;
    while (written < m_out_pos) {
        ssize_t n = write(STDOUT_FILENO, m_out_buf + written, m_out_pos - written);
        if (n < 0) {
            if (errno == EINTR)
                continue;

            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                // pty buffer is full (expected: stdout is O_NONBLOCK).
                // Wait up to 10 ms for the terminal to drain before giving up.
                // A timeout is normal backpressure — not an error.
                struct pollfd pfd = {STDOUT_FILENO, POLLOUT, 0};
                if (poll(&pfd, 1, 10) > 0 && (pfd.revents & POLLOUT))
                    continue;
                // Still not writable: retain unwritten bytes, retry next frame.
                break;
            }

            // Real write error (EPIPE, EBADF, …): log once and discard remaining
            // bytes to prevent the error from being logged again every frame.
            LOG(err).printf("[TUI] write failed (%s): dropped %zu bytes", strerror(errno), m_out_pos - written);
            m_out_pos = written;

            return;
        }

        if (n == 0)
            break;

        written += (size_t)n;
    }

    size_t remaining = m_out_pos - written;
    if (remaining > 0 && written > 0)
        std::memmove(m_out_buf, m_out_buf + written, remaining);

    m_out_pos = remaining;
}

}  // namespace Utils
}  // namespace dt
