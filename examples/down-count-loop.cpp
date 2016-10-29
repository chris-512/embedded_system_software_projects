int func1()
{
	int a = 1, b =2;
	int temp;
	for(int i = 11; i; i--) {
		temp = a;
		a = b;
		b = temp;
	}

}
int func2()
{
	int a = 1, b =2;
	int temp;
	for(int i = 0; i < 11; i++) {
		temp = a;
		a = b;
		b = temp;
	}
}
int main(void)
{
}
