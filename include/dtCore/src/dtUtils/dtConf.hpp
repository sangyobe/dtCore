// This file is part of dtCore, a C++ library for robotics software
// development.
//
// This library is commercial and cannot be redistributed, and/or modified
// WITHOUT ANY ALLOWANCE OR PERMISSION OF Hyundai Motor Company.

#ifndef __DT_UTILS_CONF_H__
#define __DT_UTILS_CONF_H__

/** \defgroup dtUtils
 *
 */

#include <string>
//#include <cassert>
#include "yaml-cpp/yaml.h"

namespace dt
{
namespace Utils
{

class Conf
{
public:
    // constructors
    Conf() = delete;
    Conf(const std::string &yaml_file)
    {
        _rootNode = YAML::LoadFile(yaml_file);
    }
    Conf(const std::istream &yaml_str)
    {
        _rootNode = YAML::Load(const_cast<std::istream &>(yaml_str));
    }
    Conf(const YAML::Node &node)
    {
        _rootNode = node;
    }

    // indexer
    template <typename Key> const Conf operator[](const Key &key) const;
    template <typename Key> Conf operator[](const Key &key);

    // value accessor
    template <typename ValueType> const ValueType to() const;
    const std::string toString() const
    {
        return _rootNode.as<std::string>();
    }
    const double toDouble() const
    {
        return _rootNode.as<double>();
    }
    const float toFloat() const
    {
        return _rootNode.as<float>();
    }
    const int32_t toInt32() const
    {
        return _rootNode.as<int32_t>();
    }
    const uint32_t toUInt32() const
    {
        return _rootNode.as<uint32_t>();
    }
    const bool toBoolean() const
    {
        return _rootNode.as<bool>();
    }
#ifdef SYSREAL
    const SYSREAL toReal() const
    {
        return _rootNode.as<SYSREAL>();
    }
#endif

    // get array size
    size_t size()
    {
        //assert(_rootNode.IsSequence());
        if (_rootNode.IsSequence())
            return _rootNode.size();
        else
            return 0; // Not an array !!!
    }

private:
    YAML::Node _rootNode;
};

template <typename Key>
const Conf Conf::operator[](const Key &key) const
{
    return Conf(_rootNode[key]);
}

template <typename Key>
Conf Conf::operator[](const Key &key)
{
    return Conf(_rootNode[key]);
}

// value accessor
template <typename ValueType>
const ValueType Conf::to() const
{
    return _rootNode.as<ValueType>();
}

} // namespace Utils
} // namespace dt

#endif // __DT_UTILS_CONF_H__