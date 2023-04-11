#define _CRT_SECURE_NO_WARNINGS 1

#include"more.h"
#include<iostream>
using namespace std;
#include<cstring>

int compare(int a, int b)
{
	if (a == b)
		return 0;
	return a > b ? 1 : -1;
}

list<More>& inpu(list<More> &l)
{
	cout << "请输入几项的多项式：" << endl;
	int m;
	cin >> m;
	int i = 1;
	while (m--)
	{
		int c;
		int n;
		cout << "请输入第" << i << "的参数和系数：" << endl;
		cin >> c >> n;
		More* p = new More(c, n);
		l.push_back(*p);
		++i;
	}
	return l;
}

list<More> mul(list<More> l1,list<More> l2)
{
	list<More> l;
	for (auto it1 = l1.begin(); it1 != l1.end(); ++it1)
	{
		for (auto it2 = l2.begin(); it2 != l2.end(); ++it2)
		{
			More* p = new More(it1->c * it2->c,it1->n + it2->n);
			l.push_back(*p);
		}
	}
	return l;
}


list<More> add(list<More> l1, list<More> l2)
{
	list<More> l;
	auto it1 = l1.begin();
	auto it2 = l2.begin();
	More* p = nullptr;
	while (it1 != l1.end() || it2 != l2.end())
	{
		switch (compare(it1->n,it2->n))
		{
		case 1:
			p = new More(it1->c, it1->n);
			l.push_back(*p);
			break;
		case -1:
			p = new More(it2->c, it2->n);
			l.push_back(*p);
			break;
		case 0:
			p = new More(it1->c + it2->c, it1->n);
			l.push_back(*p);
			break;
		default:
			break;
		}
		++it1;
		++it2;
	}
	return l;
}


int main()
{
	list<More> l1;
	list<More> l2;
	cout << "第一个多项式的输入：" << endl;
	inpu(l1);
	cout << "第二个多项式的输入：" << endl;
	inpu(l2);
	list<More> lm = mul(l1, l2);
	list<More> la = add(l1, l2);
	cout << "相乘结果为：" << endl;
	for (auto i : lm)
	{
		cout << i.c << " " << i.n << endl;
	}
	cout << "相加结果为：" << endl;
	for (auto i : la)
	{
		cout << i.c << " " << i.n << endl;
	}
	return 0;
}