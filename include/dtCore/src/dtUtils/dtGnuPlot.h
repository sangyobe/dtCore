/*!
\file       dtGnuPlot.h
\brief      GnuPlot, Graphing utility.
\author     Dong-hyun Lee, phenom8305@gmail.com
\date       2021. 07. 06, 2021. 08. 30
\version    1.1.0
\see       gnuplot software must be installed (http://www.gnuplot.info/)
\warning    Do Not delete this comment for document history! This is minimal manners!
*/

#ifndef __DT_UTILS_GNUPLOT_H__
#define __DT_UTILS_GNUPLOT_H__

/** \addtogroup dtUtils
 *
 */

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdarg.h>

#if defined(_WIN32)
//#include <windows.h>
#define GNUPLOT_PCLOSE _pclose
#define GNUPLOT_POPEN  _popen
#define GNUPLOT_FILENO _fileno
#define GNUPLOT_VSPRINTF vsprintf_s
#else
#define GNUPLOT_PCLOSE pclose
#define GNUPLOT_POPEN  popen
#define GNUPLOT_FILENO fileno
#define GNUPLOT_VSPRINTF vsnprintf 
#endif

namespace dt
{
namespace Utils
{
template <typename m_type = float>
class GnuPlot
{
public:
    enum PointType
    {
        PT_DOT = 0,
        PT_PLUS,
        PT_CROSS,
        PT_ASTERISK,
        PT_SQUARE,
        PT_SQUARE_FILL,
        PT_CIRCLE,
        PT_CIRCLE_FILL,
        PT_TRIANGLE_UP,
        PT_TRIANGLE_UP_FILL,
        PT_TRIANGLE_DN,
        PT_TRIANGLE_DN_FILL,
        PT_DIAMOND,
        PT_DIAMOND_FILL,
        PT_PANTAGON,
        PT_PANTAGON_FILL
    };

public:
    GnuPlot(bool persist = true);
    GnuPlot(const char *gnuplotName);
    ~GnuPlot();

    // send a command to gnuplot
    void Cmd(const char *format, ...);

    // plot data
    void SetData(const char *filePath);
    template <uint16_t row, uint16_t col>
    int8_t SetData(const m_type *mat);

    // plot properties
    void SetGrid(bool on = true);
    void SetTitle(const char *name, const int size = 0, const char * font = nullptr);
    void SetXlabel(const char *name, const int size = 0, const char * font = nullptr);
    void SetYlabel(const char *name, const int size = 0, const char * font = nullptr);
    void SetZlabel(const char *name, const int size = 0, const char * font = nullptr);
    void SetXtics(const int majorTic = 0, const int minorTic = -1);
    void SetYtics(const int majorTic = 0, const int minorTic = -1);
    void SetZtics(const int majorTic = 0, const int minorTic = -1);
    void SetXrange(const m_type min, const m_type max);
    void SetYrange(const m_type min, const m_type max);
    void SetZrange(const m_type min, const m_type max);
    void SetMultiPlot(const int row, const int col, const char *title = nullptr, const int fontSz = 0, const char *font = nullptr);

    // generate the plot scripts to send to gnuplot
    void Point(const char *filePath, const int idxX, const int idxY,
        const char *title = nullptr, const char *colorName = nullptr,
        const char pointType = PT_PLUS, const int pointSz = -1);
    int8_t Point(const m_type *x, const m_type *y, const int dataLen,
        const char *title = nullptr, const char *colorName = nullptr,
        const char pointType = PT_PLUS, const int pointSz = -1);
    int8_t Point(const int idxX, const int idxY,
        const char *title = nullptr, const char *colorName = nullptr,
        const char pointType = PT_PLUS, const int pointSz = -1);

    void Line(const char *filePath, const int idxX, const int idxY,
        const char *title = nullptr, const char *colorName = nullptr,
        const int lineWidth = -1);
    int8_t Line(const m_type *x, const m_type *y, const int dataLen,
        const char *title = nullptr, const char *colorName = nullptr,
        const int lineWidth = -1);
    int8_t Line(const int idxX, const int idxY,
        const char *title = nullptr, const char *colorName = nullptr,
        const int lineWidth = -1);

    void LinePoint(const char *filePath, const int idxX, const int idxY,
        const char *title = nullptr, const char *colorName = nullptr,
        const int lineWidth = -1, const char pointType = -1, const int pointSz = -1, const int pointInterval = 0);
    int8_t LinePoint(const m_type *x, const m_type *y, const int dataLen,
        const char *title = nullptr, const char *colorName = nullptr,
        const int lineWidth = -1, const char pointType = -1, const int pointSz = -1, const int pointInterval = 0);
    int8_t LinePoint(const int idxX, const int idxY,
        const char *title = nullptr, const char *colorName = nullptr,
        const int lineWidth = -1, const char pointType = -1, const int pointSz = -1, const int pointInterval = 0);

    void Dash(const char *filePath, const int idxX, const int idxY,
        const char *title = nullptr, const char *pattern = "-",
        const char *colorName = nullptr, const int lineWidth = -1);
    int8_t Dash(const m_type *x, const m_type *y, const int dataLen,
        const char *title = nullptr, const char *pattern = "-",
        const char *colorName = nullptr, const int lineWidth = -1);
    int8_t Dash(const int idxX, const int idxY,
        const char *title = nullptr, const char *pattern = "-",
        const char *colorName = nullptr, const int lineWidth = -1);

    // generate the splot(3D) scripts to send to gnuplot
    void Point(const char *filePath, const int idxX, const int idxY, const int idxZ,
        const char *title = nullptr, const char *colorName = nullptr,
        const char pointType = -1, const int pointSz = -1);
    int8_t Point(const m_type *x, const m_type *y, const m_type *z, const int dataLen,
        const char *title = nullptr, const char *colorName = nullptr,
        const char pointType = -1, const int pointSz = -1);
    int8_t Point(const int idxX, const int idxY, const int idxZ,
        const char *title = nullptr, const char *colorName = nullptr,
        const char pointType = -1, const int pointSz = -1);

    void Line(const char *filePath, const int idxX, const int idxY, const int idxZ,
        const char *title = nullptr, const char *colorName = nullptr,
        const int lineWidth = -1);
    int8_t Line(const m_type *x, const m_type *y, const m_type *z, const int dataLen,
        const char *title = nullptr, const char *colorName = nullptr,
        const int lineWidth = -1);
    int8_t Line(const int idxX, const int idxY, const int idxZ,
        const char *title = nullptr, const char *colorName = nullptr,
        const int lineWidth = -1);

    void LinePoint(const char *filePath, const int idxX, const int idxY, const int idxZ,
        const char *title = nullptr, const char *colorName = nullptr,
        const int lineWidth = -1, const char pointType = -1, const int pointSz = -1, const int pointInterval = 0);
    int8_t LinePoint(const m_type *x, const m_type *y, const m_type *z, const int dataLen,
        const char *title = nullptr, const char *colorName = nullptr,
        const int lineWidth = -1, const char pointType = -1, const int pointSz = -1, const int pointInterval = 0);
    int8_t LinePoint(const int idxX, const int idxY, const int idxZ,
        const char *title = nullptr, const char *colorName = nullptr,
        const int lineWidth = -1, const char pointType = -1, const int pointSz = -1, const int pointInterval = 0);

    void Dash(const char *filePath, const int idxX, const int idxY, const int idxZ,
        const char *title = nullptr, const char *colorName = nullptr,
        const int lineWidth = -1, const char *pattern = nullptr);   
    int8_t Dash(const m_type *x, const m_type *y, const m_type *z, const int dataLen,
        const char *title = nullptr, const char *colorName = nullptr,
        const int lineWidth = -1, const char *pattern = nullptr);
    int8_t Dash(const int idxX, const int idxY, const int idxZ,
        const char *title = nullptr, const char *colorName = nullptr,
        const int lineWidth = -1, const char *pattern = nullptr);

    // send the plot(or splot) script
    void Draw();

private:
    FILE *m_pipe;

    int m_dataNum;
    char m_filePath[64][256];

    bool m_firstPlot = false;
    char m_strCmd[4096];
    int m_strCmdPos;

    int8_t MakeDataFile(const m_type *y, const int dataLen);
    int8_t MakeDataFile(const m_type *x, const m_type *y, const int dataLen);
    int8_t MakeDataFile(const m_type *x, const m_type *y, const m_type *z, const int dataLen);
};

} // namespace Utils
} // namespace dt

#include "dtGnuPlot.tpp"

#endif // __DT_UTILS_GNUPLOT_H__
