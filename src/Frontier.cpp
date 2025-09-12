#include "Frontier.hpp"

#include <iostream>
#include <fstream>
#include <cmath>
#include <ctime>


//// Redis Reply Pointer that will free itself at tend of scope
//RedisReplyPtr::RedisReplyPtr(redisReply* r) : reply(r){}
//
//RedisReplyPtr::~RedisReplyPtr() { freeReplyObject(reply); }

Frontier::Frontier(redisContext *c, std::string seedFile){
	this-> c = c;
	loadSeeds(seedFile);
}


void Frontier::loadSeeds(std::string fileName){
	const int QUERY_SPEED = 100;

	std::string query;
	std::string seed;
	std::ifstream seeds(fileName);

	int i = 0;
	while(getline (seeds, seed)){
		// Create lists for each seed containing itself
		std::string query = "RPUSH urls:" + seed + " /";
		redisReply* reply = (redisReply*) redisCommand(c, query.c_str());
		freeReplyObject(reply);

		// Add all seeds to delayed queue with 0 delay
		query = "ZADD delayed_queue " + std::to_string(time(0) + (int) floor(i / QUERY_SPEED)) + " " + seed;
		reply = (redisReply*) redisCommand(c, query.c_str());
		freeReplyObject(reply);

		// Create BF filter for each brand with 0.001 error and an initial 10_000 capacity
		query = "BF.RESERVE seen:"  + seed + " 0.001 10000";
		reply = (redisReply*) redisCommand(c, query.c_str());
		freeReplyObject(reply);

		i++;
	}
	seeds.close();
}

RedisReplyPtr Frontier::getReadySeeds(){
	std::string query = "ZRANGEBYSCORE delayed_queue 0 " + std::to_string(time(0));
	return RedisReplyPtr((redisReply*) redisCommand(c, query.c_str()));
}

std::string Frontier::popUrl(std::string seed){
	std::string query = "LPOP urls:" + seed;
	RedisReplyPtr url = RedisReplyPtr((redisReply *) redisCommand(c, query.c_str()));
	return url.reply->str;
}

void Frontier::reQueueSeed(std::string seed, int delay){
	std::string query = "ZADD delayed_queue " + std::to_string(time(0) + delay) + " " + seed;
	RedisReplyPtr((redisReply*) redisCommand(c, query.c_str()));
}

void Frontier::addUrl(std::string seed, std::string url){
	std::string query = "LPUSH urls:" + seed + " " + url;
	RedisReplyPtr((redisReply*) redisCommand(c, query.c_str()));
}

void Frontier::addToBf(std::string bfName, std::string seed, std::string value){
	std::string query = "BF.ADD " + bfName + ":"  + seed + " " + value;
	redisReply* reply = (redisReply*) redisCommand(c, query.c_str());
	std::cout << reply->str << std::endl;
}

int Frontier::checkBf(std::string bfName, std::string seed, std::string value){
	std::string query = "BF.EXISTS " + bfName + ":"  + seed + " " + value;
	redisReply* reply = (redisReply*) redisCommand(c, query.c_str());

	return reply->integer;
}
