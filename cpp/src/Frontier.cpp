#include "Frontier.hpp"

#include <iostream>
#include <fstream>
#include <cmath>
#include <ctime>


Frontier::Frontier(redisContext *c, std::string seedFile)
	:c(c){
	loadSeeds(seedFile);
	std::cout << "URL Frontier Initialized" << std::endl;
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

int Frontier::remainingSeeds(){
	std::string query = "ZCOUNT delayed_queue -inf +inf";
	RedisReplyPtr seeds = RedisReplyPtr((redisReply*) redisCommand(c, query.c_str()));
	return seeds.reply->integer;
}

std::string Frontier::popUrl(std::string seed){
	std::string query = "LPOP urls:" + seed;
	RedisReplyPtr url = RedisReplyPtr((redisReply *) redisCommand(c, query.c_str()));

	if(url.reply->type == REDIS_REPLY_NIL){
		return "";
	}

	return url.reply->str;
}

void Frontier::removeSeed(std::string seed){
	std::string query = "ZREM delayed_queue " + seed;
	RedisReplyPtr((redisReply *) redisCommand(c, query.c_str()));
}

int Frontier::checkBf(std::string bfPrefix, std::string seed, std::string value){

	std::string query = bfPrefix + ":" + seed;

	const char* argv[] = {
			"BF.EXISTS",
			query.c_str(),
			value.c_str()
	};

	size_t argvlen[3] = {
			strlen(argv[0]),
			query.size(),
			value.size()
	};

	RedisReplyPtr exists = RedisReplyPtr((redisReply *) redisCommandArgv(c, 3, argv, argvlen));

	return exists.reply->integer;
}

void Frontier::reQueueSeed(std::string seed, int delay){
	std::string query = "ZADD delayed_queue " + std::to_string(time(0) + delay) + " " + seed;
	RedisReplyPtr((redisReply*) redisCommand(c, query.c_str()));
}

void Frontier::addUrl(std::string seed, std::string url){
	std::string query =  "urls:" + seed;

	const char* argv[] = {
			"LPUSH",
			query.c_str(),
			url.c_str()
	};

	size_t argvlen[3] = {
			strlen(argv[0]),
			query.size(),
			url.size()
	};

	RedisReplyPtr((redisReply *) redisCommandArgv(c, 3, argv, argvlen));
}

void Frontier::addToBf(std::string bfName, std::string seed, std::string value){
	std::string query = "BF.ADD " + bfName + ":"  + seed + " " + value;
	RedisReplyPtr((redisReply*) redisCommand(c, query.c_str()));
}
