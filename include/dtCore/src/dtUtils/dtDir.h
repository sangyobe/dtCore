/*!
 \file      dtDir.h
 \brief     Handle the path to the directory
 \author    Dong-hyun Lee, lee.dh@hyundai.com
 \author    Joonhee Jo, joonhee.jo@hyundai.com
 \date      2023. 09. 13
 \version   1.0.0
 \copyright RoboticsLab ART All rights reserved.
*/

#ifndef SYSTEM_UTILS_DTDIR_H_
#define SYSTEM_UTILS_DTDIR_H_

//* C/C++ System Headers -----------------------------------------------------*/
#include <string>

//* Other Lib Headers --------------------------------------------------------*/
//* Project Headers ----------------------------------------------------------*/
//* System-Specific Headers --------------------------------------------------*/

//* Public(Exported) Macro ---------------------------------------------------*/
//* Public(Exported) Types ---------------------------------------------------*/
//* Public(Exported) Variables -----------------------------------------------*/
//* Public(Exported) Functions -----------------------------------------------*/
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
#endif // SYSTEM_UTILS_DTDIR_H_