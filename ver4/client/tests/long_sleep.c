#include <stdio.h>
#include <unistd.h>

int main() {
    sleep(15);
	for(int i = 1; i <= 10; i++) {
		printf("%d ", i);
	}
	return 0;
}

