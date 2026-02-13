
#include <cstdio>
#include <source_location>

template<typename... Args>
void LOG_INFO(const char* fmt,
              Args&&... args,
              const std::source_location& loc = std::source_location::current())
{
    std::fprintf(stderr, "[INFO] %s(): ", loc.function_name());
    std::fprintf(stderr, fmt, std::forward<Args>(args)...);
    std::fprintf(stderr, "\n");
}

template<typename... Args>
void LOG_ERROR(const char* fmt,
               Args&&... args,
               const std::source_location& loc = std::source_location::current())
{
    std::fprintf(stderr, "[ERROR] %s(): ", loc.function_name());
    std::fprintf(stderr, fmt, std::forward<Args>(args)...);
    std::fprintf(stderr, "\n");
}

template<typename... Args>
#ifdef DEBUG
void LOG_DEBUG(const char* fmt,
               Args&&... args,
               const std::source_location& loc = std::source_location::current())
{
    std::fprintf(stderr, "[DEBUG] %s(): ", loc.function_name());
    std::fprintf(stderr, fmt, std::forward<Args>(args)...);
    std::fprintf(stderr, "\n");
}    
#else
    #define LOG_DEBUG(fmt, ...) ((void)0)
#endif
