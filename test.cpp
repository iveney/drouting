#include <vector>
#include <string>
#include <functional>
#include <iostream>
#include <ext/hash_map>
#include "heap.h"
#include "header.h"
using __gnu_cxx::hash_map;
using namespace std;

int main(){
	hash_map<string,int> m;
	m["123"]=5;
	return 0;
}
