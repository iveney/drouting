// Filename : droute_draw.cpp
// generate TeX files 
// read from stdin
// file format:(e.g.)
// net[0]:20
//     0: (1,3)
//     1: (2,3)
//     2: ...
//     20:(21,3)
//
// net[1]:20
// Author : Xiao Zigang
// Modifed: < Tue Mar 17 10:39:54 HKT 2009 >
// ----------------------------------------------------------------//

#include <stdio.h>
const char * getColor(){
	static int counter=0;
	const static char * colors[]={
		"blue","green","orange",
		"purple","yellow","pink",
		"violet","red","cyan"};
	const char * p=colors[counter];
	int size=sizeof(colors)/sizeof(char*);
	counter=(counter+1)%size;
	return p;
}

int main(){
	int neti,total_time;
	int t,x,y;
	while( scanf("net[%d]:%d\n",&neti,&total_time) != EOF ){
		const char * p = getColor();
		for(int i=0;i<=total_time;i++){
			scanf("%d : (%d,%d)\n",&t,&x,&y);
			printf("\\node[pins,%s] (net_%d_%d_%d) at (%d+\\half,%d+\\half) {};\n",
					p,neti,x,y,x,y);
		}
		printf("\n");
	}
	return 0;
}
