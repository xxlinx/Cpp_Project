#pragma once
#include <iostream>
#include <vector>
#include <string>
#include <fstream>

#define INCOME "����"
#define EXPEND "֧��"
#define FILENAME "AccountBook.txt"

/***********   ͨ�ù����Ժ�������   ************/
//���Ʋ˵�����
	//��ʾ���˵�
void showMainMenu();
	//��ʾ���˲˵�
void showAccountingMenu();
	//��ʾ��ѯ�˵�
void showQueryMenu();


//��ȡ����������кϷ���У��

char readMenuSelection(int options);
char readQuitConfirm();
int readAmount();