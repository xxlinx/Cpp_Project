#pragma once
#include "common.h"

//����ṹ������
struct AccountItem {
	std::string itemType;
	int amount;
	std::string detail;
};

/********   �����Ŀ���ݽ��в����ĺ�������   ***********/
//������Ŀ����
void loadDataFromFile(std::vector<AccountItem>& items);

//���˲���
void accounting(std::vector<AccountItem>& items);

void expend(std::vector<AccountItem>& items);
void income(std::vector<AccountItem>& items);

//��һ����Ŀ��Ϣд�뱾���ļ�
void insertIntoFile(AccountItem& item);
//��ѯ����
void query(const std::vector<AccountItem>& items);
void queryItems(const std::vector<AccountItem>& items);
void queryItems(const std::vector<AccountItem>& items, const std::string itemType);

//���
void printItem(AccountItem& item);