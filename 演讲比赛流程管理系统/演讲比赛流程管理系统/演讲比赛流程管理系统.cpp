#define _CRT_SECURE_NO_WARNINGS 1
#include"speechManager.h"



int main()
{
	srand((unsigned int)time(NULL));
	SpeechManager sm;

	int choice = 0;
	
	//for (map<int, Speaker>::iterator i = sm.m_Speaker.begin(); i != sm.m_Speaker.end(); i++)
	//{
	//	cout << i->first << " " << i->second.m_Name << " " << i->second.m_Score[0] << endl;
	//}

	while (1)
	{

		sm.show_Menu();
		cout << "���������ѡ��" << endl;
		cin >> choice;
		switch (choice)
		{
		case 1://��ʼ����
			sm.startSpeech();
			break;
		case 2://�鿴���������¼
			sm.showRecord();
			break;
		case 3://��ձ�����¼
			sm.clearRecord();
			break;
		case 0://�˳�ϵͳ
			sm.exitSystem();
			break;
		default:
			system("cls");
			break;
		}
	}



	system("pause");
	return 0;
}