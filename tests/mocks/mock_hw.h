#ifndef MOCK_HW_H
#define MOCK_HW_H

#ifdef __cplusplus
extern "C" {
#endif

// We mock system() via linker wrapping
int __wrap_system(const char *command);
void set_mock_system_return(int val);
void set_mock_popen_output(const char* out);
void set_mock_popen_return(int val);
void set_mock_fcntl_fail(int val);
void set_mock_pthread_allow(int val);
void set_mock_rename_fail(int val);
void set_mock_remove_fail(int val);
void set_mock_symlink_fail(int val);
void set_mock_opendir_fail(int val);
void set_mock_pthread_fail(int val);

// Mock storage files used by APIs
extern char mock_hotspot_file[256];
extern char mock_mac_file[256];
extern char mock_serial_file[256];
extern char mock_mfg_file[256];
extern char mock_snapshot_file[256];
extern char mock_factory_status_file[256];

void init_mock_paths();
void create_mock_directory(const char* path);

#ifdef __cplusplus
}
#endif

#endif
