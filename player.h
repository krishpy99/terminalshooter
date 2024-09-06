#include<iostream>
#include<mutex>
#include "helpers.h"
using namespace std;

class Player{
private:
	mutex _m;
	char rep;
	point pos;
	int bullets;
	int health;
public:
	Player(char ch){
		this->rep = ch;
		this->bullets = 10;
		this->health = 100;
	}

	char getRep(){
		return this->rep;
	}
	void takeDamage(int damage){
		this->health -= damage;
	}

	void increaseBullets(int count){
		this->bullets += count;
	}

	point shoot(){
		this->bullets -= 1;
		return this->pos;
	}

	point getPos(){
		return this->pos;
	}

	void setPos(point pos){
		lock_guard<mutex> lock(_m);
		this->pos = pos;
	}
};
