#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <liburing.h>

#define QUEUE_DEPTH 2
#define BLOCK_SIZE 4

int main(int argc, char *argv[]) {
    struct io_uring ring;
    struct io_uring_sqe *sqe1;
    struct io_uring_sqe *sqe2;
    struct io_uring_cqe *cqe;
    void *data;
    int count = 0;

    int ret = io_uring_queue_init(QUEUE_DEPTH, &ring, 0);
    if (ret != 0) {
        fprintf(stderr, "io_uring_queue_init error\n");
        exit(1);
    }

    char *buf1 = "buf1";
    char *buf2 = "buf2";

    int fd = open(argv[1], O_CREAT | O_RDWR | O_TRUNC, 0644);
    if (fd < 0) {
        fprintf(stderr, "open file error\n");
        exit(1);
    }

    //sqe1
    sqe1 = io_uring_get_sqe(&ring);

    if (sqe1 == NULL) {
        fprintf(stderr, "io_uring_get_sqe error\n");
        exit(1);
    }

    io_uring_prep_write(sqe1, fd, buf1, BLOCK_SIZE, 0);
    io_uring_sqe_set_data(sqe1, buf1);

    //sqe2
    sqe2 = io_uring_get_sqe(&ring);
    if (sqe2 == NULL) {
        fprintf(stderr, "io_uring_get_sqe error\n");
        exit(1);
    }
    io_uring_prep_write(sqe2, fd, buf2, BLOCK_SIZE, BLOCK_SIZE);
    io_uring_sqe_set_data(sqe2, buf2);


    //submit
    ret = io_uring_submit(&ring);
    if (ret != QUEUE_DEPTH) {
        fprintf(stderr, "io_uring_submit error\n");
        exit(1);
    }


    while (count < 2) {

        ret = io_uring_wait_cqe(&ring, &cqe);
        if (ret != 0) {
            fprintf(stderr, "io_uring_wait_cqe error\n");
            exit(1);
        }

        count++;
        data = io_uring_cqe_get_data(cqe);
        printf("write %d bytes to file with buf %s\n", cqe->res, (char *) data);
        io_uring_cqe_seen(&ring, cqe);
    }

    io_uring_queue_exit(&ring);

    //free(buf1);
    //free(buf2);
    close(fd);

    return 0;
}
