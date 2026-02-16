#include <stdio.h>
#include <stdarg.h>

#ifdef DEBUG
    #define LOG_DEBUG(fmt, ...) \
        fprintf(stderr, "[DEBUG] %s(): " fmt "\n", __func__, ##__VA_ARGS__)
#else
    #define LOG_DEBUG(fmt, ...) ((void)0)
#endif

#define LOG_INFO(fmt, ...) \
    fprintf(stderr, "[INFO] %s(): " fmt "\n", __func__, ##__VA_ARGS__)

#define LOG_ERROR(fmt, ...) \
  fprintf(stderr, "[ERROR] %s(): " fmt "\n", __func__, ##__VA_ARGS__)
