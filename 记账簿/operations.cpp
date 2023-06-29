#include "common.h"
#include "account_item.h"


//������Ŀ����
void loadDataFromFile(std::vector<AccountItem>& items) {
	std::ifstream input(FILENAME);

	//���ж�ȡÿһ��  ����  ���  ��ע
	AccountItem item;
	std::string line;

	while (input>>item.itemType>>item.amount>>item.detail) {
		items.push_back(item);
	}
	input.close();
}

//���˲���
void accounting(std::vector<AccountItem>& items) {
    //��ȡ����ѡ�����Ϸ�����֤
    char key = readMenuSelection(3);

    switch (key) {
    case '1':   //1 --- ����
        income(items);
        break;
    case '2':   //2  --- ֧��
        expend(items);
        break;
    case '3':   //3  --- �������˵�
        break;
    default:
        break;
    }
}
void income(std::vector<AccountItem>& items) {
    //�¹���һ��AccountItem����
    AccountItem item;
    item.itemType = INCOME;
    //��������
    std::cout << "\n���������";
    item.amount = readAmount();
    std::cout << "\n��ע��";
    getline(std::cin, item.detail);
    //��ӵ�vector
    items.push_back(item);
    //д���ļ����־û�����
    insertIntoFile(item);

    //��ʾ�ɹ���Ϣ
    std::cout << "\n------------���˳ɹ�-----------" << std::endl;
    std::cout << "\n----���س����������˵�----" << std::endl;
    //���������������Ϣ
    std::string line;
    getline(std::cin, line);
}

//��һ����Ŀ��Ϣд�뱾���ļ�
void insertIntoFile(AccountItem& item) {
    //����һ�� ofstream ����         ��׷�ӵķ�ʽ����д��
    std::ofstream output(FILENAME, std::ios::out | std::ios::app);
    output << item.itemType << "\t" << item.amount << "\t" << item.detail << std::endl;
    output.close();
}
//��¼֧��
void expend(std::vector<AccountItem>& items) {
    //�¹���һ��AccountItem����
    AccountItem item;
    item.itemType = EXPEND;
    //��������
    std::cout << "\n����֧����";
    item.amount = -readAmount();  //֧��ȡ����
    std::cout << "\n��ע��";
    getline(std::cin, item.detail);
    //��ӵ�vector
    items.push_back(item);
    //д���ļ����־û�����
    insertIntoFile(item);

    //��ʾ�ɹ���Ϣ
    std::cout << "\n------------���˳ɹ�-----------" << std::endl;
    std::cout << "\n----���س����������˵�----" << std::endl;
    //���������������Ϣ
    std::string line;
    getline(std::cin, line);
}


//��ѯ����
void query(const std::vector<AccountItem>& items) {
    //��ȡ����ѡ�����Ϸ�����֤
    char key = readMenuSelection(4);

    switch (key) {
    case '1':   //1 --- ��������
        queryItems(items);
        break;
    case '2':   //2  --- ��������
        queryItems(items, INCOME);
        break;
    case '3':   //3  --- ����֧��
        queryItems(items, EXPEND);
        break;
    case '4':   //4  --- �˳�
        break;
    default:
        break;
    }
}


//��ȡ����Ľ�����������Ϸ���У��
int readAmount() {
    int amount;
    std::string str;
    while (true) {
        getline(std::cin, str);
        //���кϷ����ж�    �쳣����
        try {
            amount = stoi(str);
            break;
        }
        catch (std::invalid_argument e) {
            std::cout << "��������������������֣� ";
        }
    }
    return amount;
}
//��ѯ��Ŀ
void queryItems(const std::vector<AccountItem>& items) {
    std::cout << "--------��ѯ������Ŀ���------------" << std::endl;
    std::cout << "\n����\t\t���\t\t��ע\n" << std::endl;

    //ͳ��
    int total = 0;
    for (auto item : items) {
        printItem(item);
        total += item.amount;
    }
    //�����Ϣ
    std::cout << "----------------------------" << std::endl;
    std::cout << "����֧:" << total << std::endl;
    std::cout << "\n----���س����������˵�----" << std::endl;
    //���������������Ϣ
    std::string line;
    getline(std::cin, line);
}

//��ѯ����  ֧��   ���غ���
void queryItems(const std::vector<AccountItem>& items, const std::string itemType) {
    std::cout << "--------��ѯ��������Ŀ���------------" << std::endl;
    std::cout << "\n����\t\t���\t\t��ע\n" << std::endl;

    //ͳ��
    int total = 0;
    for (auto item : items) {
        if (item.itemType == itemType) {
            printItem(item);
            total += item.amount;
        }   
    }
    //�����Ϣ
    std::cout << "----------------------------" << std::endl;
    std::cout << ((itemType == INCOME)?"������:" : "��֧��:") << total << std::endl;
    std::cout << "\n----���س����������˵�----" << std::endl;
    //���������������Ϣ
    std::string line;
    getline(std::cin, line);
}


void printItem(AccountItem& item) {
    std::cout << item.itemType << "\t" << item.amount << "\t" << item.detail << std::endl;
}