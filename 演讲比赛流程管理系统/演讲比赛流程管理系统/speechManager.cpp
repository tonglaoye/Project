#define _CRT_SECURE_NO_WARNINGS 1
#include"speechManager.h"

SpeechManager::SpeechManager()
{
	this->initSpeech();

	this->createSpeaker();

	this->loadRecord();
}

SpeechManager::~SpeechManager()
{

}


void SpeechManager::show_Menu()
{
	cout << "********************************************" << endl;
	cout << "***************��ӭ�μ��ݽ�����*************" << endl;
	cout << "***************1����ʼ�ݽ�����**************" << endl;
	cout << "***************2���鿴�����¼**************" << endl;
	cout << "***************3����ձ�����¼**************" << endl;
	cout << "***************0���˳�����ϵͳ**************" << endl;
	cout << "********************************************" << endl;
	cout << endl;
}


void SpeechManager ::exitSystem()
{
	cout << " ��ӭ�´�ʹ�ã�" << endl;
	system("pause");
	exit(0);
}


void SpeechManager::initSpeech()
{
	this->v1.clear();
	this->v2.clear();
	this->vVictory.clear();
	this->m_Speaker.clear();
	this->m_Index = 1;
	this->m_Record.clear();
}

void SpeechManager::createSpeaker()
{
	string nameSeed = "ABCDEFGHIJKL";
	for (int i = 0; i < nameSeed.size(); i++)
	{
		string name = "ѡ��";
		name += nameSeed[i];
		Speaker sp;
		sp.m_Name = name;
		for (int j = 0; j < 2; j++)
		{
			sp.m_Score[j] = 0;
		}

		this->v1.push_back(i + 10001);
		this->m_Speaker.insert(make_pair(i + 10001, sp));
	}
}


void SpeechManager::startSpeech()
{
	//��һ�ֱ���
	//��ǩ
	this->speechDrew();

	//���� 
	this->speechContest();

	//��ʾ�������
	this->showScore();

	//�ڶ��ֱ���
	this->m_Index++;
	//��ǩ
	this->speechDrew();

	//����
	this->speechContest();

	//��ʾ���ս��
	this->showScore();

	// �������
	this->saveScore();
	//��ֵ����
	this->initSpeech();

	this->createSpeaker();

	this->loadRecord();

	cout << "���������ϣ�����" << endl;
	system("pause");
	system("cls");
}

void SpeechManager::speechDrew()
{
	cout << ">>��" << this->m_Index << "�ֱ���ѡ�����ڳ�ǩ��" << endl;
	cout << "--------------------------" << endl;
	cout << "��ǩ����ݽ�˳�����£�" << endl;
	if (this->m_Index == 1)
	{
		random_shuffle(this->v1.begin(), this->v1.end());
		for (vector<int>::iterator i = v1.begin(); i != v1.end(); i++)
		{
			cout << *i << " ";
		}
		cout << endl;
	}
	else
	{
		random_shuffle(this->v2.begin(), this->v2.end());
		for (vector<int>::iterator i = v2.begin(); i != v2.end(); i++)
		{
			cout << *i << " ";
		}
		cout << endl;
	}
	cout << "--------------------------" << endl;
	system("pause");
	cout << endl;
}



void SpeechManager::speechContest()
{
	cout << ">>��" << this->m_Index << "�ֱ�����ʽ��ʼ" << endl;

	multimap<double, int, greater<double>> groupScore;
	int num = 0;

	vector<int>v_Src;
	if (this->m_Index == 1)
	{
		v_Src = v1;
	}
	else
	{
		v_Src = v2;
	}

	for (vector<int>::iterator i = v_Src.begin(); i != v_Src.end(); i++)
	{
		num++;
		deque<double>d;
		for (int i = 0; i < 10; i++)
		{
			double score = (rand() % 401 + 600) / 10.f;
			//cout << score << " ";
			d.push_back(score);
		}
		//cout << endl;
		sort(d.begin(), d.end(), greater<double>());
		d.pop_front();
		d.pop_back();
		double sum = accumulate(d.begin(), d.end(), 0.0f);
		double avg = sum / (double)d.size();

		//cout << *i << this->m_Speaker[*i].m_Name << " " << avg << endl;

		this->m_Speaker[*i].m_Score[this->m_Index - 1] = avg;

		groupScore.insert(make_pair(avg, *i));

		if (num % 6 == 0)
		{
			cout << "��" << num / 6 << "С���������: " << endl;
			for (multimap<double, int, greater<double>>::iterator i = groupScore.begin(); i != groupScore.end(); i++)
			{
				cout << "��ţ�" << i->second << " ������" << this->m_Speaker[i->second].m_Name << " �ɼ���" << this->m_Speaker[i->second].m_Score[this->m_Index-1] << endl;

			}
			int count = 0;
			for (multimap<double, int, greater<double>>::iterator i = groupScore.begin(); i != groupScore.end() && count < 3 ; count++,i++)
			{
				if (this->m_Index == 1)
				{
					v2.push_back((*i).second);
				}
				else
				{
					vVictory.push_back((*i).second);
				}
			}

			groupScore.clear();
			cout << endl;
		}
	}
	cout << ">>��" << this->m_Index << "�ֱ�����ϣ�" << endl;
	system("pause");
	//system("cls");
}


void SpeechManager::showScore()
{
	cout << ">>��" << this->m_Index << "�ֽ�����ѡ����Ϣ���£�" << endl;
	vector<int>v;
	if (this->m_Index == 1)
	{
		v = v2;
	}
	else
	{
		v = vVictory;
	}
	for (vector<int>::iterator i = v.begin(); i != v.end(); i++)
	{
		cout << "ѡ�ֱ�ţ�" << *i << " ������" << this->m_Speaker[*i].m_Name << " �ɼ���" << this->m_Speaker[*i].m_Score[this->m_Index - 1] << endl;

	}
	cout << endl;
	system("pause");
	system("cls");
	this->show_Menu();
}


void SpeechManager::saveScore()
{
	ofstream ofs;
	ofs.open("speech.csv", ios::out | ios::app);
	for (vector<int>::iterator i = vVictory.begin(); i != vVictory.end(); i++)
	{
		ofs << *i << "," << this->m_Speaker[*i].m_Score[1] << ",";

	}
	ofs << endl;
	ofs.close();
	cout << "��¼�Ա��棡" << endl;
	this->fileIsEmpty = false;
}


void SpeechManager::loadRecord()
{
	ifstream ifs("speech.csv", ios::in);
	if (!ifs.is_open())
	{
		this->fileIsEmpty = true;
		//cout << "�ļ������ڣ�" << endl;
		ifs.close();
		return;
	}

	char ch;
	ifs >> ch;
	if (ifs.eof())
	{
		//cout << "�ļ�Ϊ�գ�" << endl;
		this->fileIsEmpty = true;
		ifs.close();
		return;
	}

	this->fileIsEmpty = false;
	ifs.putback(ch);
	string data;
	int flag = 0;

	while (ifs >> data)
	{
		//cout << data << endl;

		vector<string>v;
		int pos = -1;
		int start = 0;
		
		while (1)
		{
			pos = data.find(",", start);
			if (pos == -1)
			{
				break;
			}
			string temp = data.substr(start, pos - start);
			//cout << temp << endl;
			v.push_back(temp);
			start = pos + 1;
		}
		this->m_Record.insert(make_pair(flag, v));
		flag++;
	}
	ifs.close();
	//for (map<int, vector<string>>::iterator i = m_Record.begin(); i != m_Record.end(); i++)
	//{
	//	cout << i->first << " �ھ���ţ�" << i->second[0] << " �÷֣�" << i->second[1] << endl;
	//}

}


void SpeechManager::showRecord()
{
	if (this->fileIsEmpty)
	{
		cout << "�ļ������ڻ�Ϊ�գ�" << endl;
	}
	else
	{
		for (int i = 0; i < this->m_Record.size(); i++)
		{
			cout << "��" << i + 1 << "��" << " �ھ���ţ�" << this->m_Record[i][0] << " �ھ��÷֣�" << this->m_Record[i][1] << endl;
			cout << "��" << i + 1 << "��" << " �Ǿ���ţ�" << this->m_Record[i][2] << " �Ǿ��÷֣�" << this->m_Record[i][3] << endl;
			cout << "��" << i + 1 << "��" << " ������ţ�" << this->m_Record[i][4] << " �����÷֣�" << this->m_Record[i][5] << endl;
		}
	}
	
	system("pause");
	system("cls");
}




void SpeechManager::clearRecord()
{
	cout << "ȷ����գ�" << endl;
	cout << "1��ȷ��" << endl;
	cout << "2��ȡ��" << endl;
	int select = 0;
	cin >> select;
	if (select == 1)
	{
		ofstream ofs("speech.csv", ios::trunc);
		ofs.close();

		this->initSpeech();
		this->createSpeaker();
		this->loadRecord();

		cout << "��ճɹ���" << endl;
	}
	system("pause");
	system("cls");
}
