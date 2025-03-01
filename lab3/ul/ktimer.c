// devinkt winslowd 2025
#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <fcntl.h>
#include <unistd.h>

#define KERN_BUF 150
#define MSG_LEN 128

void sighandler(int);

char to_kernel[KERN_BUF];
char timer_msg[MSG_LEN];

/**
 * Every program should have a good help message.
 */
void print_help() {
    printf("Usage: ktimer [options]\n");
    printf("Options:\n");
    printf("  -l                Read and print the contents of /dev/timer and quit\n");
    printf("  -s [sec] [msg]    Set a new timer of length [sec] with message [msg] and quit\n");
    printf("  -m [count]        Set the kernel timer count to [count] and quit (1 or 2 only)\n");
    printf("  -r                Remove all timers and quit\n");
    printf("  -h, --help        Print this help message and quit\n");
    printf("  -v, --version     Print version and copyright information\n");
}

void print_version() {
    printf("ktimer version 1.1\n");
}

int main(int argc, char **argv) {
    int pFile, oflags;
    struct sigaction action;

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

    // Opens to device file
    pFile = open("/dev/mytimer", O_RDWR);
    if (pFile < 0) {
        fprintf (stderr, "mytimer module isn't loaded\n");
        return 1;
    }

    // Setup signal handler
    memset(&action, 0, sizeof(action));
    action.sa_handler = sighandler;
    action.sa_flags = SA_SIGINFO;
    sigemptyset(&action.sa_mask);
    sigaction(SIGIO, &action, NULL);
    fcntl(pFile, F_SETOWN, getpid());
    oflags = fcntl(pFile, F_GETFL);
    fcntl(pFile, F_SETFL, oflags | FASYNC);

    // Write to file.
    memset(to_kernel, 0, KERN_BUF);
    snprintf(to_kernel, KERN_BUF, "%s %s %s", argv[1], argv[2], argv[3]);
    snprintf(timer_msg, MSG_LEN, "%s", argv[3]);
    // printf("to kernel %s, timer msg %s, argv1 %s, argv2 %s, argv3 %s\n", to_kernel, timer_msg, argv[1], argv[2], argv[3]);

    if(argc == 2 && (strcmp(argv[1], "-l") == 0)) // list timer(s) expiration message(s) and time(s)
    {
        // ssize_t bytes_written = write(pFile, to_kernel, strlen(to_kernel) + 1);
        // printf("DEBUG: user_input before write: [%s]\n", to_kernel);
        // read the device and store in a buffer
        char buffer[KERN_BUF];
        ssize_t bytes_read = read(pFile, buffer, KERN_BUF - 1);
        if (bytes_read < 0) {
            perror("Failed to read from device");
            close(pFile);
            return EXIT_FAILURE;
        }
        buffer[bytes_read - 4] = '\0';
        printf("%s", buffer);
    }

    else if(argc == 2 && (strcmp(argv[1], "-r") == 0)) // remove all timers
    {
        ssize_t bytes_written = write(pFile, to_kernel, strlen(to_kernel) + 1);
        return 0;
    }

    else if(argc == 3 && (strcmp(argv[1], "-m") == 0)) // set the kernel timer count to [count]
    {
        if (argv[2][0] == '2' || argv[2][0] == '1') {
            ssize_t bytes_written = write(pFile, to_kernel, strlen(to_kernel) + 1);
            return 0;
        } else {
            printf("Only 1 or 2 timers allowed!\n");
            return 0;
        }


    }

    else if(argc == 4 && (strcmp(argv[1], "-s") == 0)) // add a new timer with expiration in argv[2] and message in argv[3]
    {
        // Check if timer with message already exists
        // read the device and store in a buffer
        char buffer[KERN_BUF];
        ssize_t bytes_read = read(pFile, buffer, KERN_BUF - 1);
        if (bytes_read < 0) {
            perror("Failed to read from device");
            close(pFile);
            return EXIT_FAILURE;
        }
        buffer[bytes_read] = '\0';

        // iterate through the list of timers and check if the message already exists
        int i =0;
        char token_msg[129];
        unsigned long expiration;
        int allowed_timers;
        int active_timers;

        // message ends with "...\n N N"
        //Re-terminate the string, cutting off the trailer data
        int cutoff_pos = strlen(buffer) - 4;
        char *cutoff = buffer + cutoff_pos;
        *cutoff = '\0';
        // parse the cutoff data by measuring offset from '0'
        // poor man's char to int conversion
        active_timers  = (char)cutoff[1] - '0';
        allowed_timers = (char)cutoff[3] - '0';

        //split the buffer into tokens on '/n'
        char *token = strtok(buffer, "\n");
        while (token != NULL) {
            // printf("DEBUG: active_timers: %d\n", active_timers);
            // find last instance of space
            char *last_space = strrchr(token, ' ');
            // replace the last space with a null terminator
            *last_space = '\0';

            // printf("DEBUG: token: [%s], argument: [%s]\n", token, argv[3]);

            if (strcmp(token, argv[3]) == 0) {
                // printf("DEBUG: token_msg: [%s], argv[3]: [%s]\n", token_msg, argv[3]);
                snprintf(to_kernel, sizeof(to_kernel), "-s %s %s", argv[2], argv[3]);
                ssize_t bytes_written = write(pFile, to_kernel, strlen(to_kernel) + 1); //access timer_write kernel function with string indicating how to setup new timer
                printf("The timer %s was updated!\n", argv[3]);
                close(pFile);
                return 0;
            }
            token = strtok(NULL, "\n");
        }

        // check if we have too many timers
        if(active_timers >= allowed_timers) {
            printf("%d timer(s) already exist(s)!\n", active_timers);
            return 0;
        }

        snprintf(to_kernel, sizeof(to_kernel), "-s %s %s", argv[2], argv[3]);
        // read(pFile, )
        // printf("DEBUG: user_input before write: [%s]\n", to_kernel);
        ssize_t bytes_written = write(pFile, to_kernel, strlen(to_kernel) + 1); //access timer_write kernel function with string indicating how to setup new timer
        pause();
    }


    // printf("Closing out!\n");
    // Closes.
    close(pFile);
    return 0;

}

// SIGIO handler
void sighandler(int signo)
{
    printf("%s\n", timer_msg);
    // exit(0);
}