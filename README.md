# Laporan Hasil Praktikum Sistem Operasi 2024 Modul 4 - IT02

## Anggota Kelompok IT 02 :

- Maulana Ahmad Zahiri (5027231010)
- Syela Zeruya Tandi Lalong (5027231076)
- Kharisma Fahrun Nisa' (5027231086)

## Daftar Isi

- [Soal 1](#soal-1)
- [Soal 2](#soal-2)
- [Soal 3](#soal-3)

# Soal 1

## Deskripsi Soal

## Pengerjaan

## Penjelasan

## Dokumentasi

# Soal 2

## Deskripsi Soal

## Pengerjaan

## Penjelasan

## Dokumentasi

# Soal 3

`> Maulana`

## Deskripsi Soal

Seorang arkeolog menemukan sebuah gua yang didalamnya tersimpan banyak relik dari zaman praaksara, sayangnya semua barang yang ada pada gua tersebut memiliki bentuk yang terpecah belah akibat bencana yang tidak diketahui.

Sang arkeolog ingin menemukan cara cepat agar ia bisa menggabungkan relik-relik yang terpecah itu, namun karena setiap pecahan relik itu masih memiliki nilai tersendiri, ia memutuskan untuk membuat sebuah file system yang mana saat ia mengakses file system tersebut ia dapat melihat semua relik dalam keadaan utuh, sementara relik yang asli tidak berubah sama sekali.

## Pengerjaan

jadi yang paling awal yakni membuat direktori seperti pada soal yakni:

![alt text](images/image.png)

dimana pada `nama_bebas` saya menamainya dengan fuze, dimana nantinya akan menjadi direktori fuse dengan yang direktori asalnya adalah direktori relics.

dan sebelumnya kita disediakan link untuk mendownload file relics yang diperlukan, yakni kira mendownloadnya dengan cara:

```
curl -L -o '[link download file]'
```

dimana pada bagian dalam kurung siku diisi dengan link untuk file yang akan di download.

kemudian untuk selanjutnya kita akan men settingnya sesuai dengan permintaan soal.

untuk yang pertama, yakni kita membuat program archeology.c dimana berisi fungsi-fungsi terkait dengan penyelesaian soal.

```c
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

// Deklarasi fungsi-fungsi yang akan digunakan
static int split_file(const char *path);
static int xmp_combine_files(const char *relic_name);
static int delete_parts(const char *path);
```

pada program tersebut kurang lebih merupakan sebuah deklarasi dan juga memberitahu tentang dimana file tersebut disimpan.

```c
static int xmp_getattr(const char *path, struct stat *stbuf) {
    int res;
    char fpath[1000];
    sprintf(fpath, "%s%s", relics_path, path); // Mengubah ke direktori asal untuk akses ke relics

    if (access(fpath, F_OK) == 0) {
        res = lstat(fpath, stbuf);
    } else {
        sprintf(fpath, "%s%s", fuze_path, path); // Mengubah ke fuze_path untuk akses gabungan
        res = lstat(fpath, stbuf);
    }

    if (res == -1) return -errno;
    return 0;
}

static int xmp_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi) {
    char fpath[1000];
    if (strcmp(path, "/") == 0)
        path = fuze_path;
    else
        sprintf(fpath, "%s%s", fuze_path, path);

    if (strcmp(path, fuze_path) == 0) {
        DIR *dp;
        struct dirent *de;
        (void)offset;
        (void)fi;

        dp = opendir(relics_path); // Buka direktori relics
        if (dp == NULL) return -errno;

        while ((de = readdir(dp)) != NULL) {
            // Skip pecahan file
            if (strchr(de->d_name, '.') && de->d_name[strlen(de->d_name) - 4] == '.') {
                continue;
            }
            struct stat st;
            memset(&st, 0, sizeof(st));
            st.st_ino = de->d_ino;
            st.st_mode = de->d_type << 12;
            if (filler(buf, de->d_name, &st, 0))
                break;
        }

        closedir(dp);
    } else {
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
    }

    return 0;
}

static int xmp_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {
    char fpath[1000];
    sprintf(fpath, "%s%s", relics_path, path); // Mengubah ke direktori asal untuk akses ke relics

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

static int xmp_write(const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {
    // Simpan sementara di direktori fuze
    char temp_fpath[1000];
    sprintf(temp_fpath, "%s%s", fuze_path, path);

    int fd = open(temp_fpath, O_WRONLY | O_CREAT, 0644);
    if (fd == -1) return -errno;

    int res = pwrite(fd, buf, size, offset);
    if (res == -1) res = -errno;

    close(fd);

    // Setelah menulis, pecah file dan pindahkan ke relics
    res = split_file(path);
    if (res != 0) return res;

    // Hapus file sementara
    unlink(temp_fpath);

    return size;
}
```

kemudian setelah itu juga terdapat beberapa fungsi contoh nya fungsi untuk membaca, menulis, dan menginisiasi file.

```c
static int xmp_combine_files(const char *relic_name) {
    char combined_file_path[1000];
    sprintf(combined_file_path, "%s/%s", fuze_path, relic_name);

    FILE *output_file = fopen(combined_file_path, "w");
    if (output_file == NULL) {
        perror("Failed to open output file for combining");
        return -errno;
    }

    char part_file_path[1000];
    int part_number = 0;
    while (1) {
        sprintf(part_file_path, "%s/%s.%03d", relics_path, relic_name, part_number);
        FILE *input_file = fopen(part_file_path, "r");
        if (input_file == NULL) break;

        char buffer[10240]; // 10 KB buffer
        size_t bytes_read;
        while ((bytes_read = fread(buffer, 1, sizeof(buffer), input_file)) > 0) {
            fwrite(buffer, 1, bytes_read, output_file);
        }

        fclose(input_file);
        part_number++;
    }

    fclose(output_file);
    return 0;
}

static struct fuse_operations xmp_oper = {
    .getattr = xmp_getattr,
    .readdir = xmp_readdir,
    .read = xmp_read,
    .write = xmp_write,
    .create = xmp_create,
    .unlink = xmp_unlink,
};

static void combine_all_files() {
    // Gabungkan file-file pecahan menjadi 10 file gabungan
    for (int i = 1; i <= 10; i++) {
        char relic_name[100];
        sprintf(relic_name, "relic_%d.png", i);
        xmp_combine_files(relic_name);
    }
}
```

kemudian juga terdapat fungsi untuk menyatukan beberapa pecahan gambar pada direktori relics, yang akan tergabung melalui fuse pada direktori fuze.

sehingga apabila dilakukan listing file, maka akan menampilkan file yang tergabung dalam direktori fuze, dan file yang terpecah pada direktori relics.

```c

static int split_file(const char *path) {
    char temp_fpath[1000];
    sprintf(temp_fpath, "%s%s", fuze_path, path); // Path ke file sementara di direktori fuze

    FILE *input_file = fopen(temp_fpath, "rb");
    if (input_file == NULL) {
        perror("Failed to open input file");
        return -errno;
    }

    fseek(input_file, 0, SEEK_END);
    long file_size = ftell(input_file);
    fseek(input_file, 0, SEEK_SET);

    if (file_size <= 0) {
        fclose(input_file);
        printf("File is empty or error getting file size: %s\n", temp_fpath);
        return 0;
    }

    printf("Splitting file: %s of size %ld bytes\n", temp_fpath, file_size);

    char part_file_path[1000];
    int part_number = 0;
    size_t total_bytes_read = 0;
    while (total_bytes_read < file_size) {
        sprintf(part_file_path, "%s/%s.%03d", relics_path, path, part_number);
        FILE *output_file = fopen(part_file_path, "wb");
        if (output_file == NULL) {
            perror("Failed to open output file");
            fclose(input_file);
            return -errno;
        }

        char buffer[10240]; // 10 KB buffer
        size_t bytes_read = fread(buffer, 1, sizeof(buffer), input_file);
        if (bytes_read > 0) {
            fwrite(buffer, 1, bytes_read, output_file);
            total_bytes_read += bytes_read;
            printf("Written %zu bytes to %s\n", bytes_read, part_file_path);
        }

        fclose(output_file);
        part_number++;
    }

    fclose(input_file);

    printf("Total bytes read: %zu\n", total_bytes_read);
    printf("Split file completed.\n");

    return 0;
}


static int delete_parts(const char *path) {
    char fpath[1000];
    sprintf(fpath, "%s%s", relics_path, path); // Path ke file di direktori relics

    char part_file_path[1000];
    for (int i = 0; ; i++) {
        sprintf(part_file_path, "%s.%03d", fpath, i);
        if (access(part_file_path, F_OK) != 0) break; // Stop jika tidak ada lagi pecahan

        if (remove(part_file_path) != 0) {
            perror("Failed to remove part file");
            return -errno;
        } else {
            printf("Removed part file %s\n", part_file_path);
        }
    }

    printf("All part files removed for %s\n", fpath);
    return 0;
}
```

pada program diatas terdapat fungsi untuk memecah file dan juga menghapus file.

yakni apabila kita mengupload file dengan ukuran diatas 10 kb dan menaruhnya pada direktori fuze, maka akan dilakukan pemecahan pada direktori relics sehingga menampilkan banyak jumah pecahan dengan format `[namafile].000` .

kemudian apabila kita menghapus file tersebut, maka pecahan file tersebut juga kan ikut terhapus.

![alt text](images/imagelutpi.png)

sebagai contoh saat saya mengupload file lutpi dengan ukuran 18 kb , maka akan terpecah menjadi 2 gambar, dimana berukuran 10kb, dan satunya 8 kb.

![alt text](images/imagereport.png)

pada gambar diatas merupakan list pada direktori report yang sebelumnya disalin dari direktori fuze.

![alt text](images/rep.png)
gambar diatas meupakan list dari direktori report jika dilihat melalui finder pada mac.

sekian terimakasih.

## Dokumentasi

![alt text](<images/archeology.c -o archeology.png>)
Berhasil di compile dan di run Tanpa ada masalah mounting

![alt text](images/relic_1.png)
tampilan direktori fuze setelah berhasil menggabungkan

![alt text](<images/Pasted Graphic 4.png>)
Berhasil menampilkan pecahan relics

![alt text](images/boom.png)
berhasil memuat dan menyalin direktori fuze ke direktori report

## beberapa error sebelumnya

![alt text](<images/• mwlanaz@MawlsBook-Pro soal_3  cp luffy-png .fuze.png>)
saat elum berhasil menampilkan pecahan dari gambar yang diupload, kemudian setelah di coba dengan menambahkan fungsi diatas dapat berjalan dengan baik.

![alt text](<images/total 280.png>)
fuse belum berjalan, dan kemudian setelah di lengkapi dengan fungsi fuse diatas dapat berjalan dengan baik.
