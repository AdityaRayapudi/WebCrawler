#include "Frontier.hpp"
#include "RedisReplyPtr.hpp"
#include "DNS_Resolver.hpp"

#include <iostream>
#include <hiredis/hiredis.h>

int main(){
	// Connect to Redis DB
	redisContext* c = redisConnect("127.0.0.1", 6379);

	// Check for NULL or Error
	if (c == NULL || c->err){
		if (c != NULL) {
			std::cout << c->err << std::endl;
		} else {
			std::cout << "Can't allocate redis context" << std::endl;
		}
		exit(1);
	}

	// Initialize frontier
	Frontier frontier(c, "Seeds.txt");

	// Initialize DNS Resolver
	DNS_Resolver resolver(c);


	RedisReplyPtr readySeeds = frontier.getReadySeeds();


	for(int i = 0; i < readySeeds.reply->elements; i++){

		std::string seed = readySeeds.reply->element[i]->str;


		std::string query = "GET ip:" + seed;
		RedisReplyPtr ip = RedisReplyPtr((redisReply *) redisCommand(c, query.c_str()));

		// Lookup Ipv4 if not cached
		if(ip.reply->str == NULL){
			std::cout << "Searching for ipv4 of " << seed << "..." << std::endl;
			resolver.lookup(seed);
			continue;
		}


		std::string url = frontier.popUrl(seed);

		std::cout << ip.reply->str << url << std::endl;


		frontier.reQueueSeed(seed);
	}



	redisFree(c);
}
