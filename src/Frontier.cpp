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
	std::string url;
	std::ifstream seeds(fileName);

	int i = 0;
	while(getline (seeds, url)){
		// Create lists for each seed containing itself
		std::string query = "RPUSH urls:" + url + " /";
		redisReply* reply = (redisReply*) redisCommand(c, query.c_str());
		freeReplyObject(reply);

		// Add all seeds to delayed queue with 0 delay
		query = "ZADD delayed_queue " + std::to_string(time(0) + (int) floor(i / QUERY_SPEED)) + " " + url;
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
