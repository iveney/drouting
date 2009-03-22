#ifndef __HEAP_H__
#define __HEAP_H__

template <typename T,typename Container, typename Compare>
class heap{
public:
	/*
	class iterator:public Container::iterator{
	};
	*/
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
};

#endif
