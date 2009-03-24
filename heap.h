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
	// define the heap's container's iterator(a class)
	typedef class Container::iterator iterator;

	////////////////////////////////////////////////////////
	// members
	// get the size of current heap
	int size() const;

	// check if the heap is empty
	bool empty() const;

	// return the top of the heap
	T top() const;

	// push an element into heap
	void push(const T & x);

	// pop an element outside heap
	void pop();
	iterator begin();
	iterator end(); 
	void sort();

	/////////////////////////////////////////////////////////////
	// static members
	// put the value type x into resource,and save its mapping
	static void store(const T &x); 

	// clean up the resources
	static void free();

	// get the push order of value_type x
	static void get_order(const T &x);

	/////////////////////////////////////////////////////////////
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
