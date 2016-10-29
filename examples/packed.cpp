#include <iostream>

using namespace std;

typedef struct {
	short a[3];
	int k;
	char uu;
	int b;
} __attribute__((packed)) pack2;

typedef struct {
	short a;
	char b;
	short c;
	int d;
} pack1;

int main(void)
{
	cout << sizeof(pack2) << endl;
}
