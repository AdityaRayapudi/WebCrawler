#ifndef FRONTIER_HPP
#define FRONTIER_HPP

#include "RedisReplyPtr.hpp"


#include <string>
#include <hiredis/hiredis.h>



class Frontier {
private:
	redisContext* c;

	void loadSeeds(std::string fileName);

public:
	Frontier(redisContext* c, std::string seedFile);

	RedisReplyPtr getReadySeeds();

	std::string popUrl(std::string seed);

	void reQueueSeed(std::string seed, int delay = 2);

	void addUrl(std::string seed, std::string url);

	void addToBf(std::string bfName, std::string seed, std::string value);

	int checkBf(std::string bfName, std::string seed, std::string value);

};

#endif
