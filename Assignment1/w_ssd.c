#include <stdio.h>
#include <stdlib.h>
#include <time.h>

int main(int argc, char *argv[]) {
    const long int bench_iter = 10;
    struct timespec bench_start_time, bench_stop_time;
    double bench_diff_time;

    srand(time(NULL));

    const int size = 1 << 20;
    int buf[size];
    for (size_t ix = 0; ix < size; ++ix) {
        buf[ix] = ix;
    }

    FILE *pFile;

    pFile = fopen("/run/mount/scratch/hpcuser084/testssd" , "w");
    if (pFile == NULL){
        perror("Error opening file");
        return 1;
    }
 
    timespec_get(&bench_start_time, TIME_UTC);
    for (size_t jx = 0; jx < bench_iter; ++jx) {
        fwrite(buf, sizeof(int), size, pFile);
        fflush(pFile);
        fread(buf, sizeof(int), size, pFile);
    }
    timespec_get(&bench_stop_time, TIME_UTC);

    fclose(pFile);

    bench_diff_time =
        difftime(bench_stop_time.tv_sec, bench_start_time.tv_sec) * 1000000
        + (bench_stop_time.tv_nsec - bench_start_time.tv_nsec) / 1000;
    printf("benchmark time for one iteration: %fmus\n",
        bench_diff_time / bench_iter);

    return 0;
}
