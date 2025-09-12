#include "Frontier.hpp"
#include "RedisReplyPtr.hpp"
#include "DNS_Resolver.hpp"
#include "WebScraper.hpp"

#include <iostream>
#include <hiredis/hiredis.h>
#include <nlohmann/json.hpp>

#include <typeinfo>

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

	// Initialize Web Scraper to end-point
	WebScraper webScraper("http://127.0.0.1:8000");

	for(int i = 0; i < readySeeds.reply->elements; i++){

		std::string seed = readySeeds.reply->element[i]->str;

		// Lookup Ipv4 if not cached
		std::string query = "EXISTS ip:" + seed;
		RedisReplyPtr ip_chached = RedisReplyPtr((redisReply *) redisCommand(c, query.c_str()));

		if(ip_chached.reply->integer == 0){
			std::cout << "Searching for ipv4 of " << seed << "..." << std::endl;
			resolver.lookup(seed);
			continue;
		}

		// Get next url and parse it
		std::string url = frontier.popUrl(seed);
		nlohmann::json pageData = webScraper.parsePage(seed, url);

		// Add current URL to seen Bloom Filter
		frontier.addToBf("seen", seed, url);


		// Add all unscraped urls to queue
		for(std::string newUrl : pageData["urls"]){
			// Skip url if already scrapped
			if(frontier.checkBf("seen", seed, newUrl) == true){
				std::cout << "Already scraped: " << seed << newUrl << std::endl;
				continue;
			}

			std::cout << "New url: " << seed << newUrl << std::endl;
			frontier.addUrl(seed, newUrl);
		}

		frontier.reQueueSeed(seed);
	}

	redisFree(c);
}
