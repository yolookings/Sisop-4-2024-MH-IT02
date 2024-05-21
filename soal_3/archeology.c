#define FUSE_USE_VERSION 28
#include <fuse.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <errno.h>
#include <sys/time.h>
#include <stdlib.h>

static const char *relics_path = "/Users/mwlanaz/Desktop/praktikum/praktikum-sisop/modul-4/soal_3/relics"; // Ganti dengan path ke direktori relics
static const char *fuze_path = "/Users/mwlanaz/Desktop/praktikum/praktikum-sisop/modul-4/soal_3/fuze"; // Ganti dengan path ke direktori fuze

static int xmp_getattr(const char *path, struct stat *stbuf) {
    int res;
    char fpath[1000];
    sprintf(fpath, "%s%s", fuze_path, path); // Mengubah ke fuze_path untuk akses gabungan
    res = lstat(fpath, stbuf);
    if (res == -1) return -errno;
    return 0;
}

static int xmp_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi) {
    char fpath[1000];
    if (strcmp(path, "/") == 0)
        path = fuze_path;
    else
        sprintf(fpath, "%s%s", fuze_path, path);

    DIR *dp;
    struct dirent *de;
    (void)offset;
    (void)fi;

    dp = opendir(fpath);
    if (dp == NULL) return -errno;

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

static int xmp_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {
    char fpath[1000];
    sprintf(fpath, "%s%s", fuze_path, path); // Mengubah ke fuze_path untuk akses gabungan

    int fd;
    int res;
    (void)fi;

    fd = open(fpath, O_RDONLY);
    if (fd == -1) return -errno;

    res = pread(fd, buf, size, offset);
    if (res == -1) res = -errno;

    close(fd);
    return res;
}

static int xmp_combine_files(const char *relic_name) {
    char combined_file_path[1000];
    sprintf(combined_file_path, "%s/%s", fuze_path, relic_name);

    FILE *output_file = fopen(combined_file_path, "w");
    if (output_file == NULL) {
        return -errno;
    }

    char part_file_path[1000];
    for (int i = 0; ; i++) {
        sprintf(part_file_path, "%s/%s.%03d", relics_path, relic_name, i);
        FILE *input_file = fopen(part_file_path, "r");
        if (input_file == NULL) break; // Stop if no more part files

        char buffer[1024];
        size_t bytes_read;
        while ((bytes_read = fread(buffer, 1, sizeof(buffer), input_file)) > 0) {
            fwrite(buffer, 1, bytes_read, output_file);
        }
        fclose(input_file);
    }

    fclose(output_file);
    return 0;
}

static struct fuse_operations xmp_oper = {
    .getattr = xmp_getattr,
    .readdir = xmp_readdir,
    .read = xmp_read,
};

static void combine_all_files() {
    // Gabungkan file-file pecahan menjadi 10 file gabungan
    for (int i = 1; i <= 10; i++) {
        char relic_name[100];
        sprintf(relic_name, "relic_%d.png", i);
        xmp_combine_files(relic_name);
    }
}

int main(int argc, char *argv[]) {
    umask(0);

    // Gabungkan file-file sebelum menjalankan FUSE
    combine_all_files();

    return fuse_main(argc, argv, &xmp_oper, NULL);
}
