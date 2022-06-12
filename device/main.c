#include <stdint.h>
#include <stdlib.h>

void xargon_main (int argc, char *argv[]);

void main() {
    char *argv[] = { "xargon", NULL };

    xargon_main(1, argv);
}
