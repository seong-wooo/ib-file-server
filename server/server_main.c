
#include "ib_server.h"
#include "server.h"

int main(int argc, char const *argv[]) {
    if (*argv[1] == 'i') {
        ib_server();
    }
}