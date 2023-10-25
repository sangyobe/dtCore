namespace dtCore {

template <typename T>
dtLog::LogStream& dtLog::LogStream::operator<<(const T& value) 
{
    _log_stream << value;
    return *this;
}

template<typename... Args>
inline void dtLog::LogStream::format(spdlog::format_string_t<Args...> fmt_string, Args &&...args)
{
    _log_stream << fmt::format(fmt_string, std::forward<Args>(args)...);
}

template <typename T>
dtLog::NamedLogStream& dtLog::NamedLogStream::operator<<(const T& value) 
{
    _log_stream << value;
    return *this;
}

template<typename... Args>
inline void dtLog::NamedLogStream::format(spdlog::format_string_t<Args...> fmt_string, Args &&...args)
{
    _log_stream << fmt::format(fmt_string, std::forward<Args>(args)...);
}

} // namespace dtCore