#pragma once
#include <string>
#include <random>
#include <sstream>
#include <map>
#include <type_traits>
#include <algorithm>
#include "board.h"
#include "action.h"

//rnd env : random bag size = 3 done by bag and op_space last_op
//taketurn : init = 9 block : done by man(x,8)%2
//slide
//output
extern int last_op;

class agent {
public:
	agent(const std::string& args = "") {
		std::stringstream ss("name=unknown role=unknown " + args);
		for (std::string pair; ss >> pair; ) {
			std::string key = pair.substr(0, pair.find('='));
			std::string value = pair.substr(pair.find('=') + 1);
			meta[key] = { value };
		}
	}
	virtual ~agent() {}
	virtual void open_episode(const std::string& flag = "") {}
	virtual void close_episode(const std::string& flag = "") {}
	virtual action take_action(const board& b) { return action(); }
	virtual bool check_for_win(const board& b) { return false; }

public:
	virtual std::string property(const std::string& key) const { return meta.at(key); }
	virtual void notify(const std::string& msg) { meta[msg.substr(0, msg.find('='))] = { msg.substr(msg.find('=') + 1) }; }
	virtual std::string name() const { return property("name"); }
	virtual std::string role() const { return property("role"); }

protected:
	typedef std::string key;
	struct value {
		std::string value;
		operator std::string() const { return value; }
		template<typename numeric, typename = typename std::enable_if<std::is_arithmetic<numeric>::value, numeric>::type>
		operator numeric() const { return numeric(std::stod(value)); }
	};
	std::map<key, value> meta;
};

class random_agent : public agent {
public:
	random_agent(const std::string& args = "") : agent(args) {
		if (meta.find("seed") != meta.end())
			engine.seed(int(meta["seed"]));
	}
	virtual ~random_agent() {}

protected:
	std::default_random_engine engine;
};

/**
 * random environment
 * add a new random tile to an empty cell
 * 2-tile: 90%
 * 4-tile: 10%
 */
class rndenv : public random_agent { //evil as instance
public:
	rndenv(const std::string& args = "") : random_agent("name=random role=environment " + args),
		space({ 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15 })/*, popup(0, 9)*/ {
			idx = 0; //consruction wont re run for another episode
		}

	virtual void open_episode(const std::string& flag = "") {
		idx = 0;
	}

	virtual action take_action(const board& after) { //affect ep_state by returning action
		if(idx == 0){
			std::shuffle(bag, bag + 3, engine); //reuse engine ok?
			//printf("\nshuffle!!!\n");
		}

		if(last_op == -1){ //cant use ~last_op, maybe type issue
			std::shuffle(space.begin(), space.end(), engine);
			for (int pos : space) {
				if (after(pos) != 0) continue;
				//printf("\nno op : place bag[%d] = %d @ %d\n", idx, bag[idx], pos);
				board::cell tile = bag[idx];
				idx = (idx + 1) % 3;
				return action::place(pos, tile); //how to affect ep_state
			}
			return action();
		}else{
			std::shuffle(opspace[last_op], opspace[last_op] + 4, engine);
			for (int pos : opspace[last_op]) {
				if (after(pos) != 0) continue;
				board::cell tile = bag[idx];
				//printf("\nop code = %d : place bag[%d] = %d @ %d\n", last_op, idx, bag[idx], pos);
				idx = (idx + 1) % 3;
				return action::place(pos, tile);
			}
			return action();
		}
	}

private:
	std::array<int, 16> space;
	int opspace[4][4] = { {12, 13, 14, 15}, {0, 4, 8, 12}, {0, 1, 2, 3}, {3, 7, 11, 15} }; //urdl
	//std::uniform_int_distribution<int> popup;
	int bag[3] = {1, 2, 3};
	int idx;
};

/**
 * dummy player
 * select a legal action randomly
 */
class player : public random_agent {
public:
	player(const std::string& args = "") : random_agent("name=dummy role=player " + args),
		opcode({ 0, 1, 2, 3 }) {}

	virtual action take_action(const board& before) {
		//std::shuffle(opcode.begin(), opcode.end(), engine);
		for (int op : opcode) {
			board::reward reward = board(before).slide(op);
			if (reward != -1) return action::slide(op);
		}
		return action();
	}

private:
	std::array<int, 4> opcode;
};
