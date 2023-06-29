#pragma once
#include <iostream>
#include <vector>
#include <string>
#include <fstream>

#define INCOME "收入"
#define EXPEND "支出"
#define FILENAME "AccountBook.txt"

/***********   通用功能性函数声明   ************/
//绘制菜单函数
	//显示主菜单
void showMainMenu();
	//显示记账菜单
void showAccountingMenu();
	//显示查询菜单
void showQueryMenu();


//读取键盘输入进行合法性校验

char readMenuSelection(int options);
char readQuitConfirm();
int readAmount();