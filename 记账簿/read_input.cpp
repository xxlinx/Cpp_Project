#include "common.h"

//��ȡ����������кϷ���У��
char readMenuSelection(int options) {
	std::string str;
	while (true) {
		getline(std::cin, str);
		//���кϷ����ж�
		if (str.size() != 1 || str[0] - '0' <= 0 || str[0] - '0' > options) {
			std::cout << "���������������";
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
		//���кϷ����ж�
		if (str.size() != 1 || toupper(str[0]) != 'Y' && toupper(str[0]) != 'N') {  //�Ƚ����߼������㣬�ٽ����߼�������
			std::cout << "��������������� ��Y/N�� �����ִ�Сд";
		}
		else
			break;
	}
	return toupper(str[0]);

}