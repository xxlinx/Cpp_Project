#include "common.h"

//读取键盘输入进行合法性校验
char readMenuSelection(int options) {
	std::string str;
	while (true) {
		getline(std::cin, str);
		//进行合法性判断
		if (str.size() != 1 || str[0] - '0' <= 0 || str[0] - '0' > options) {
			std::cout << "输入错误，重新输入";
		}
		else
			break;
	}
	return str[0];
}
char readQuitConfirm() {

	std::string str;
	while (true) {
		getline(std::cin, str);
		//进行合法性判断
		if (str.size() != 1 || toupper(str[0]) != 'Y' && toupper(str[0]) != 'N') {  //先进行逻辑与运算，再进行逻辑或运算
			std::cout << "输入错误，重新输入 （Y/N） 不区分大小写";
		}
		else
			break;
	}
	return toupper(str[0]);

}