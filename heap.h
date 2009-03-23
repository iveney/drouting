// C++ heap class using template
// just a simple implementation using STL push_heap and sort_heap
#ifndef __HEAP_H__
#define __HEAP_H__

#include <algorithm>
#include <functional>
#include <vector>
#include <map>
using std::vector;
using std::sort_heap;
using std::less;
using std::map;

template <typename T,typename Container = vector<T>, 
	 typename Compare = less<typename Container::value_type> >
class heap{
public:
	typedef class Container::iterator iterator;

	////////////////////////////////////////////////////////
	// members
	int size() const {return c.size();}
	bool empty() const{return c.size() == 0; }
	T top() const {return c.front();}
	void push(const T & x){
		store(x);
		c.push_back(x);
		push_heap(c.begin(),c.end(),comp);
	}
	void pop() {
		pop_heap(c.begin(), c.end(),comp);
		c.pop_back();
	}
	iterator begin() { return c.begin(); }
	iterator end() { return c.end(); }
	void sort(){
		sort_heap(c.begin(),c.end(),comp);
	}

	////////////////////////////////////////////////////////
	// static members
	static void store(const T &x) {
		resource.push_back(x);
		order[x]=counter++;
	}
	static void free(){
		for(size_t i=0;i<resource.size();i++)
			delete resource[i];
		resource.clear();
		order.clear();
	}
	static void get_order(const T &x){
		return order[x];
	}

	/////////////////////////////////////////////////////////
	// data members
	
	Container c;   // the container to store elements
	Compare comp;  // compare function(strictly weak order
	static vector<T> resource;  // resource collector
	static map<T,int> order; // mark the generated order
	static int counter;
};

template <typename T,typename Container, typename Compare>
vector<T> heap<T,Container,Compare>::resource=vector<T>();

template <typename T,typename Container, typename Compare>
map<T,int> heap<T,Container,Compare>::order=map<T,int>();

template <typename T,typename Container, typename Compare>
int heap<T,Container,Compare>::counter=0;

#endif
