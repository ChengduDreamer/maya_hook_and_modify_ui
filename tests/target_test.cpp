#include <iostream>
#include <Windows.h>

int main()
{
    MessageBoxA(NULL, "Orgin MessageBox", "tip", NULL);	//注入前调用
    Sleep(10000);	//简单Sleep一下
    MessageBoxA(NULL, "Orgin MessageBox", "tip", NULL);	//注入后调用
    return 0;
}
