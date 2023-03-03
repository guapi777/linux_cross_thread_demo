#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <liburing.h>

#define BUF_SIZE 2

int main(int argc, char *argv[]) {
    struct io_uring ring;
    struct io_uring_sqe *sqe;
    struct io_uring_cqe *cqe;
    int ret, file_fd, i;
    off_t file_size, offset = 0;
    char *buf1, *buf2;

    // Open the file
    if ((file_fd = open(argv[1], O_RDONLY)) == -1) {
        perror("open");
        exit(1);
    }

    // Get the file size
    struct stat st;
    if (fstat(file_fd, &st) == -1) {
        perror("fstat");
        exit(1);
    }
    file_size = st.st_size;

    // Allocate the buffers for IO
    buf1 = malloc(BUF_SIZE);
    buf2 = malloc(BUF_SIZE);

    // Initialize the IO ring
    if (io_uring_queue_init(2, &ring, 0) < 0) {
        perror("io_uring_queue_init");
        exit(1);
    }

    // Submit the first batch of IO requests
    sqe = io_uring_get_sqe(&ring);
    io_uring_prep_read(sqe, file_fd, buf1, BUF_SIZE, offset);
    io_uring_sqe_set_data(sqe, buf1);
    offset += BUF_SIZE;

    sqe = io_uring_get_sqe(&ring);
    io_uring_prep_read(sqe, file_fd, buf2, BUF_SIZE, offset);
    io_uring_sqe_set_data(sqe, buf2);
    offset += BUF_SIZE;

    // Wait for completion of IO requests and print the data
    for (i = 0; i < file_size / BUF_SIZE; i++) {
        ret = io_uring_submit(&ring);
        if (ret < 0) {
            perror("io_uring_submit");
            exit(1);
        }

        ret = io_uring_wait_cqe(&ring, &cqe);
        if (ret < 0) {
            perror("io_uring_wait_cqe");
            exit(1);
        }

        char *data = (char *) io_uring_cqe_get_data(cqe);
        if (data == buf1) {
            printf("Data from buf1: %s\n", data);
        } else if (data == buf2) {
            printf("Data from buf2: %s\n", data);
        }

        io_uring_cqe_seen(&ring, cqe);

        sqe = io_uring_get_sqe(&ring);
        io_uring_prep_read(sqe, file_fd, data, BUF_SIZE, offset);
        io_uring_sqe_set_data(sqe, data);
        offset += BUF_SIZE;
    }

    // Cleanup
    free(buf1);
    free(buf2);
    close(file_fd);
    io_uring_queue_exit(&ring);

    return 0;
}