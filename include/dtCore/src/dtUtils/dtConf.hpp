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
#include <vector>
#include <memory>
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
    Conf(const Conf &rhs)
    {
        _rootNode = rhs._rootNode;
    }
    Conf(const std::string &yaml_file)
    {
        _rootNode = YAML::LoadFile(yaml_file);
    }
    Conf(const std::vector<std::string> &yaml_file)
    {
        for (auto it = yaml_file.begin(); it != yaml_file.end(); ++it)
            Append(*it, true);
    }
    Conf(const std::istream &yaml_str)
    {
        _rootNode = YAML::Load(const_cast<std::istream &>(yaml_str));
    }
    Conf(const std::vector<std::shared_ptr<std::istream>> &yaml_str)
    {
        for (auto it = yaml_str.begin(); it != yaml_str.end(); ++it)
            Append(**it, true);
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

    // append from a file
    bool Append(const std::string &yaml_file, bool override = true)
    {
        if (!_rootNode.IsDefined())
            return Load(yaml_file);

        YAML::Node overrideNode = YAML::LoadFile(yaml_file);
        if (!overrideNode.IsDefined())
            return false;

        _rootNode = mergeNodes(_rootNode, overrideNode, !override);
        return true;
    }

    // append from memory
    bool Append(const std::istream &yaml_str, bool override = true)
    {
        if (!_rootNode.IsDefined())
            return Load(yaml_str);

        YAML::Node overrideNode = YAML::Load(const_cast<std::istream &>(yaml_str));
        if (!overrideNode.IsDefined())
            return false;

        _rootNode = mergeNodes(_rootNode, overrideNode, !override);
        return true;
    }

    // save content as a file
    bool Dump(const std::string &out_file) const
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

    // dump content to a output stream
    bool Dump(std::ostream &out) const
    {
        out << _rootNode;
        return true;
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

    // assignment
    template <typename ValueType> const Conf &operator=(const ValueType &value)
    {
        _rootNode = value;
        return *this;
    }

    // assignment to a sequence node
    template <typename ValueType> Conf &operator<<(const ValueType &value)
    {
        _rootNode.push_back(value);
        return *this;
    }

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
    friend std::ostream& operator<<(std::ostream& out, const Conf& conf);

private:
    YAML::Node mergeNodes(const YAML::Node& baseNode, const YAML::Node& overrideNode, bool concatSequences = false) 
    {
        if (!overrideNode.IsDefined() || overrideNode.IsNull()) 
        {
            return baseNode;
        }
        if (!baseNode.IsDefined() || baseNode.IsNull()) 
        {
            return overrideNode;
        }

        if (overrideNode.IsMap() && baseNode.IsMap()) 
        {
            YAML::Node merged = baseNode; // Start with the base node
            for (auto it = overrideNode.begin(); it != overrideNode.end(); ++it) 
            {
                const std::string& key = it->first.as<std::string>();
                merged[key] = mergeNodes(baseNode[key], it->second, concatSequences);
            }
            return merged;
        } 
        else if (concatSequences && overrideNode.IsSequence() && baseNode.IsSequence()) 
        {
            // Concatenate sequences
            YAML::Node merged = baseNode;
            for (const auto& item : overrideNode) 
            {
                merged.push_back(item);
            }
            return merged;
        } 
        else 
        {
            // For scalar or mixed types, override with the overrideNode's value
            return overrideNode;
        }
    }
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

inline std::ostream& operator<<(std::ostream& out, const Conf& conf) 
{
    out << conf._rootNode;
    return out;
}


} // namespace Utils
} // namespace dt

#endif // __DT_UTILS_CONF_H__