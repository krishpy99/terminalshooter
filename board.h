#include<iostream>
#include<thread>
#include<mutex>
#include<vector>
#include "helpers.h"
using namespace std;


class Board{
private:
	int rows, cols;
	mutex _m;
	vector<vector<char>> board;
public:

	Board(int rows = 23, int cols = 25) : rows(rows), cols(cols){
		board = vector<vector<char>>(rows, vector<char>(cols, ' '));
		for(int i = 0;i < rows;i++){
			for(int j = 0;j < cols;j++){
				if(i == 0 || i == rows - 1 || j == 0 || j == cols - 1)
					this->board[i][j] = 'X';
				else if(i == rows/2)
					this->board[i][j] = '-';
			}
		}
	}

	int rows_(){
		return this->rows;
	}

	int cols_(){
		return this->cols;
	}


	void showBoard(){
		for(int i = 0;i < this->rows;i++){
			for(int j = 0;j < this->cols;j++){
				printf("%c", this->board[i][j]);
			}
			cout<<endl;
		}
	}

	point placeObject(char ch, point prev, point p){
		if(p.x < 1 || p.y < 1 || p.x > this->rows - 2 || p.y > cols - 2) {
			if(ch == '|') {
				this->board[prev.x][prev.y] = ' ';
			}
			return prev;
		}
		if(ch != '|'){
			if(prev.x < this->rows / 2 && p.x >= rows / 2) return prev;
			if(prev.x > this->rows / 2 && p.x <= rows / 2) return prev;
		}
		lock_guard<mutex> lock(_m);
		if(prev.x != -1 && this->board[prev.x][prev.y] == ch){
			this->board[prev.x][prev.y] = ' ';
			if(prev.x == this->rows / 2)
				this->board[prev.x][prev.y] = '-';
		}
		this->board[p.x][p.y] = ch;
		return p;
	}

	void placeBullet(int X, int Y){
		if(X < 1 || Y < 1 || X > this->rows - 2 or Y > cols - 2) return;
		lock_guard<mutex> lock(_m);
		this->board[X][Y] = '|';
	}
};
