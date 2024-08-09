namespace dt
{

template <typename T>
Log::LogStream &Log::LogStream::operator<<(const T &value)
{
    _log_stream << value;
    return *this;
}

template <typename... Args>
inline void Log::LogStream::format(spdlog::format_string_t<Args...> fmt_string, Args &&... args)
{
    _log_stream << fmt::format(fmt_string, std::forward<Args>(args)...);
}

template <typename T>
Log::NamedLogStream &Log::NamedLogStream::operator<<(const T &value)
{
    _log_stream << value;
    return *this;
}

template <typename... Args>
inline void Log::NamedLogStream::format(spdlog::format_string_t<Args...> fmt_string, Args &&... args)
{
    _log_stream << fmt::format(fmt_string, std::forward<Args>(args)...);
}

} // namespace dt