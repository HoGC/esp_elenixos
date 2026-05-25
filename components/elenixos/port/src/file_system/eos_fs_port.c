#include <dirent.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "eos_fs_port.h"

eos_file_t eos_fs_open_read(const char *path)
{
    return path ? fopen(path, "rb") : EOS_FILE_INVALID;
}

eos_file_t eos_fs_open_write(const char *path)
{
    return path ? fopen(path, "wb") : EOS_FILE_INVALID;
}

int eos_fs_read(eos_file_t file, void *buf, size_t len)
{
    if (!file || !buf) {
        return -1;
    }
    size_t n = fread(buf, 1, len, file);
    return (n == 0 && ferror(file)) ? -1 : (int)n;
}

int eos_fs_write(eos_file_t file, const void *buf, size_t len)
{
    if (!file || !buf) {
        return -1;
    }
    size_t n = fwrite(buf, 1, len, file);
    return n == len ? (int)n : -1;
}

int eos_fs_seek(eos_file_t file, uint32_t pos)
{
    return file ? fseek(file, (long)pos, SEEK_SET) : -1;
}

int eos_fs_size(eos_file_t file, uint32_t *size)
{
    if (!file || !size) {
        return -1;
    }
    long cur = ftell(file);
    if (cur < 0 || fseek(file, 0, SEEK_END) != 0) {
        return -1;
    }
    long end = ftell(file);
    if (end < 0 || fseek(file, cur, SEEK_SET) != 0) {
        return -1;
    }
    *size = (uint32_t)end;
    return 0;
}

int eos_fs_tell(eos_file_t file, uint32_t *pos)
{
    if (!file || !pos) {
        return -1;
    }
    long cur = ftell(file);
    if (cur < 0) {
        return -1;
    }
    *pos = (uint32_t)cur;
    return 0;
}

void eos_fs_close(eos_file_t file)
{
    if (file) {
        fclose(file);
    }
}

int eos_fs_mkdir(const char *path)
{
    if (!path) {
        return -1;
    }
    if (mkdir(path, 0775) == 0 || errno == EEXIST) {
        return 0;
    }
    return -1;
}

int eos_fs_rmdir(const char *path)
{
    return path ? rmdir(path) : -1;
}

int eos_fs_remove(const char *path)
{
    return path ? remove(path) : -1;
}

int eos_fs_exists(const char *path)
{
    if (!path) {
        return 0;
    }
    struct stat st;
    return stat(path, &st) == 0 ? 1 : 0;
}

int eos_fs_type(const char *path)
{
    struct stat st;
    if (!path || stat(path, &st) != 0) {
        return EOS_FS_TYPE_NOT_EXIST;
    }
    return S_ISDIR(st.st_mode) ? EOS_FS_TYPE_DIR : EOS_FS_TYPE_FILE;
}

eos_dir_t eos_fs_opendir(const char *path)
{
    return path ? opendir(path) : EOS_DIR_INVALID;
}

int eos_fs_readdir(eos_dir_t dir, char *name, size_t max_len)
{
    if (!dir || !name || max_len == 0) {
        return -1;
    }
    struct dirent *entry = readdir(dir);
    if (!entry) {
        return -1;
    }
    strlcpy(name, entry->d_name, max_len);
    return 0;
}

void eos_fs_closedir(eos_dir_t dir)
{
    if (dir) {
        closedir(dir);
    }
}

int eos_fs_mv(const char *old_path, const char *new_path)
{
    return (old_path && new_path && rename(old_path, new_path) == 0) ? 0 : -1;
}

int eos_fs_sync(eos_file_t file)
{
    return file ? fflush(file) : -1;
}
