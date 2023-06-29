#pragma once
#include "common.h"

//定义结构体类型
struct AccountItem {
	std::string itemType;
	int amount;
	std::string detail;
};

/********   针对账目数据进行操作的函数声明   ***********/
//加载账目数据
void loadDataFromFile(std::vector<AccountItem>& items);

//记账操作
void accounting(std::vector<AccountItem>& items);

void expend(std::vector<AccountItem>& items);
void income(std::vector<AccountItem>& items);

//将一条账目信息写入本地文件
void insertIntoFile(AccountItem& item);
//查询操作
void query(const std::vector<AccountItem>& items);
void queryItems(const std::vector<AccountItem>& items);
void queryItems(const std::vector<AccountItem>& items, const std::string itemType);

//输出
void printItem(AccountItem& item);