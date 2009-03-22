#include <vector>
#include <string>
#include <functional>
#include <iostream>
using namespace std;
#include "heap.h"

typedef heap<string*,vector<string*> > string_heap;

int main(){
	string_heap h;
	h.push(new string("bcd"));
	h.push(new string("abc"));
	h.push(new string("abd"));
	h.push(new string("cbc"));
	h.push(new string("ab"));
	h.push(new string("c"));

	h.sort();

	string_heap::iterator it;
	string str;
	
	for(it=h.begin();it!=h.end();it++)
		cout<<*it<<endl;

	/*
	while(!h.empty()){
		str = h.top();
		cout<<str<<endl;
		h.pop();
	}
	*/

	return 0;
}
