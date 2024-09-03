#include<iostream>
#include<thread>
#include<mutex>
#include "structures.h"
using namespace std;


class Board{
private:
	enum {rows=23, cols=60};
	mutex _m;
	char board[rows][cols];
	int pbulletXY[2], phealthXY[2], obulletXY[2], ohealthXY[2];
public:

	Board(){
		memset(this->board, ' ', sizeof(this->board));
		for(int i = 0;i < rows;i++){
			for(int j = 0;j < cols;j++){
				if(i == 0 || i == rows / 2 || i == rows - 1 || j == 0 || j == cols - 1)
					this->board[i][j] = 'X';
			}
		}
	}


	void showBoard(){
		for(int i = 0;i < rows;i++){
			for(int j = 0;j < cols;j++){
				printf("%c", this->board[i][j]);
			}
			cout<<endl;
		}
	}

	point placePlayer(char ch, point prev, point p){
		if(p.x < 1 || p.y < 1 || p.x > (rows - 2) / 2 or p.y > cols - 2) return prev;
		lock_guard<mutex> lock(_m);
		if(prev.x != -1){
			this->board[prev.x][prev.y] = ' ';
		}
		this->board[p.x][p.y] = ch;
		return p;
	}

	void placeBullet(int X, int Y){
		if(X < 1 || Y < 1 || X > rows - 2 or Y > cols - 2) return;
		lock_guard<mutex> lock(_m);
		this->board[X][Y] = '|';
	}
};
