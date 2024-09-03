#include<iostream>
#include<thread>
#include<cstdlib>
#include "board.h"
#include "structures.h"
using namespace std;

int main(int argc, char const *argv[]){
	Board b;

	point prev1, prev2;

	auto placePlayer = [&](char ch, point* prev, point* p) -> void {
		point q = b.placePlayer(ch, *prev, *p);
		prev->x = q.x; prev->y = q.y;
	};

	while(true){
		point p, q;
		p.x = rand()%21 + 1, q.x = rand()%21 + 1;
		p.y = rand()%58 + 1, q.y = rand()%58 + 1;   
		thread t1(placePlayer, 'P', &prev1, &p);
		thread t2(placePlayer, 'Q', &prev2, &q);

		t1.join();
		t2.join();
		system("clear");
		b.showBoard();
	}
	return 0;
}
