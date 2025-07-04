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
#include <fstream>

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

    // load from a file
    bool Load(const std::string &yaml_file)
    {
        std::ifstream fin(yaml_file);
        if (fin.is_open())
        {
            _rootNode = YAML::Load(fin);
            return true;
        }
        else
            return false;
    }

    // load from memory
    bool Load(const std::istream &yaml_str)
    {
        _rootNode = YAML::Load(const_cast<std::istream &>(yaml_str));
        return true;
    }

    // save content as a file
    bool Dump(const std::string &out_file)
    {
        std::ofstream fout(out_file);
        if (fout.is_open())
        {
            fout << _rootNode;
            return true;
        }
        else
            return false;
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
    const size_t size() const
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
    try
    {
        return Conf(_rootNode[key]);
    }
    catch (const YAML::InvalidNode &e)
    {
        std::stringstream msg;
        msg << "invalid config node; invalid key: \"" << YAML::key_to_string(key) << "\"";
        throw std::runtime_error(msg.str());
    }
}

template <typename Key>
Conf Conf::operator[](const Key &key)
{
    try
    {
        return Conf(_rootNode[key]);
    }
    catch (const YAML::InvalidNode &e)
    {
        std::stringstream msg;
        msg << "invalid config node; invalid key: \"" << YAML::key_to_string(key) << "\"";
        throw std::runtime_error(msg.str());
    }
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