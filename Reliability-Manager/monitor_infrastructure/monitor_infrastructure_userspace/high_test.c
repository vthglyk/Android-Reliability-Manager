#include <stdio.h>

int main(int argc, char *argv[]) {
	int t = 0;
	long k = 200000000;

        printf("START:test intensive\n");

        while(1) {
                t++;
                if (t > k) {
                        t = 0;
                        printf("Hello from test\n");
                        sleep(1);
                }
        }

	return 0;
}
