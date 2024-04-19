#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define LOG_INFO "INFO"
#define LOG_WARN "WARN"
#define LOG_ERROR "ERROR"
#define LOG_DEBUG "DEBUG"

#define LOGFILE "log.txt"

void write_log(const char* level, const char* message) {
    FILE* fp = fopen(LOGFILE, "a");
    if (fp == NULL) {
        perror("Error opening log file");
        exit(1);
    }
    time_t now;
    time(&now);

    struct tm *local_time = localtime(&now);
    char time[20]; 
    strftime(time, sizeof(time), "%Y%m%d%H%M%S", local_time);

    fprintf(fp, "%s [%s]: %s\n", time, level, message);
    fclose(fp);
}