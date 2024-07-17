namespace dt
{
namespace Utils
{

template <typename m_type>
inline GnuPlot<m_type>::GnuPlot(bool persist)
{
    m_pipe = GNUPLOT_POPEN(persist ? "gnuplot -persist -slow" : "gnuplot -slow", "w");

    if (!m_pipe) printf("GnuPlot Open - Fail\n");
    else
    {
        fputs("reset\n", m_pipe); // init setting
        fputs("set autoscale\n", m_pipe);
        fputs("set grid x y z vertical\n", m_pipe); // grid on
        fputs("set xyplane at 0\n", m_pipe);
    }
}

template <typename m_type>
inline GnuPlot<m_type>::GnuPlot(const char *gnuplotName)
{
    m_pipe = GNUPLOT_POPEN(gnuplotName, "w");

    if (!m_pipe) printf("GnuPlot Open - Fail\n");
    else
    {
        fputs("reset\n", m_pipe); // init setting
        fputs("set autoscale\n", m_pipe);
        fputs("set grid x y z vertical\n", m_pipe); // grid on
        fputs("set xyplane at 0\n", m_pipe);
    }
}

template <typename m_type>
inline GnuPlot<m_type>::~GnuPlot()
{
    if (m_pipe) GNUPLOT_PCLOSE(m_pipe);
}

template <typename m_type>
inline void GnuPlot<m_type>::Cmd(const char *format, ...)
{
    if (!m_pipe) return;

    va_list ap;
    char strCmd[1024];
    int rtn;

    va_start(ap, format);
    rtn = GNUPLOT_VSPRINTF(strCmd, sizeof(strCmd), format, ap);
    va_end(ap);

    fputs(strCmd, m_pipe);
    fputs("\n", m_pipe);
}

template <typename m_type>
inline void GnuPlot<m_type>::SetData(const char *filePath)
{
    memcpy(m_filePath, filePath, sizeof(m_filePath));
}

template <typename m_type>
template <uint16_t row, uint16_t col>
inline int8_t GnuPlot<m_type>::SetData(const m_type *mat)
{
    // file name
    FILE *fp;

    memset(&m_filePath[63][0], 0, sizeof(char) * 256);
#if defined(_WIN32)
    snprintf(&m_filePath[63][0], 256, "C:\\temp\\%p-%02d.dat", this, 63);
    fopen_s(&fp, &m_filePath[63][0], "w");
#else
    snprintf(&m_filePath[63][0], 256, "\tmp\%p-%02d.dat", this, 63);
    fp = fopen(&m_filePath[m_dataNum][0], "w");
#endif

    if (fp)
    {
        for (int i = 0; i < row; i++)
        {
            for (int j = 0; j < col; j++)
            {
                fprintf(fp, "%f\t", mat[i * col + j]);
            }
            fprintf(fp, "\n");
        }
        fclose(fp);
    }
    else return -1;

    return 0;
}

template <typename m_type>
inline void GnuPlot<m_type>::SetGrid(bool on)
{
    if (on) fputs("set grid x y z vertical\n", m_pipe);
    else fputs("unset grid\n", m_pipe);
}

template <typename m_type>
inline void GnuPlot<m_type>::SetTitle(const char *name, const int size, const char *font)
{
    char strCmd[1024];

    if (font != nullptr && size > 0)
        snprintf(strCmd, sizeof(strCmd), "set title \"%s\" font \"%s,%d\"", name, font, size);
    else if (font == nullptr && size > 0)
        snprintf(strCmd, sizeof(strCmd), "set title \"%s\" font \",%d\"", name, size);
    else if (font != nullptr && size <= 0)
        snprintf(strCmd, sizeof(strCmd), "set title \"%s\" font \"%s,10\"", name, font);
    else
        snprintf(strCmd, sizeof(strCmd), "set title \"%s\"", name);

    fputs(strCmd, m_pipe);
    fputs("\n", m_pipe);
}

template <typename m_type>
inline void GnuPlot<m_type>::SetXlabel(const char *name, const int size, const char *font)
{
    char strCmd[1024];

    if (font != nullptr && size > 0)
        snprintf(strCmd, sizeof(strCmd), "set xlabel \"%s\" font \"%s,%d\"", name, font, size);
    else if (font == nullptr && size > 0)
        snprintf(strCmd, sizeof(strCmd), "set xlabel \"%s\" font \",%d\"", name, size);
    else if (font != nullptr && size <= 0)
        snprintf(strCmd, sizeof(strCmd), "set xlabel \"%s\" font \"%s,10\"", name, font);
    else
        snprintf(strCmd, sizeof(strCmd), "set xlabel \"%s\"", name);

    fputs(strCmd, m_pipe);
    fputs("\n", m_pipe);
}

template <typename m_type>
inline void GnuPlot<m_type>::SetYlabel(const char *name, const int size, const char *font)
{
    char strCmd[1024];

    if (font != nullptr && size > 0)
        snprintf(strCmd, sizeof(strCmd), "set ylabel \"%s\" font \"%s,%d\"", name, font, size);
    else if (font == nullptr && size > 0)
        snprintf(strCmd, sizeof(strCmd), "set ylabel \"%s\" font \",%d\"", name, size);
    else if (font != nullptr && size <= 0)
        snprintf(strCmd, sizeof(strCmd), "set ylabel \"%s\" font \"%s,10\"", name, font);
    else
        snprintf(strCmd, sizeof(strCmd), "set ylabel \"%s\"", name);

    fputs(strCmd, m_pipe);
    fputs("\n", m_pipe);
}

template <typename m_type>
inline void GnuPlot<m_type>::SetZlabel(const char *name, const int size, const char *font)
{
    char strCmd[1024];

    if (font != nullptr && size > 0)
        snprintf(strCmd, sizeof(strCmd), "set zlabel \"%s\" font \"%s,%d\"", name, font, size);
    else if (font == nullptr && size > 0)
        snprintf(strCmd, sizeof(strCmd), "set zlabel \"%s\" font \",%d\"", name, size);
    else if (font != nullptr && size <= 0)
        snprintf(strCmd, sizeof(strCmd), "set zlabel \"%s\" font \"%s,10\"", name, font);
    else
        snprintf(strCmd, sizeof(strCmd), "set zlabel \"%s\"", name);

    fputs(strCmd, m_pipe);
    fputs("\n", m_pipe);
}

template <typename m_type>
inline void GnuPlot<m_type>::SetXtics(const int majorTic, const int minorTic)
{
    char strCmd[64];

    if (majorTic < 0) snprintf(strCmd, sizeof(strCmd), "unset xtics");
    else if (majorTic == 0) snprintf(strCmd, sizeof(strCmd), "set xtics autofreq");
    else snprintf(strCmd, sizeof(strCmd), "set xtics %d", majorTic);

    fputs(strCmd, m_pipe);
    fputs("\n", m_pipe);

    memset(strCmd, 0, sizeof(strCmd));

    if (minorTic < 0) snprintf(strCmd, sizeof(strCmd), "unset mxtics");
    else if (minorTic == 0) snprintf(strCmd, sizeof(strCmd), "set mxtics");
    else snprintf(strCmd, sizeof(strCmd), "set mxtics %d", minorTic);

    fputs(strCmd, m_pipe);
    fputs("\n", m_pipe);
}

template <typename m_type>
inline void GnuPlot<m_type>::SetYtics(const int majorTic, const int minorTic)
{
    char strCmd[64];

    if (majorTic < 0) snprintf(strCmd, sizeof(strCmd), "unset ytics");
    else if (majorTic == 0) snprintf(strCmd, sizeof(strCmd), "set ytics autofreq");
    else snprintf(strCmd, sizeof(strCmd), "set ytics %d", majorTic);

    fputs(strCmd, m_pipe);
    fputs("\n", m_pipe);

    memset(strCmd, 0, sizeof(strCmd));

    if (minorTic < 0) snprintf(strCmd, sizeof(strCmd), "unset mytics");
    else if (minorTic == 0) snprintf(strCmd, sizeof(strCmd), "set mytics");
    else snprintf(strCmd, sizeof(strCmd), "set mytics %d", minorTic);

    fputs(strCmd, m_pipe);
    fputs("\n", m_pipe);
}

template <typename m_type>
inline void GnuPlot<m_type>::SetZtics(const int majorTic, const int minorTic)
{
    char strCmd[64];

    if (majorTic < 0) snprintf(strCmd, sizeof(strCmd), "unset ztics");
    else if (majorTic == 0) snprintf(strCmd, sizeof(strCmd), "set ztics autofreq");
    else snprintf(strCmd, sizeof(strCmd), "set ztics %d", majorTic);

    fputs(strCmd, m_pipe);
    fputs("\n", m_pipe);

    memset(strCmd, 0, sizeof(strCmd));

    if (minorTic < 0) snprintf(strCmd, sizeof(strCmd), "unset mztics");
    else if (minorTic == 0) snprintf(strCmd, sizeof(strCmd), "set mztics");
    else snprintf(strCmd, sizeof(strCmd), "set mztics %d", minorTic);

    fputs(strCmd, m_pipe);
    fputs("\n", m_pipe);
}

template <typename m_type>
inline void GnuPlot<m_type>::SetXrange(const m_type min, const m_type max)
{
    char strCmd[64];
    snprintf(strCmd, sizeof(strCmd), "set xrange [%f:%f]", min, max);
    fputs(strCmd, m_pipe);
    fputs("\n", m_pipe);
}

template <typename m_type>
inline void GnuPlot<m_type>::SetYrange(const m_type min, const m_type max)
{
    char strCmd[64];
    snprintf(strCmd, sizeof(strCmd), "set yrange [%f:%f]", min, max);
    fputs(strCmd, m_pipe);
    fputs("\n", m_pipe);
}

template <typename m_type>
inline void GnuPlot<m_type>::SetZrange(const m_type min, const m_type max)
{
    char strCmd[64];
    snprintf(strCmd, sizeof(strCmd), "set zrange [%f:%f]", min, max);
    fputs(strCmd, m_pipe);
    fputs("\n", m_pipe);
}

template <typename m_type>
inline void GnuPlot<m_type>::SetMultiPlot(const int row, const int col, const char *title, const int fontSz, const char *font)
{
    char strCmd[1024];
    int strCmdPos = 0;

    strCmdPos = snprintf(strCmd, sizeof(strCmd), "set multiplot layout %d,%d", row, col);
    if (title != nullptr) strCmdPos += snprintf(strCmd, sizeof(strCmd), " title \"%s\"", title);

    if (font != nullptr && fontSz > 0) strCmdPos += snprintf(strCmd, sizeof(strCmd), " font \"%s:%d\"", font, fontSz);
    else if (font != nullptr && fontSz <= 0) strCmdPos += snprintf(strCmd, sizeof(strCmd), " font \"%s:\"", font);
    else if (font == nullptr && fontSz > 0) strCmdPos += snprintf(strCmd, sizeof(strCmd), " font \":%d\"", fontSz);

    fputs(strCmd, m_pipe);
    fputs("\n", m_pipe);

    //m_multiPlot = true;
}

template <typename m_type>
inline void GnuPlot<m_type>::Point(const char *filePath, const int idxX, const int idxY, const char *title, const char *colorName, const char pointType, const int pointSz)
{
    if (!m_firstPlot)
    {
        m_firstPlot = true;
        memset(m_strCmd, 0, sizeof(m_strCmd));
        m_strCmdPos = snprintf(m_strCmd, sizeof(m_strCmd), "plot '%s' using %d:%d with points", filePath, idxX, idxY);
    }
    else
    {
        m_strCmdPos += snprintf(m_strCmd + m_strCmdPos, 4096 - m_strCmdPos, ", '%s' using %d:%d with points", filePath, idxX, idxY);
    }

    if (colorName != nullptr) m_strCmdPos += snprintf(m_strCmd + m_strCmdPos, 4096 - m_strCmdPos, " lc rgb \"%s\"", colorName);
    if (pointType >= 0 && pointType < 16) m_strCmdPos += snprintf(m_strCmd + m_strCmdPos, 4096 - m_strCmdPos, " pt %d", pointType);
    else m_strCmdPos += snprintf(m_strCmd + m_strCmdPos, 4096 - m_strCmdPos, " pt \"%c\"", pointType);
    if (pointSz > 0) m_strCmdPos += snprintf(m_strCmd + m_strCmdPos, 4096 - m_strCmdPos, " ps %d", pointSz);
    if (title != nullptr) m_strCmdPos += snprintf(m_strCmd + m_strCmdPos, 4096 - m_strCmdPos, " title \"%s\"", title);
}

template <typename m_type>
inline int8_t GnuPlot<m_type>::Point(const m_type *x, const m_type *y, const int dataLen, const char *title, const char *colorName, const char pointType, const int pointSz)
{
    if (m_dataNum >= 63) return -1;

    if (x != nullptr)
    {
        if (MakeDataFile(x, y, dataLen)) return -1;
        Point(&m_filePath[m_dataNum][0], 1, 2, title, colorName, pointType, pointSz);
    }
    else
    {
        if (MakeDataFile(y, dataLen)) return -1;
        Point(&m_filePath[m_dataNum][0], 0, 1, title, colorName, pointType, pointSz);
    }

    m_dataNum++;

    return 0;
}

template <typename m_type>
inline int8_t GnuPlot<m_type>::Point(const int idxX, const int idxY, const char *title, const char *colorName, const char pointType, const int pointSz)
{
    if (m_filePath[63][0] == 0) return -1;

    Point(&m_filePath[63][0], idxX, idxY, title, colorName, pointType, pointSz);

    return 0;
}

template <typename m_type>
inline void GnuPlot<m_type>::Line(const char *filePath, const int idxX, const int idxY, const char *title, const char *colorName, const int lineWidth)
{
    if (!m_firstPlot)
    {
        m_firstPlot = true;
        memset(m_strCmd, 0, sizeof(m_strCmd));
        m_strCmdPos = snprintf(m_strCmd, sizeof(m_strCmd), "plot '%s' using %d:%d with lines", filePath, idxX, idxY);
    }
    else
    {
        m_strCmdPos += snprintf(m_strCmd + m_strCmdPos, 4096 - m_strCmdPos, ", '%s' using %d:%d with lines", filePath, idxX, idxY);
    }

    if (colorName != nullptr) m_strCmdPos += snprintf(m_strCmd + m_strCmdPos, 4096 - m_strCmdPos, " lc rgb \"%s\"", colorName);
    if (lineWidth > 0) m_strCmdPos += snprintf(m_strCmd + m_strCmdPos, 4096 - m_strCmdPos, " lw %d", lineWidth);
    if (title != nullptr) m_strCmdPos += snprintf(m_strCmd + m_strCmdPos, 4096 - m_strCmdPos, " title \"%s\"", title);
}

template <typename m_type>
inline int8_t GnuPlot<m_type>::Line(const m_type *x, const m_type *y, const int dataLen, const char *title, const char *colorName, const int lineWidth)
{
    if (m_dataNum >= 63) return -1;

    if (x != nullptr)
    {
        if (MakeDataFile(x, y, dataLen)) return -1;
        Line(&m_filePath[m_dataNum][0], 1, 2, title, colorName, lineWidth);
    }
    else
    {
        if (MakeDataFile(y, dataLen)) return -1;
        Line(&m_filePath[m_dataNum][0], 0, 1, title, colorName, lineWidth);
    }

    m_dataNum++;

    return 0;
}

template <typename m_type>
inline int8_t GnuPlot<m_type>::Line(const int idxX, const int idxY, const char *title, const char *colorName, const int lineWidth)
{
    if (m_filePath[63][0] == 0) return -1;

    Line(&m_filePath[63][0], idxX, idxY, title, colorName, lineWidth);

    return 0;
}

template <typename m_type>
inline void GnuPlot<m_type>::LinePoint(const char *filePath, const int idxX, const int idxY, const char *title, const char *colorName, const int lineWidth, const char pointType, const int pointSz, const int pointInterval)
{
    if (!m_firstPlot)
    {
        m_firstPlot = true;
        memset(m_strCmd, 0, sizeof(m_strCmd));
        m_strCmdPos = snprintf(m_strCmd, sizeof(m_strCmd), "plot '%s' using %d:%d with linespoints", filePath, idxX, idxY);
    }
    else
    {
        m_strCmdPos += snprintf(m_strCmd + m_strCmdPos, 4096 - m_strCmdPos, ", '%s' using %d:%d with linespoints", filePath, idxX, idxY);
    }

    if (colorName != nullptr) m_strCmdPos += snprintf(m_strCmd + m_strCmdPos, 4096 - m_strCmdPos, " lc rgb \"%s\"", colorName);
    if (lineWidth > 0)m_strCmdPos += snprintf(m_strCmd + m_strCmdPos, 4096 - m_strCmdPos, " lw %d", lineWidth);
    if (pointType > 0 && pointType < 16) m_strCmdPos += snprintf(m_strCmd + m_strCmdPos, 4096 - m_strCmdPos, " pt %d", pointType);
    else m_strCmdPos += snprintf(m_strCmd + m_strCmdPos, 4096 - m_strCmdPos, " pt \"%c\"", pointType);
    if (pointSz > 0) m_strCmdPos += snprintf(m_strCmd + m_strCmdPos, 4096 - m_strCmdPos, " ps %d", pointSz);
    if (pointInterval) m_strCmdPos += snprintf(m_strCmd + m_strCmdPos, 4096 - m_strCmdPos, " pi %d", pointInterval);
    if (title != nullptr) m_strCmdPos += snprintf(m_strCmd + m_strCmdPos, 4096 - m_strCmdPos, " title \"%s\"", title);
}

template <typename m_type>
inline int8_t GnuPlot<m_type>::LinePoint(const m_type *x, const m_type *y, const int dataLen, const char *title, const char *colorName, const int lineWidth, const char pointType, const int pointSz, const int pointInterval)
{
    if (m_dataNum >= 63) return -1;

    if (x != nullptr)
    {
        if (MakeDataFile(x, y, dataLen)) return -1;
        LinePoint(&m_filePath[m_dataNum][0], 1, 2, title, colorName, lineWidth, pointType, pointSz, pointInterval);
    }
    else
    {
        if (MakeDataFile(y, dataLen)) return -1;
        LinePoint(&m_filePath[m_dataNum][0], 0, 1, title, colorName, lineWidth, pointType, pointSz, pointInterval);
    }

    m_dataNum++;

    return 0;
}

template <typename m_type>
inline int8_t GnuPlot<m_type>::LinePoint(const int idxX, const int idxY, const char *title, const char *colorName, const int lineWidth, const char pointType, const int pointSz, const int pointInterval)
{
    if (m_filePath[63][0] == 0) return -1;

    LinePoint(&m_filePath[63][0], idxX, idxY, title, colorName, lineWidth, pointType, pointSz, pointInterval);

    return 0;
}

template <typename m_type>
inline void GnuPlot<m_type>::Dash(const char *filePath, const int idxX, const int idxY, const char *title, const char *pattern, const char *colorName, const int lineWidth)
{
    if (!m_firstPlot)
    {
        m_firstPlot = true;
        memset(m_strCmd, 0, sizeof(m_strCmd));
        m_strCmdPos = snprintf(m_strCmd, sizeof(m_strCmd), "plot '%s' using %d:%d with lines", filePath, idxX, idxY);
    }
    else
    {
        m_strCmdPos += snprintf(m_strCmd + m_strCmdPos, 4096 - m_strCmdPos, ", '%s' using %d:%d with lines", filePath, idxX, idxY);
    }

    if (colorName != nullptr) m_strCmdPos += snprintf(m_strCmd + m_strCmdPos, 4096 - m_strCmdPos, " lc rgb \"%s\"", colorName);
    if (lineWidth > 0) m_strCmdPos += snprintf(m_strCmd + m_strCmdPos, 4096 - m_strCmdPos, " lw %d", lineWidth);
    if (pattern != nullptr) m_strCmdPos += snprintf(m_strCmd + m_strCmdPos, 4096 - m_strCmdPos, " dt \"%s\"", pattern);
    if (title != nullptr) m_strCmdPos += snprintf(m_strCmd + m_strCmdPos, 4096 - m_strCmdPos, " title \"%s\"", title);
}

template <typename m_type>
inline int8_t GnuPlot<m_type>::Dash(const m_type *x, const m_type *y, const int dataLen, const char *title, const char *pattern, const char *colorName, const int lineWidth)
{
    if (m_dataNum >= 63) return -1;

    if (x != nullptr)
    {
        if (MakeDataFile(x, y, dataLen)) return -1;
        Dash(&m_filePath[m_dataNum][0], 1, 2, title, pattern, colorName, lineWidth);
    }
    else
    {
        if (MakeDataFile(y, dataLen)) return -1;
        Dash(&m_filePath[m_dataNum][0], 0, 1, title, pattern, colorName, lineWidth);
    }

    m_dataNum++;

    return 0;
}

template <typename m_type>
inline int8_t GnuPlot<m_type>::Dash(const int idxX, const int idxY, const char *title, const char *pattern, const char *colorName, const int lineWidth)
{
    if (m_filePath[63][0] == 0) return -1;

    Dash(&m_filePath[63][0], idxX, idxY, title, pattern, colorName, lineWidth);

    return 0;
}

template <typename m_type>
inline void GnuPlot<m_type>::Point(const char *filePath, const int idxX, const int idxY, const int idxZ, const char *title, const char *colorName, const char pointType, const int pointSz)
{
    if (!m_firstPlot)
    {
        m_firstPlot = true;
        memset(m_strCmd, 0, sizeof(m_strCmd));
        m_strCmdPos = snprintf(m_strCmd, sizeof(m_strCmd), "splot '%s' using %d:%d:%d with points", filePath, idxX, idxY, idxZ);
    }
    else
    {
        m_strCmdPos += snprintf(m_strCmd + m_strCmdPos, 4096 - m_strCmdPos, ", '%s' using %d:%d:%d with points", filePath, idxX, idxY, idxZ);
    }

    if (colorName != nullptr) m_strCmdPos += snprintf(m_strCmd + m_strCmdPos, 4096 - m_strCmdPos, " lc rgb \"%s\"", colorName);
    if (pointType > 0 && pointType < 16) m_strCmdPos += snprintf(m_strCmd + m_strCmdPos, 4096 - m_strCmdPos, " pt %d", pointType);
    else m_strCmdPos += snprintf(m_strCmd + m_strCmdPos, 4096 - m_strCmdPos, " pt \"%c\"", pointType);
    if (pointSz > 0) m_strCmdPos += snprintf(m_strCmd + m_strCmdPos, 4096 - m_strCmdPos, " ps %d", pointSz);
    if (title != nullptr) m_strCmdPos += snprintf(m_strCmd + m_strCmdPos, 4096 - m_strCmdPos, " title \"%s\"", title);
}

template <typename m_type>
inline int8_t GnuPlot<m_type>::Point(const m_type *x, const m_type *y, const m_type *z, const int dataLen, const char *title, const char *colorName, const char pointType, const int pointSz)
{
    if (m_dataNum >= 63) return -1;

    if (MakeDataFile(x, y, z, dataLen)) return -1;

    Point(&m_filePath[m_dataNum][0], 1, 2, 3, title, colorName, pointType, pointSz);

    m_dataNum++;

    return 0;
}

template <typename m_type>
inline int8_t GnuPlot<m_type>::Point(const int idxX, const int idxY, const int idxZ, const char *title, const char *colorName, const char pointType, const int pointSz)
{
    if (m_filePath[63][0] == 0) return -1;

    Point(&m_filePath[63][0], idxX, idxY, idxZ, title, colorName, pointType, pointSz);

    return 0;
}

template <typename m_type>
inline void GnuPlot<m_type>::Line(const char *filePath, const int idxX, const int idxY, const int idxZ, const char *title, const char *colorName, const int lineWidth)
{
    if (!m_firstPlot)
    {
        m_firstPlot = true;
        memset(m_strCmd, 0, sizeof(m_strCmd));
        m_strCmdPos = snprintf(m_strCmd, sizeof(m_strCmd), "splot '%s' using %d:%d:%d with lines", filePath, idxX, idxY, idxZ);
    }
    else
    {
        m_strCmdPos += snprintf(m_strCmd + m_strCmdPos, 4096 - m_strCmdPos, ", '%s' using %d:%d:%d with lines", filePath, idxX, idxY, idxZ);
    }

    if (colorName != nullptr) m_strCmdPos += snprintf(m_strCmd + m_strCmdPos, 4096 - m_strCmdPos, " lc rgb \"%s\"", colorName);
    if (lineWidth > 0) m_strCmdPos += snprintf(m_strCmd + m_strCmdPos, 4096 - m_strCmdPos, " lw %d", lineWidth);
    if (title != nullptr) m_strCmdPos += snprintf(m_strCmd + m_strCmdPos, 4096 - m_strCmdPos, " title \"%s\"", title);
}

template <typename m_type>
inline int8_t GnuPlot<m_type>::Line(const m_type *x, const m_type *y, const m_type *z, const int dataLen, const char *title, const char *colorName, const int lineWidth)
{
    if (m_dataNum >= 63) return -1;

    if (MakeDataFile(x, y, z, dataLen)) return -1;

    Line(&m_filePath[m_dataNum][0], 1, 2, 3, title, colorName, lineWidth);

    m_dataNum++;

    return 0;
}

template <typename m_type>
inline int8_t GnuPlot<m_type>::Line(const int idxX, const int idxY, const int idxZ, const char *title, const char *colorName, const int lineWidth)
{
    if (m_filePath[63][0] == 0) return -1;

    Line(&m_filePath[63][0], idxX, idxY, idxZ, title, colorName, lineWidth);

    return 0;
}

template <typename m_type>
inline void GnuPlot<m_type>::LinePoint(const char *filePath, const int idxX, const int idxY, const int idxZ, const char *title, const char *colorName, const int lineWidth, const char pointType, const int pointSz, const int pointInterval)
{
    if (!m_firstPlot)
    {
        m_firstPlot = true;
        memset(m_strCmd, 0, sizeof(m_strCmd));
        m_strCmdPos = snprintf(m_strCmd, sizeof(m_strCmd), "splot '%s' using %d:%d:%d with linespoints", filePath, idxX, idxY, idxZ);
    }
    else
    {
        m_strCmdPos += snprintf(m_strCmd + m_strCmdPos, 4096 - m_strCmdPos, ", '%s' using %d:%d:%d with linespoints", filePath, idxX, idxY, idxZ);
    }

    if (colorName != nullptr) m_strCmdPos += snprintf(m_strCmd + m_strCmdPos, 4096 - m_strCmdPos, " lc rgb \"%s\"", colorName);
    if (lineWidth > 0)m_strCmdPos += snprintf(m_strCmd + m_strCmdPos, 4096 - m_strCmdPos, " lw %d", lineWidth);
    if (pointType > 0 && pointType < 16) m_strCmdPos += snprintf(m_strCmd + m_strCmdPos, 4096 - m_strCmdPos, " pt %d", pointType);
    else m_strCmdPos += snprintf(m_strCmd + m_strCmdPos, 4096 - m_strCmdPos, " pt \"%c\"", pointType);
    if (pointSz > 0) m_strCmdPos += snprintf(m_strCmd + m_strCmdPos, 4096 - m_strCmdPos, " ps %d", pointSz);
    if (pointInterval) m_strCmdPos += snprintf(m_strCmd + m_strCmdPos, 4096 - m_strCmdPos, " pi %d", pointInterval);
    if (title != nullptr) m_strCmdPos += snprintf(m_strCmd + m_strCmdPos, 4096 - m_strCmdPos, " title \"%s\"", title);
}

template <typename m_type>
inline int8_t GnuPlot<m_type>::LinePoint(const m_type *x, const m_type *y, const m_type *z, const int dataLen, const char *title, const char *colorName, const int lineWidth, const char pointType, const int pointSz, const int pointInterval)
{
    if (m_dataNum >= 63) return -1;

    if (MakeDataFile(x, y, z, dataLen)) return -1;

    LinePoint(&m_filePath[m_dataNum][0], 1, 2, 3, title, colorName, lineWidth, pointType, pointSz, pointInterval);

    m_dataNum++;

    return 0;
}

template <typename m_type>
inline int8_t GnuPlot<m_type>::LinePoint(const int idxX, const int idxY, const int idxZ, const char *title, const char *colorName, const int lineWidth, const char pointType, const int pointSz, const int pointInterval)
{
    if (m_filePath[63][0] == 0) return -1;

    LinePoint(&m_filePath[63][0], idxX, idxY, idxZ, title, colorName, lineWidth, pointType, pointSz, pointInterval);

    return 0;
}

template <typename m_type>
inline void GnuPlot<m_type>::Dash(const char *filePath, const int idxX, const int idxY, const int idxZ, const char *title, const char *colorName, const int lineWidth, const char *pattern)
{
    if (!m_firstPlot)
    {
        m_firstPlot = true;
        memset(m_strCmd, 0, sizeof(m_strCmd));
        m_strCmdPos = snprintf(m_strCmd, sizeof(m_strCmd), "splot '%s' using %d:%d:%d with lines", filePath, idxX, idxY, idxZ);
    }
    else
    {
        m_strCmdPos += snprintf(m_strCmd + m_strCmdPos, 4096 - m_strCmdPos, ", '%s' using %d:%d:%d with lines", filePath, idxX, idxY, idxZ);
    }

    if (colorName != nullptr) m_strCmdPos += snprintf(m_strCmd + m_strCmdPos, 4096 - m_strCmdPos, " lc rgb \"%s\"", colorName);
    if (lineWidth > 0) m_strCmdPos += snprintf(m_strCmd + m_strCmdPos, 4096 - m_strCmdPos, " lw %d", lineWidth);
    if (pattern != nullptr) m_strCmdPos += snprintf(m_strCmd + m_strCmdPos, 4096 - m_strCmdPos, " dt \"%s\"", pattern);
    if (title != nullptr) m_strCmdPos += snprintf(m_strCmd + m_strCmdPos, 4096 - m_strCmdPos, " title \"%s\"", title);
}

template <typename m_type>
inline int8_t GnuPlot<m_type>::Dash(const m_type *x, const m_type *y, const m_type *z, const int dataLen, const char *title, const char *colorName, const int lineWidth, const char *pattern)
{
    if (m_dataNum >= 63) return -1;

    if (MakeDataFile(x, y, z, dataLen)) return -1;

    Dash(&m_filePath[m_dataNum][0], 1, 2, 3, title, colorName, lineWidth, pattern);

    m_dataNum++;

    return 0;
}

template <typename m_type>
inline int8_t GnuPlot<m_type>::Dash(const int idxX, const int idxY, const int idxZ, const char *title, const char *colorName, const int lineWidth, const char *pattern)
{
    if (m_filePath[63][0] == 0) return -1;

    Dash(&m_filePath[63][0], idxX, idxY, idxZ, title, colorName, lineWidth, pattern);

    return 0;
}

template <typename m_type>
inline void GnuPlot<m_type>::Draw()
{
    fputs("clear\n", m_pipe);

    if (m_strCmd[0] != 0)
    {
        fputs(m_strCmd, m_pipe);
        fputs("\n", m_pipe);
    }
}

template <typename m_type>
inline int8_t GnuPlot<m_type>::MakeDataFile(const m_type *y, const int dataLen)
{
    FILE *fp;

    memset(&m_filePath[m_dataNum][0], 0, sizeof(char) * 256);
#if defined(_WIN32)
    snprintf(&m_filePath[m_dataNum][0], 256, "C:\\temp\\%p-%02d.dat", this, m_dataNum);
    fopen_s(&fp, &m_filePath[m_dataNum][0], "w");
#else
    snprintf(&m_filePath[m_dataNum][0], 256, "\tmp\%p-%02d.dat", this, m_dataNum);
    fp = fopen(&m_filePath[m_dataNum][0], "w");
#endif

    if (fp)
    {
        for (int i = 0; i < dataLen; i++)
        {
            fprintf(fp, "%f\n", y[i]);
        }
        fclose(fp);
    }
    else return -1;

    return 0;
}

template <typename m_type>
inline int8_t GnuPlot<m_type>::MakeDataFile(const m_type *x, const m_type *y, const int dataLen)
{
    FILE *fp;

    memset(&m_filePath[m_dataNum][0], 0, sizeof(char) * 256);
#if defined(_WIN32)
    snprintf(&m_filePath[m_dataNum][0], 256, "C:\\temp\\%p-%02d.dat", this, m_dataNum);
    fopen_s(&fp, &m_filePath[m_dataNum][0], "w");
#else
    snprintf(&m_filePath[m_dataNum][0], 256, "\tmp\%p-%02d.dat", this, m_dataNum);
    fp = fopen(&m_filePath[m_dataNum][0], "w");
#endif

    if (fp)
    {
        for (int i = 0; i < dataLen; i++)
        {
            fprintf(fp, "%f\t%f\n", x[i], y[i]);
        }
        fclose(fp);
    }
    else return -1;

    return 0;
}

template <typename m_type>
inline int8_t GnuPlot<m_type>::MakeDataFile(const m_type *x, const m_type *y, const m_type *z, const int dataLen)
{
    FILE *fp;

    memset(&m_filePath[m_dataNum][0], 0, sizeof(char) * 256);
#if defined(_WIN32)
    snprintf(&m_filePath[m_dataNum][0], 256, "C:\\temp\\%p-%02d.dat", this, m_dataNum);
    fopen_s(&fp, &m_filePath[m_dataNum][0], "w");
#else
    snprintf(&m_filePath[m_dataNum][0], 256, "\tmp\%p-%02d.dat", this, m_dataNum);
    fp = fopen(&m_filePath[m_dataNum][0], "w");
#endif

    if (fp)
    {
        for (int i = 0; i < dataLen; i++)
        {
            fprintf(fp, "%f\t%f\t%f\n", x[i], y[i], z[i]);
        }
        fclose(fp);
    }
    else return -1;

    return 0;
}

} // namespace Utils
} // namespace dt