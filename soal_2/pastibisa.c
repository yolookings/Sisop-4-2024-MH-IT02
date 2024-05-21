#define FUSE_USE_VERSION 30

#include <fuse.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>
#include <fcntl.h>
#include <unistd.h>
#include <openssl/bio.h>
#include <openssl/evp.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>

// Fungsi untuk mencatat ke log
void log_action(const char *status, const char *action, const char *info) {
    time_t now;
    struct tm *local_time;
    char timestamp[20];

    time(&now);
    local_time = localtime(&now);
    strftime(timestamp, sizeof(timestamp), "%d/%m/%Y-%H:%M:%S", local_time);

    FILE *log_file = fopen("logs-fuse.log", "a");
    if (log_file == NULL) {
        perror("Failed to open log file");
        exit(EXIT_FAILURE);
    }

    fprintf(log_file, "[%s]::%s::%s::%s\n", status, timestamp, action, info);
    fclose(log_file);
}

// Fungsi untuk mendekode Base64
char *base64_decode(const char *input, int length, int *out_len) {
    BIO *bio, *b64;
    int decodeLen = (length * 3) / 4;
    char *buffer = (char *)malloc(decodeLen + 1);
    memset(buffer, 0, decodeLen + 1);

    bio = BIO_new_mem_buf(input, length);
    b64 = BIO_new(BIO_f_base64());
    bio = BIO_push(b64, bio);

    *out_len = BIO_read(bio, buffer, length);
    buffer[*out_len] = '\0';

    BIO_free_all(bio);

    return buffer;
}

// Fungsi untuk mendekode ROT13
void rot13_decode(char *str) {
    while (*str) {
        if (isalpha(*str)) {
            if ((*str >= 'a' && *str <= 'm') || (*str >= 'A' && *str <= 'M'))
                *str += 13;
            else
                *str -= 13;
        }
        str++;
    }
}

// Fungsi untuk mendekode Hex
char *hex_decode(const char *input, int length) {
    int len = length / 2;
    char *output = (char *)malloc(len + 1);
    memset(output, 0, len + 1);

    for (int i = 0; i < len; i++) {
        sscanf(input + 2 * i, "%2hhx", &output[i]);
    }

    return output;
}

// Fungsi untuk membalikkan string
void reverse_string(char *str) {
    int length = strlen(str);
    char *start = str;
    char *end = str + length - 1;

    while (start < end) {
        char temp = *start;
        *start = *end;
        *end = temp;
        start++;
        end--;
    }
}

// Fungsi untuk mendekripsi konten file
void decrypt_content(char *content, int length) {
    if (strncmp(content, "base64,", 7) == 0) {
        int out_len;
        char *decoded = base64_decode(content + 7, length - 7, &out_len);
        memmove(content, decoded, out_len + 1);
        free(decoded);
        log_action("SUCCESS", "base64Decode", "Base64 decoded successfully");
    } else if (strncmp(content, "rot13,", 6) == 0) {
        rot13_decode(content + 6);
        memmove(content, content + 6, length - 5);
        log_action("SUCCESS", "rot13Decode", "ROT13 decoded successfully");
    } else if (strncmp(content, "hex,", 4) == 0) {
        char *decoded = hex_decode(content + 4, length - 4);
        strcpy(content, decoded);
        free(decoded);
        log_action("SUCCESS", "hexDecode", "Hex decoded successfully");
    } else if (strncmp(content, "rev,", 4) == 0) {
        reverse_string(content + 4);
        memmove(content, content + 4, length - 3);
        log_action("SUCCESS", "reverseDecode", "Reverse decoded successfully");
    } else {
        log_action("FAILED", "decode", "Unsupported encoding prefix");
    }
}

static const char *pesan_dir = "/home/syela/tes/sensitif/pesan";

// Fungsi untuk membaca atribut file
static int xmp_getattr(const char *path, struct stat *stbuf) {
    int res;
    char fpath[1000];
    snprintf(fpath, sizeof(fpath), "%s%s", pesan_dir, path);
    res = lstat(fpath, stbuf);
    if (res == -1)
        return -errno;

    return 0;
}

// Fungsi untuk membaca isi direktori
static int xmp_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
                       off_t offset, struct fuse_file_info *fi) {
    DIR *dp;
    struct dirent *de;
    char fpath[1000];

    snprintf(fpath, sizeof(fpath), "%s%s", pesan_dir, path);
    dp = opendir(fpath);
    if (dp == NULL)
        return -errno;

    while ((de = readdir(dp)) != NULL) {
        struct stat st;
        memset(&st, 0, sizeof(st));
        st.st_ino = de->d_ino;
        st.st_mode = de->d_type << 12;
        if (filler(buf, de->d_name, &st, 0))
            break;
    }

    closedir(dp);
    return 0;
}

// Fungsi untuk membuka file
static int xmp_open(const char *path, struct fuse_file_info *fi) {
    int res;
    char fpath[1000];

    snprintf(fpath, sizeof(fpath), "%s%s", pesan_dir, path);
    res = open(fpath, fi->flags);
    if (res == -1)
        return -errno;

    close(res);
    return 0;
}

// Fungsi untuk membaca isi file
static int xmp_read(const char *path, char *buf, size_t size, off_t offset,
                    struct fuse_file_info *fi) {
    int fd;
    int res;
    char fpath[1000];
    char *content;

    snprintf(fpath, sizeof(fpath), "%s%s", pesan_dir, path);
    fd = open(fpath, O_RDONLY);
    if (fd == -1)
        return -errno;

    content = (char *)malloc(size + 1);
    if (!content) {
        close(fd);
        return -ENOMEM;
    }

    res = pread(fd, content, size, offset);
    if (res == -1) {
        free(content);
        close(fd);
        return -errno;
    }

    content[res] = '\0';
    decrypt_content(content, res); // Dekripsi konten sebelum disalin ke buffer
    memcpy(buf, content, res);
    free(content);
    close(fd);
    return res;
}

static struct fuse_operations xmp_oper = {
    .getattr    = xmp_getattr,
    .readdir    = xmp_readdir,
    .open       = xmp_open,
    .read       = xmp_read,
};

int main(int argc, char *argv[]) {
    return fuse_main(argc, argv, &xmp_oper, NULL);
}
