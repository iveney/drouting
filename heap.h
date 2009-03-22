// C++ heap class using template
// just a simple implementation using STL push_heap and sort_heap
#ifndef __HEAP_H__
#define __HEAP_H__

#include <algorithm>
#include <functional>
#include <vector>
using std::vector;
using std::sort_heap;
using std::less;

template <typename T,typename Container = vector<T>, 
	 typename Compare = less<typename Container::value_type> >
class heap{
public:
	typedef class Container::iterator iterator;
	Container c;
	Compare comp;
	int size() const {return c.size();}
	bool empty() const{return c.size() == 0; }
	T top() const {return c.front();}
	void push(const T & x){
		c.push_back(x);
		push_heap(c.begin(),c.end(),comp);
	}
	void pop() {
		pop_heap(c.begin(), c.end(), comp);
		c.pop_back();
	}
	iterator begin() { return c.begin(); }
	iterator end() { return c.end(); }
	void sort(){
		sort_heap(c.begin(),c.end(),comp);
	}
};

#endif
