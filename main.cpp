#include<iostream>
#include<thread>
#include<cstdlib>
#include<mutex>
#include "board.h"
#include "helpers.h"
#include "player.h"
using namespace std;


int main(int argc, char const *argv[]){
	Board b;
	Player p1('P'), p2('Q');
	p1.setPos(b.placeObject(p1.getRep(), p1.getPos(), point(1,1)));
	p2.setPos(b.placeObject(p2.getRep(), p2.getPos(), point(19,5)));

	mutex _main;

	vector<point> activeBullets;
	
	auto movePlayer = [&](Player *p) -> void {
		while(true){
			point cur = p->getPos();
			char ch = getch();
			switch(ch){
				case 'w':
					cur.x--;
					break;
				case 'a':
					cur.y--;
					break;
				case 's':
					cur.x++;
					break;
				case 'd':
					cur.y++;
					break;
				case ' ':
					{
						lock_guard<mutex> lock(_main);
						activeBullets.push_back(point(cur.x > b.rows_() ? -cur.x : cur.x, cur.y)); //need to update
					}
					break;
				default:
					break;
			}
			point pt = b.placeObject(p->getRep(), p->getPos(), cur);
			p->setPos(pt);
		}
	};

	auto moveBullets = [&]() -> void {
		while(true){
			int i = 0;
			while(i < activeBullets.size()){
				point c = activeBullets[i];
				point pt = b.placeObject('|', c, point(abs(c.x+1), c.y));
				if(pt.x == abs(c.x)){
					lock_guard<mutex> lock(_main);
					swap(activeBullets[i], activeBullets[activeBullets.size() - 1]);
					activeBullets.pop_back();
					if(activeBullets.size() == 0) break;
					continue;
				}
				activeBullets[i] = pt;
				i++;
			}
			this_thread::sleep_for(100ms);
		}
	};

	auto showBoard = [&]() -> void {
		while(true){
			system("clear");
			b.showBoard();
		}
	};

	auto spawnHealth = [&]() -> void {
		
	};

	auto spawnBullets = [&]() -> void {

	};


	thread t1(movePlayer, &p1);
	thread t2(showBoard);
	thread t3(moveBullets);
	t1.join();
	t2.join();
	t3.join();
	return 0;
}
