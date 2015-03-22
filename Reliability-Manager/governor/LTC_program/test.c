#include  <stdio.h>// printf
#include  <stdlib.h>//for exit

int main(int argc, char **argv)
{
        int i = 1;
        i+=2;
	long double e =  2.71828182845904523536028747L;
	e /= i;
        printf("Hello, world (i=%d)! and %Le\n", i, e);

        exit(0);
}
