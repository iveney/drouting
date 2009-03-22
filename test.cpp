#include <vector>
#include <string>
#include <functional>
#include <iostream>
using namespace std;
#include "heap.h"

typedef heap<string,vector<string>,greater<string> > string_heap;

int main(){
	string_heap h;
	h.push("bcd");
	h.push("abc");
	h.push("abd");
	h.push("cbc");
	h.push("ab");
	h.push("c");

	string_heap::iterator it;
	string str;

	while(!h.empty()){
		str = h.top();
		cout<<str<<endl;
		h.pop();
	}

	return 0;
}
