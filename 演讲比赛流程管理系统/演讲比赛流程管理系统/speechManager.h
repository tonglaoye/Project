#pragma once
#include<iostream>
using namespace std;
#include<vector>
#include<map>
#include"speaker.h"
#include<string>
#include<algorithm>
#include<deque>
#include<functional>
#include<numeric>
#include<fstream>
#include<ctime>
class SpeechManager
{
public:

	SpeechManager();

	~SpeechManager();

	void show_Menu();

	void exitSystem();

	void initSpeech();

	void createSpeaker();

	void startSpeech();

	void speechDrew();

	void speechContest();

	void showScore();

	void saveScore();

	void loadRecord();

	void showRecord();

	void clearRecord();

	bool fileIsEmpty;

	map<int, vector<string>>m_Record;
	
	vector<int>v1;

	vector<int>v2;

	vector<int>vVictory;

	map<int, Speaker>m_Speaker;

	int m_Index;
};