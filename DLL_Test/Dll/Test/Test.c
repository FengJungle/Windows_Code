#include <stdio.h>
#include <Windows.h>
//#include "MyDll.h"

typedef int(*p_sum)(int a, int b);
typedef int(*p_sub)(int a, int b);
typedef int(*p_mul)(int a, int b);


int main()
{
	p_sum f_sum = NULL;
	p_sub f_sub = NULL;;
	p_mul f_mul = NULL;;
	HINSTANCE hHandle = LoadLibrary("MathSample.dll");
	if (INVALID_HANDLE_VALUE == hHandle) {
		printf("Invalid handle\n");
		goto exit;
	}
	f_sum = (p_sum)GetProcAddress(hHandle, "sum");
	f_sub = (p_sub)GetProcAddress(hHandle, "sub");
	f_mul = (p_mul)GetProcAddress(hHandle, "mul");
	if (NULL == f_sum || NULL == f_sub || NULL == f_mul) {
		printf("Invalid parameters!\n");
		goto exit;
	}
	int a = 0;
	int b = 0;
	while (1) {
		printf("Input a and b:\n");
		scanf("%d %d", &a, &b);
		/*printf("%d + %d = %d \n", a, b, sum(a, b));
		printf("%d - %d = %d \n", a, b, sub(a, b));
		printf("%d * %d = %d \n", a, b, mul(a, b));*/
		printf("%d + %d = %d \n", a, b, f_sum(a, b));
		printf("%d - %d = %d \n", a, b, f_sub(a, b));
		printf("%d * %d = %d \n", a, b, f_mul(a, b));
		system("pause");
	}

exit:
	system("pause");
	return 0;
}