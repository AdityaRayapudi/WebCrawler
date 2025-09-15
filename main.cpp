#include "Frontier.hpp"
#include "RedisReplyPtr.hpp"
#include "DNS_Resolver.hpp"
#include "WebScraper.hpp"

#include <iostream>
#include <hiredis/hiredis.h>
#include <nlohmann/json.hpp>

#include <chrono>
#include <thread>

int main(){

	std::cout << "--- Web Crawler ---" << "\n" << std::endl;

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

	std::cout << "Redis Connected" << std::endl;

	// Initialize frontier
	Frontier frontier(c, "Seeds.txt");

	// Initialize DNS Resolver
	DNS_Resolver resolver(c);


	RedisReplyPtr readySeeds = frontier.getReadySeeds();

	// Initialize Web Scraper to end-point
	WebScraper webScraper("http://127.0.0.1:8000");

	std::cout << "\n" << "-------------" << std::endl;

	// Main Crawler Loop

	for (int k = 0; k < 20; k++){
		std::cout << k << std::endl;
		for(int i = 0; i < readySeeds.reply->elements; i++){

			std::string seed = readySeeds.reply->element[i]->str;
			std::cout << "Run 1" << std::endl;

			// If ip is not cached use DNS Resolver
			if(resolver.is_cached(seed) == false){
				std::cout << "Searching for ipv4 of " << seed << "..." << std::endl;
				resolver.lookup(seed);
				continue;
			}

			std::cout << "Run 2" << std::endl;

			// Get next url and parse it
			std::string url = frontier.popUrl(seed);
			nlohmann::json pageData = webScraper.parsePage(seed, url);

			// Add current URL to seen Bloom Filter
			frontier.addToBf("seen", seed, url);

			std::cout << "Run 3" << std::endl;

			try{
				// Add all unscraped urls to queue
				for(std::string newUrl : pageData["urls"]){
					// Skip url if already scrapped
					if(frontier.checkBf("seen", seed, newUrl) == 1){
						continue;
					}

					frontier.addUrl(seed, newUrl);
				}
			}catch(int errorCode){
				std::cout << errorCode << std::endl;
			}


			std::cout << "Run 4" << std::endl;

			frontier.reQueueSeed(seed);

			std::cout << "sleeping" << std::endl;
//			std::this_thread::sleep_for(std::chrono::seconds(1));

		}

	}
	redisFree(c);
}
