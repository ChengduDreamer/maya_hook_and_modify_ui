#include <iostream>
#include <Windows.h>

int main()
{
    MessageBoxA(NULL, "Orgin MessageBox", "tip", NULL);	//ע��ǰ����
    Sleep(10000);	//��Sleepһ��
    MessageBoxA(NULL, "Orgin MessageBox", "tip", NULL);	//ע������
    return 0;
}
