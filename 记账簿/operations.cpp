#include "common.h"
#include "account_item.h"


//加载账目数据
void loadDataFromFile(std::vector<AccountItem>& items) {
	std::ifstream input(FILENAME);

	//逐行读取每一条  类型  金额  备注
	AccountItem item;
	std::string line;

	while (input>>item.itemType>>item.amount>>item.detail) {
		items.push_back(item);
	}
	input.close();
}

//记账操作
void accounting(std::vector<AccountItem>& items) {
    //读取键盘选择，做合法性验证
    char key = readMenuSelection(3);

    switch (key) {
    case '1':   //1 --- 收入
        income(items);
        break;
    case '2':   //2  --- 支出
        expend(items);
        break;
    case '3':   //3  --- 返回主菜单
        break;
    default:
        break;
    }
}
void income(std::vector<AccountItem>& items) {
    //新构建一个AccountItem对象
    AccountItem item;
    item.itemType = INCOME;
    //交互输入
    std::cout << "\n本次收入金额：";
    item.amount = readAmount();
    std::cout << "\n备注：";
    getline(std::cin, item.detail);
    //添加到vector
    items.push_back(item);
    //写入文件做持久化保存
    insertIntoFile(item);

    //显示成功信息
    std::cout << "\n------------记账成功-----------" << std::endl;
    std::cout << "\n----按回车键返回主菜单----" << std::endl;
    //以免有乱输入的信息
    std::string line;
    getline(std::cin, line);
}

//将一条账目信息写入本地文件
void insertIntoFile(AccountItem& item) {
    //创建一个 ofstream 对象         以追加的方式进行写入
    std::ofstream output(FILENAME, std::ios::out | std::ios::app);
    output << item.itemType << "\t" << item.amount << "\t" << item.detail << std::endl;
    output.close();
}
//记录支出
void expend(std::vector<AccountItem>& items) {
    //新构建一个AccountItem对象
    AccountItem item;
    item.itemType = EXPEND;
    //交互输入
    std::cout << "\n本次支出金额：";
    item.amount = -readAmount();  //支出取负数
    std::cout << "\n备注：";
    getline(std::cin, item.detail);
    //添加到vector
    items.push_back(item);
    //写入文件做持久化保存
    insertIntoFile(item);

    //显示成功信息
    std::cout << "\n------------记账成功-----------" << std::endl;
    std::cout << "\n----按回车键返回主菜单----" << std::endl;
    //以免有乱输入的信息
    std::string line;
    getline(std::cin, line);
}


//查询操作
void query(const std::vector<AccountItem>& items) {
    //读取键盘选择，做合法性验证
    char key = readMenuSelection(4);

    switch (key) {
    case '1':   //1 --- 遍历所以
        queryItems(items);
        break;
    case '2':   //2  --- 遍历收入
        queryItems(items, INCOME);
        break;
    case '3':   //3  --- 遍历支出
        queryItems(items, EXPEND);
        break;
    case '4':   //4  --- 退出
        break;
    default:
        break;
    }
}


//读取输入的金额数，并作合法性校验
int readAmount() {
    int amount;
    std::string str;
    while (true) {
        getline(std::cin, str);
        //进行合法性判断    异常处理
        try {
            amount = stoi(str);
            break;
        }
        catch (std::invalid_argument e) {
            std::cout << "输入错误，重新输入金额数字： ";
        }
    }
    return amount;
}
//查询账目
void queryItems(const std::vector<AccountItem>& items) {
    std::cout << "--------查询所有账目结果------------" << std::endl;
    std::cout << "\n类型\t\t金额\t\t备注\n" << std::endl;

    //统计
    int total = 0;
    for (auto item : items) {
        printItem(item);
        total += item.amount;
    }
    //输出信息
    std::cout << "----------------------------" << std::endl;
    std::cout << "总收支:" << total << std::endl;
    std::cout << "\n----按回车键返回主菜单----" << std::endl;
    //以免有乱输入的信息
    std::string line;
    getline(std::cin, line);
}

//查询收入  支出   重载函数
void queryItems(const std::vector<AccountItem>& items, const std::string itemType) {
    std::cout << "--------查询总收入账目结果------------" << std::endl;
    std::cout << "\n类型\t\t金额\t\t备注\n" << std::endl;

    //统计
    int total = 0;
    for (auto item : items) {
        if (item.itemType == itemType) {
            printItem(item);
            total += item.amount;
        }   
    }
    //输出信息
    std::cout << "----------------------------" << std::endl;
    std::cout << ((itemType == INCOME)?"总收入:" : "总支出:") << total << std::endl;
    std::cout << "\n----按回车键返回主菜单----" << std::endl;
    //以免有乱输入的信息
    std::string line;
    getline(std::cin, line);
}


void printItem(AccountItem& item) {
    std::cout << item.itemType << "\t" << item.amount << "\t" << item.detail << std::endl;
}