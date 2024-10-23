#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <pthread.h>
#include <unistd.h> // For pipe, fork, close, read, write
#include <sys/wait.h> // For wait

#define NUM_THREADS 10 // Số lượng thread
#define ARRAY_SIZE 100000000 // Kích thước mảng
#define NUM_PROCESSES 2 // Số lượng process

int arr[ARRAY_SIZE];

typedef struct {
    int start;
    int end;
    int sum;
} ThreadData;

// Hàm tính tổng một phần của mảng cho threads
void* calculate_sum(void* arg) {
    ThreadData* data = (ThreadData*)arg;
    data->sum = 0;

    for (int i = data->start; i < data->end; i++) {
        data->sum += arr[i];
    }

    return NULL;
}

// Hàm tính tổng một phần của mảng cho processes
void calculate_sum_process(int start, int end, int* sum) {
    *sum = 0;
    for (int i = start; i < end; i++) {
        *sum += arr[i];
    }
}

int main() {
    // Khởi tạo mảng ngẫu nhiên
    srand(time(NULL));
    for (int i = 0; i < ARRAY_SIZE; i++) {
        arr[i] = rand() % 100;
    }

    // Tính tổng bằng single-thread
    clock_t begin = clock();
    int sum_single = 0;
    for (int i = 0; i < ARRAY_SIZE; i++) {
        sum_single += arr[i];
    }
    clock_t end = clock();
    double time_spent = (double)(end - begin) / CLOCKS_PER_SEC;
    printf("Tong bang single-thread: %d, thoi gian: %.6fs\n", sum_single, time_spent);

    // Tính tổng bằng multi-thread
    pthread_t threads[NUM_THREADS];
    ThreadData thread_data[NUM_THREADS];

    int sum_multi_thread = 0;
    begin = clock();
    for (int i = 0; i < NUM_THREADS; i++) {
        thread_data[i].start = i * ARRAY_SIZE / NUM_THREADS;
        thread_data[i].end = (i + 1) * ARRAY_SIZE / NUM_THREADS;
        pthread_create(&threads[i], NULL, calculate_sum, &thread_data[i]);
    }

    for (int i = 0; i < NUM_THREADS; i++) {
        pthread_join(threads[i], NULL);
        sum_multi_thread += thread_data[i].sum;
    }
    end = clock();
    time_spent = (double)(end - begin) / CLOCKS_PER_SEC;
    printf("Tong bang multi-thread: %d, thoi gian: %.6fs\n", sum_multi_thread, time_spent);

    // Tính tổng bằng multi-process
    int pipefd[NUM_PROCESSES][2];
    pid_t pid[NUM_PROCESSES];
    int sum_multi_process = 0;

    begin = clock();
    for (int i = 0; i < NUM_PROCESSES; i++) {
        if (pipe(pipefd[i]) == -1) {
            perror("pipe");
            exit(EXIT_FAILURE);
        }

        pid[i] = fork();
        if (pid[i] == -1) {
            perror("fork");
            exit(EXIT_FAILURE);
        } else if (pid[i] == 0) { // Child process
            close(pipefd[i][0]); // Đóng đầu đọc
            int start = i * ARRAY_SIZE / NUM_PROCESSES;
            int end = (i + 1) * ARRAY_SIZE / NUM_PROCESSES;
            int local_sum;
            calculate_sum_process(start, end, &local_sum);
            write(pipefd[i][1], &local_sum, sizeof(local_sum));
            close(pipefd[i][1]); // Đóng đầu ghi
            exit(EXIT_SUCCESS);
        } else { // Parent process
            close(pipefd[i][1]); // Đóng đầu ghi
        }
    }

    for (int i = 0; i < NUM_PROCESSES; i++) {
        int local_sum;
        read(pipefd[i][0], &local_sum, sizeof(local_sum));
        sum_multi_process += local_sum;
        close(pipefd[i][0]); // Đóng đầu đọc
        wait(NULL); // Chờ child process kết thúc
    }
    end = clock();
    time_spent = (double)(end - begin) / CLOCKS_PER_SEC;
    printf("Tong bang multi-process: %d, thoi gian: %.6fs\n", sum_multi_process, time_spent);

    return 0;
}
