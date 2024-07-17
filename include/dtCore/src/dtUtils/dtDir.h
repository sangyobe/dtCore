/*!
 \file      dtDir.h
 \brief     Handle the path to the directory
 \author    Dong-hyun Lee, lee.dh@hyundai.com
 \author    Joonhee Jo, joonhee.jo@hyundai.com
 \date      2023. 09. 13
 \version   1.0.0
 \copyright RoboticsLab ART All rights reserved.
*/

#ifndef __DT_UTILS_DIR_H__
#define __DT_UTILS_DIR_H__

//* C/C++ System Headers -----------------------------------------------------*/
#include <string>

//* Other Lib Headers --------------------------------------------------------*/
//* Project Headers ----------------------------------------------------------*/
//* System-Specific Headers --------------------------------------------------*/

namespace dt
{
namespace Utils
{

// returns the path to the directory containing the current executable
std::string GetExecutableDir();
// returns the directory where tasks are stored
std::string GetTasksDir();
// returns path to a model XML file given path relative to models dir
std::string GetModelPath(std::string path);
// return file extension for the given file
std::string GetFileExtension(const std::string &filePath);
// returns the current user's home directory
std::string GetUserHomeDir();

} // namespace Utils
} // namespace dt

#endif // __DT_UTILS_DIR_H__