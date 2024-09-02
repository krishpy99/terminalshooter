#include<iostream>
#include<thread>
#include<mutex>
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

	void placePlayer1(int prevX, int prevY, int X, int Y){
		if(X < 1 || Y < 1 || X > (rows - 2) / 2 or Y > cols - 2) return;
		lock_guard<mutex> lock(_m);
		if(prevX != -1){
			this->board[prevX][prevY] = ' ';
		}
		this->board[X][Y] = 'P';
	}

	void placePlayer2(int prevX, int prevY, int X, int Y){
		if(X < (rows + 2) / 2 || Y < 1 || X > rows - 2 or Y > cols - 2) return;
		lock_guard<mutex> lock(_m);
		if(prevX != -1){
			this->board[prevX][prevY] = ' ';
		}
		this->board[X][Y] = 'Q';
	}

	void placeBullet(int X, int Y){
		if(X < 1 || Y < 1 || X > rows - 2 or Y > cols - 2) return;
		lock_guard<mutex> lock(_m);
		this->board[X][Y] = '|';
	}
};
