#include "common.h"

/*********    绘制菜单函数     *********/
//显示主菜单
void showMainMenu() {

	system("cls");

	std::cout << "---------------------------------------" << std::endl;
	std::cout << "|=======     欢迎使用记账簿     =======|" << std::endl;
	std::cout << "|                                     |" << std::endl;
	std::cout << "|=======        1.记账         =======|" << std::endl;
	std::cout << "|=======        2.查询         =======|" << std::endl;
	std::cout << "|=======        3.退出         =======|" << std::endl;
	std::cout << "|-------------------------------------|" << std::endl;
	std::cout << "\n请选择(1-3): ";
}

//显示记账菜单
void showAccountingMenu() {

	system("cls");

	std::cout << "---------------------------------------" << std::endl;
	std::cout << "|=======     选择记账种类       =======|" << std::endl;
	std::cout << "|                                     |" << std::endl;
	std::cout << "|=======        1.收入         =======|" << std::endl;
	std::cout << "|=======        2.支出         =======|" << std::endl;
	std::cout << "|=======      3.返回主菜单      =======|" << std::endl;
	std::cout << "|-------------------------------------|" << std::endl;
	std::cout << "\n请选择(1-3): ";
}

//显示查询菜单
void showQueryMenu() {

	system("cls");

	std::cout << "---------------------------------------" << std::endl;
	std::cout << "|=======     选择查询条件       =======|" << std::endl;
	std::cout << "|                                     |" << std::endl;
	std::cout << "|=======     1.统计所有账目     =======|" << std::endl;
	std::cout << "|=======     2.统计收入         =======|" << std::endl;
	std::cout << "|=======     3.统计支出         =======|" << std::endl;
	std::cout << "|=======     4.返回主菜单       =======|" << std::endl;
	std::cout << "|-------------------------------------|" << std::endl;
	std::cout << "\n请选择(1-4): ";
}
