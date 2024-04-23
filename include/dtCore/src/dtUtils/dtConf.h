// This file is part of dtCore, a C++ library for robotics software
// development.
//
// This library is commercial and cannot be redistributed, and/or modified
// WITHOUT ANY ALLOWANCE OR PERMISSION OF Hyundai Motor Company.

#ifndef __DTCORE_DTCONF_H__
#define __DTCORE_DTCONF_H__

/** \defgroup dtFile
 *
 */

#include <string>
//#include <cassert>
#include "yaml-cpp/yaml.h"

namespace dtCore {

class dtConf {
public:
    // constructors
    dtConf() = delete;
    dtConf(const std::string& yaml_file);
    dtConf(const YAML::Node& node);

    // indexer
    template<typename Key> const dtConf operator[](const Key& key) const;
    template<typename Key> dtConf operator[](const Key& key);

    // value accessor
    template<typename ValueType> const ValueType to() const;
    const std::string toString() const;
    const double toDouble() const;
    const float toFloat() const;
    const int32_t toInt32() const;
    const uint32_t toUInt32() const;
    const bool toBoolean() const;
#ifdef SYSREAL
    const SYSREAL toReal() const;
#endif

    // get array size
    size_t size();

private:
    YAML::Node _rootNode;
};

template<typename Key>
const dtConf dtConf::operator[](const Key& key) const
{
    return dtConf(_rootNode[key]);
}

template<typename Key>
dtConf dtConf::operator[](const Key& key)
{
    return dtConf(_rootNode[key]);
}

// value accessor
template<typename ValueType>
const ValueType dtConf::to() const
{
    return _rootNode.as<ValueType>();
}

}

#endif // __DTCORE_DTCONF_H__