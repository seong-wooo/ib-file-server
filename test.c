#include <stdio.h>
#include <time.h>
#include <sys/time.h>
#include <unistd.h>
#include "test.h"

struct timeval  tv;
double begin, end;

void start_test() {
    gettimeofday(&tv, NULL);
	begin = (tv.tv_sec) * 1000 + (tv.tv_usec) / 1000 ;
}

void end_test() {
    gettimeofday(&tv, NULL);
	end = (tv.tv_sec) * 1000 + (tv.tv_usec) / 1000 ;
    printf("Execution time %f\n", (end - begin) / 1000);
}

void test_func(void *func(void *), void *data, char *msg) {
    printf("%s 테스트 시작 \n", msg);
    start_test();
    func(data);
    end_test();
}