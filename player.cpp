#include<iostream>
using namespace std;

class Player{
private:
	char rep;
	int position[2], pposition[2];
	int bullets;
	int health;
public:
	Player(){
		this->pposition = {-1,-1};
		this->position = {-1, -1};
		this->bullets = 10;
		this->health = 100;
	}

	void takeDamage(int damage){
		this->health -= damage;
	}

	void increaseBullets(int count){
		this->bullets += count;
	}

	int[2] shoot(){
		this->bullets -= 1;
		return position;
	}
}
