// very disgusting problem with g++
// I just can not separte .h and .cpp
// so do not compile this file directly
// but just include "heap.h"
#include "heap.h"

template <typename T,typename Container, typename Compare>
heap<T,Container,Compare>::heap(){}

template <typename T,typename Container, typename Compare>
heap<T,Container,Compare>::~heap(){}

template <typename T,typename Container, typename Compare>
int heap<T,Container,Compare>::size() const{
	return c.size();
}

template <typename T,typename Container, typename Compare>
bool heap<T,Container,Compare>::empty() const{
	return c.size() == 0;
}

template <typename T,typename Container, typename Compare>
T heap<T,Container,Compare>::top() const {
	return c.front();
}

template <typename T,typename Container, typename Compare>
void heap<T,Container,Compare>::push(const T & x){
	store(x);
	c.push_back(x);
	push_heap(c.begin(),c.end(),comp);
}

template <typename T,typename Container, typename Compare>
void heap<T,Container,Compare>::pop() {
	pop_heap(c.begin(), c.end(),comp);
	c.pop_back();
}

template <typename T,typename Container, typename Compare>
class Container::iterator heap<T,Container,Compare>::end(){ return c.end(); }

template <typename T,typename Container, typename Compare>
class Container::iterator heap<T,Container,Compare>::begin(){return c.begin();}

template <typename T,typename Container, typename Compare>
void heap<T,Container,Compare>::sort(){
	sort_heap(c.begin(),c.end(),comp);
}

template <typename T,typename Container, typename Compare>
void heap<T,Container,Compare>::store(const T &x) {
	resource.push_back(x);
	order[x]=counter++;
}

template <typename T,typename Container, typename Compare>
void heap<T,Container,Compare>::free(){
	for(size_t i=0;i<resource.size();i++)
		delete resource[i];
	resource.clear();
	order.clear();
}

template <typename T,typename Container, typename Compare>
void heap<T,Container,Compare>::get_order(const T &x){
	return order[x];
}
