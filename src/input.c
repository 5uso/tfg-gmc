#include "input.h"

int __count_dir_files(const char * path) {
    int cnt = 0;
    struct dirent * entry;
    DIR * d = opendir(path);

    if(!d) return 0;

    while((entry = readdir(d))) cnt += (entry->d_type == DT_REG);

    closedir(d);
    return cnt;
}

void __fill_matrix(matrix m, FILE * fd) {
    for(int x = 0; x < m.w; x++)
        for(int y = 0; y < m.h; y++)
            fscanf(fd, "%lf", &m.data[y * m.w + x]);
}

matrix * read_dataset(const char * path) {
    int views = __count_dir_files(path);
    if(!views) return NULL;

    matrix * data = malloc(sizeof(matrix) * views);

    struct dirent * entry;
    DIR * d = opendir(path);

    for(int i = 0; i < views; i++) {
        entry = readdir(d);
        if(entry->d_type != DT_REG) {
            i--;
            continue;
        }

        char filepath[1024];
        snprintf(filepath, 1024, "%s/%s", path, entry->d_name);
        filepath[1023] = '\0';
        FILE * fd = fopen(filepath, "r");

        uint w, h;
        fscanf(fd, "%u %u", &w, &h);
        data[i] = new_matrix(w, h);

        __fill_matrix(data[i], fd);
    }

    return data;
}
