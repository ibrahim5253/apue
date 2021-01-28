#include <sys/stat.h>

#include <errno.h>
#include <fcntl.h>
#include <libgen.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define BUFFSIZE 512

static void usage(void);
int main(int, char**);

int
main(int argc, char** argv)
{
    setprogname(argv[0]);
    if (argc != 3) {
        usage();
        return EXIT_FAILURE;
    }

    char *source = argv[1];
    char *target = argv[2];

    struct stat sb_source;
    if (stat(source, &sb_source) < 0) {
        perror("stat");
        return EXIT_FAILURE;
    } else if (S_ISDIR(sb_source.st_mode)) {
        fprintf(stderr, "%s: %s is a directory.",
                getprogname(), source);
        return EXIT_FAILURE;
    }

    struct stat sb_target;
    int isdir = 0;
    if (stat(target, &sb_target) < 0) {
        if (errno != ENOENT) {
            perror("stat");
            return EXIT_FAILURE;
        }
    } else if (sb_source.st_ino == sb_target.st_ino) {
        fprintf(stderr, "%s: %s and %s are the same.\n",
                getprogname(), source, target);
        return EXIT_FAILURE;
    } else if (S_ISDIR(sb_target.st_mode)) {
        isdir = 1;
        struct stat sb;
        if (stat(dirname(source), &sb) < 0) {
            perror("stat");
            return EXIT_FAILURE;
        }
        if (sb.st_ino == sb_target.st_ino) {
            fprintf(stderr, "%s: %s and %s/%s are the same.\n",
                    getprogname(), source, target, basename(source));
            return EXIT_FAILURE;
        }
    }

    int fds;
    if ((fds = open(source, O_RDONLY)) < 0) {
        perror("open");
        return EXIT_FAILURE;
    }

    int fdt;
    if (isdir) {
        int fdd;
        if ((fdd = open(target, O_RDONLY | O_DIRECTORY)) < 0) {
            perror("open");
            (void)close(fds);
            return EXIT_FAILURE;
        }
        if ((fdt = openat(fdd, basename(source),
                O_WRONLY | O_CREAT | O_TRUNC, sb_source.st_mode)) < 0)
        {
            perror("open");
            (void)close(fdd);
            (void)close(fds);
            return EXIT_FAILURE;
        }
        (void)close(fdd);
    } else {
        if ((fdt = open(target, O_WRONLY | O_CREAT | O_TRUNC,
                        sb_source.st_mode)) < 0)
        {
            perror("open");
            (void)close(fds);
            return EXIT_FAILURE;
        }
    }

    char buf[BUFFSIZE];
    ssize_t nbytes;
    while ((nbytes = read(fds, buf, BUFFSIZE)) > 0) {
        if (write(fdt, buf, nbytes) != nbytes) {
            perror("write");
            (void)close(fdt);
            (void)close(fds);
            return EXIT_FAILURE;
        }
    }
    if (nbytes < 0) {
        perror("read");
        (void)close(fdt);
        (void)close(fds);
        return EXIT_FAILURE;
    }

    (void)close(fdt);
    (void)close(fds);

    return EXIT_SUCCESS;
}

static void
usage(void)
{
    fprintf(stderr, "usage: %s source target\n", getprogname());
}
