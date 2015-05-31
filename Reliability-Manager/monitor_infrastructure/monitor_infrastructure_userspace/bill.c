#include <stdio.h>

int main(int argc, char *argv[]) {
	int t = 0;
	long k = 200000000;

        printf("START:test intensive\n");

        while(1) {
                t++;
                if (t > atoi(argv[1])) {
                        t = 0;
                        printf("Hello from test k = %d and sleep = %d\n", atoi(argv[1]), atoi(argv[2]));
                        sleep(atoi(argv[2]));
                }
        }

	return 0;
}
