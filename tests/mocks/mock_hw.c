#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>

char mock_hotspot_file[256];
char mock_mac_file[256];
char mock_serial_file[256];
char mock_mfg_file[256];
char mock_snapshot_file[256];
char mock_factory_status_file[256];

static int mock_system_ret = 0;
static char mock_popen_out[1024] = "";
static int mock_popen_ret = 0;
static int mock_fcntl_fail = 0;
static int mock_pthread_allow = 1;
static int mock_rename_fail = 0;
static int mock_remove_fail = 0;
static int mock_symlink_fail = 0;
static int mock_opendir_fail = 0;
static int mock_pthread_fail = 0;

void set_mock_system_return(int val) {
    mock_system_ret = val;
}

void set_mock_popen_output(const char* out) {
    strncpy(mock_popen_out, out, sizeof(mock_popen_out) - 1);
}

void set_mock_popen_return(int val) {
    mock_popen_ret = val;
}

void set_mock_fcntl_fail(int val) {
    mock_fcntl_fail = val;
}

void set_mock_pthread_allow(int val) {
    mock_pthread_allow = val;
}

void set_mock_rename_fail(int val) {
    mock_rename_fail = val;
}

void set_mock_remove_fail(int val) {
    mock_remove_fail = val;
}

void set_mock_symlink_fail(int val) {
    mock_symlink_fail = val;
}

void set_mock_opendir_fail(int val) {
    mock_opendir_fail = val;
}

void set_mock_pthread_fail(int val) {
    mock_pthread_fail = val;
}

extern int __real_system(const char *command);

int __wrap_system(const char *command) {
    if (command && (strncmp(command, "mkdir", 5) == 0 || strncmp(command, "rm -rf /tmp", 11) == 0)) {
        return __real_system(command);
    }
    printf("MOCK SYSTEM CALL: %s\n", command);
    return mock_system_ret;
}

FILE* __real_popen(const char* command, const char* type);
FILE* __wrap_popen(const char* command, const char* type) {
    printf("MOCK POPEN CALL: %s\n", command);
    FILE* f = tmpfile();
    if (f) {
        fputs(mock_popen_out, f);
        rewind(f);
    }
    return f;
}

int __real_pclose(FILE* stream);
int __wrap_pclose(FILE* stream) {
    if (stream) fclose(stream);
    return mock_popen_ret;
}

#include <stdarg.h>
#include <fcntl.h>
#include <errno.h>

int __real_fcntl(int fd, int cmd, ...);
int __wrap_fcntl(int fd, int cmd, ...) {
    va_list args;
    va_start(args, cmd);
    void* arg = va_arg(args, void*);
    va_end(args);
    
    if (cmd == F_SETLK && mock_fcntl_fail) {
        errno = EACCES;
        return -1;
    }
    // For other commands, we might need more va_arg calls but F_SETLK only takes one.
    // However, some fcntl calls might use int arg.
    return __real_fcntl(fd, cmd, arg);
}

#include <pthread.h>
int __real_pthread_create(pthread_t *thread, const pthread_attr_t *attr,
                          void *(*start_routine) (void *), void *arg);
int __wrap_pthread_create(pthread_t *thread, const pthread_attr_t *attr,
                          void *(*start_routine) (void *), void *arg) {
    if (mock_pthread_fail) return EAGAIN;
    if (mock_pthread_allow) {
        return __real_pthread_create(thread, attr, start_routine, arg);
    }
    printf("MOCK PTHREAD_CREATE: Suppressed\n");
    return 0;
}

int __real_rename(const char *oldpath, const char *newpath);
int __wrap_rename(const char *oldpath, const char *newpath) {
    if (mock_rename_fail) {
        errno = EACCES;
        return -1;
    }
    return __real_rename(oldpath, newpath);
}

int __real_remove(const char *pathname);
int __wrap_remove(const char *pathname) {
    if (mock_remove_fail) {
        errno = EACCES;
        return -1;
    }
    return __real_remove(pathname);
}

int __real_symlink(const char *target, const char *linkpath);
int __wrap_symlink(const char *target, const char *linkpath) {
    if (mock_symlink_fail) {
        errno = EACCES;
        return -1;
    }
    return __real_symlink(target, linkpath);
}

#include <dirent.h>
DIR* __real_opendir(const char *name);
DIR* __wrap_opendir(const char *name) {
    if (mock_opendir_fail) {
        errno = ENOENT;
        return NULL;
    }
    return __real_opendir(name);
}

void create_mock_directory(const char* path) {
    char command[512];
    snprintf(command, sizeof(command), "mkdir -p %s", path);
    system(command);
}
