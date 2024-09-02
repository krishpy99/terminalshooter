#include<iostream>
#include<thread>
#include<cstdlib>
#include "board.h"
using namespace std;

int main(int argc, char const *argv[]){
	int prevX1 = -1, prevX2 = -1, prevY1 = -1, prevY2 = -1; 
	Board b;

	auto placePlayer1 = [&](int x, int y, int X, int Y) -> void {
		b.placePlayer1(x, y, X, Y);
	};

	auto placePlayer2 = [&](int x, int y, int X, int Y) -> void {
		b.placePlayer2(x, y, X, Y);
	};

	while(true){
		int X1 = rand()%21 + 1, X2 = rand()%21 + 1;
		int Y1 = rand()%58 + 1, Y2 = rand()%58 + 1;   
		thread t1(placePlayer1, prevX1, prevY1, X1, Y1);
		thread t2(placePlayer2, prevX2, prevY2, X2, Y2);

		t1.join();
		t2.join();
		prevX1 = X1;prevY1 = Y1;prevX2 = X2;prevY2 = Y2;
		system("clear");
		b.showBoard();
	}
	return 0;
}
