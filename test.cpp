#include <vector>
#include <string>
#include <functional>
#include <iostream>
#include <iterator>
#include <set>
#include "heap.h"
#include "header.h"
using namespace std;

void test_vector_erase(){
	int o[]={1,2,3,4};
	vector<int> v(o,o+sizeof(o)/sizeof(int));
	//v.insert(v.begin(),o,o+sizeof(o)/sizeof(int));
	v.erase(++v.begin());
	for (int i = 0; i < v.size(); i++) {
		cout<<i<<":"<<v[i]<<endl;
	}
//	if( v.jjjjjj
}

void test_set(){
	int arr[]={3,4,1,2};
	set<int> s(arr,arr+sizeof(arr)/sizeof(int));
	s.insert(3);
	copy(s.begin(), s.end(), ostream_iterator<int>(cout, "\n"));
	cout<<*s.rbegin()<<endl;
}

int main(int argc, char const* argv[])
{
	test_vector_erase();
	//test_set();
	return 0;
}
