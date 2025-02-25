// Dakota Winslow 2025
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

#define DEVICE_PATH "/dev/mytimer"
#define BUFFER_SIZE 128

// have to track this here AND in the kernel, since despite the insistance that
// the kernel module must do all the work, we are required to print error messages
// from the userspace program.
int g_num_timers;


/**
 * Every program should have a good help message.
 */
void print_help() {
    printf("Usage: ktimer [options]\n");
    printf("Options:\n");
    printf("  -l                Read and print the contents of /dev/timer and quit\n");
    printf("  -s [sec] [msg]    Set a new timer of length [sec] with message [msg] and quit\n");
    printf("  -m [count]        Set the kernel timer count to [count] and quit\n");
    printf("  -h, --help        Print this help message and quit\n");
    printf("  -v, --version     Print version and copyright information\n");
}

void print_version() {
    printf("ktimer version 1.0\n");
    printf("2025 Dakota Winslow\n");
}

int main(int argc, char *argv[]) {
    int fd;
    char buffer[BUFFER_SIZE];
    ssize_t bytes_read, bytes_written;
    char has_room;
    int max_timers;

    // There is no valid command without an argument
    if (argc < 2) {
        print_help();
        return EXIT_FAILURE;
    }

    // Help Message
    if (strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0) {
        print_help();
        return EXIT_SUCCESS;
    }

    // Version Message
    if (strcmp(argv[1], "-v") == 0 || strcmp(argv[1], "--version") == 0) {
        print_version();
        return EXIT_SUCCESS;
    }

    // All options beyond this point require the device to be open
    fd = open(DEVICE_PATH, O_RDWR);
    if (fd < 0) {
        perror("Failed to open device");
        return EXIT_FAILURE;
    }

    // List option just prints the contents of the device
    if (strcmp(argv[1], "-l") == 0) {
        bytes_read = read(fd, buffer, BUFFER_SIZE - 1);
        if (bytes_read < 0) {
            perror("Failed to read from device");
            close(fd);
            return EXIT_FAILURE;
        }
        buffer[bytes_read] = '\0';

        // pop the second to last character off the buffer, which is the capacity status
        has_room = buffer[bytes_read - 2];
        buffer[bytes_read - 2] = '\n';
        buffer[bytes_read - 1] = '\0';

        printf("%s", buffer);

        // Set option requires two additional arguments, [sec] and [msg]
    } else if (strcmp(argv[1], "-s") == 0 && argc == 4) {
        // Read current timers
        bytes_read = read(fd, buffer, BUFFER_SIZE - 1);

        // pop the second to last character off the buffer, which is the capacity status
        has_room = buffer[bytes_read - 2];
        buffer[bytes_read - 2] = '\n';
        buffer[bytes_read - 1] = '\0';

        if (bytes_read < 0) {
            perror("Failed to read from device");
            close(fd);
            return EXIT_FAILURE;
        }
        buffer[bytes_read] = '\0';

        // Check if the message already exists
        if (strstr(buffer, argv[3]) != NULL) {
            // If it does, give it an update
            snprintf(buffer, BUFFER_SIZE, "%s %s", argv[2], argv[3]);
            bytes_written = write(fd, buffer, strlen(buffer));
            if (bytes_written < 0) {
                perror("Failed to write to device");
                close(fd);
                return EXIT_FAILURE;
            }
            printf("The timer %s was updated!\n", argv[3]);
        }


        // Check if there is room for a new timer
        if (has_room == '0') {
            // Count the number of lines in the buffer; this is our max!
            max_timers = 0;
            for (int i = 0; i < bytes_read; i++) {
                if (buffer[i] == '\n') {
                    max_timers++;
                }
            }
            printf("%d timer(s) already exist(s)!\n", max_timers);
            close(fd);
            return EXIT_FAILURE;
        }

        // Set new timer
        snprintf(buffer, BUFFER_SIZE, "%s %s", argv[2], argv[3]);
        bytes_written = write(fd, buffer, strlen(buffer));
        if (bytes_written < 0) {
            perror("Failed to write to device");
            close(fd);
            return EXIT_FAILURE;
        }
        // Set the kernel timer count
        // The kernel support further definition of '^'-led control strings
    } else if (strcmp(argv[1], "-m") == 0 && argc == 3) {
        snprintf(buffer, BUFFER_SIZE, "^COUNT=%s", argv[2]);
        bytes_written = write(fd, buffer, strlen(buffer));
        if (bytes_written < 0) {
            perror("Failed to write to device");
            close(fd);
            return EXIT_FAILURE;
        }

        // Users be users
    } else {
        print_help();
        close(fd);
        return EXIT_FAILURE;
    }

    close(fd);
    return EXIT_SUCCESS;
}
