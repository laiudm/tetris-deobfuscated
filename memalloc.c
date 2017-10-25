#include <stdio.h>


struct image {
	int a;
	int b;
};

struct image getImage(void) {
	struct image Image = {};
	Image.a = 1;
	Image.b = 2;
	return (Image);
}

int main(int argc, char **argv) {
	printf("Hello world\n");
	struct image i = getImage();
	int j = i.a;
	printf("j is %i\n", j);
	
}