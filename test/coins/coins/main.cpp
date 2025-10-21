
#include <vector>
#include <map>
#include <algorithm>
#include <iostream>
#include <string>

using namespace std;

namespace
{

	map<int, long long> m;

} // namespace

long long make_change(vector<int> coins, int money)
{
//	static map<int, long long> m;

	if (money <= 0)
	{
		return 0;
	}

	auto i = m.find(money);
	if (i != m.end())
	{
		return i->second;
	}

	long long n = 0;
	for (const auto& c : coins)
	{
		n += make_change(coins, money - c);
	}

	m[money] = n;

	return n;
}

void init(vector<int> coins)
{
	sort(coins.begin(), coins.end());
	int n = coins.size();

	for (int i = 0; i < n; ++i)
	{
		m[coins[i]] = 1;
		for (int j = 0; j < i; ++j)
		{
			m[coins[i]] += make_change(vector<int>(coins.begin(), coins.begin() + i), coins[i] - coins[j]);
		}
	}

	return;
}

long long makeChange(vector<int> coins, int money)
{
	vector<long long> DP(money + 1);
	DP[0] = 1;

	for (int coin : coins)
	{
		for (int j = coin; j < DP.size(); ++j)
		{
			DP[j] += DP[j - coin];
		}
	}

	return DP[money];
}

int main(int argc, char* argv[])
{
	argc;
	argv;

//	vector<int> coins = { 1, 2, 3 };
	vector<int> coins = { 2, 5, 3, 6 };
	init(coins);
//	cout << make_change(coins, 4);
//	cout << makeChange(coins, 4);
	cout << makeChange(coins, 10);
	
	string s;
	cin >> s;
}
