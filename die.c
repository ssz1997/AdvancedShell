#include <stdio.h>
#include <unistd.h>  
#include <assert.h>
int main() {

        printf("foo\n");
        assert(close(STDOUT_FILENO) == 0);
        printf("bar\n");
}
