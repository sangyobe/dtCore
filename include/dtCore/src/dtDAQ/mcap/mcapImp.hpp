// This file is part of dtCore, a C++ library for robotics software
// development.
//
// This library is commercial and cannot be redistributed, and/or modified
// WITHOUT ANY ALLOWANCE OR PERMISSION OF Hyundai Motor Company.
//
//
// file: mcapImp.hpp
// 
// IMPORTANT! 
//
// You must include this header file exactly once 
// in your application source files to link the mcap library.
//
#define MCAP_COMPRESSION_NO_LZ4
#define MCAP_COMPRESSION_NO_ZSTD
#define MCAP_IMPLEMENTATION
#include <mcap/mcap.hpp>