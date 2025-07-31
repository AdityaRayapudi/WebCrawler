#include <iostream>
#include <hiredis/hiredis.h>
#include <ctime>
#include <fstream>
#include <cmath>


// Redis Reply Pointer that will free itself at tend of scope
struct RedisReplyPtr {
	redisReply* reply;
	RedisReplyPtr(redisReply* r): reply(r){}
	~RedisReplyPtr(){ freeReplyObject(reply); }
};

class Frontier{
private:
	redisContext* c;

	void loadSeeds(std::string fileName){
		const int QUERY_SPEED = 100;

		std::string query;
		std::string url;
		std::ifstream seeds(fileName);

		int i = 0;
		while(getline (seeds, url)){
			// Create lists for each seed containing itself
			std::string query = "RPUSH urls:" + url + " " + url;
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

public:
	Frontier(redisContext *c, std::string seedFile){
		this-> c = c;
		loadSeeds(seedFile);
	}

	RedisReplyPtr getReadySeeds(){
		std::string query = "ZRANGEBYSCORE delayed_queue 0 " + std::to_string(time(0));
		return RedisReplyPtr((redisReply*) redisCommand(c, query.c_str()));
	}

	std::string popUrl(std::string seed){
		std::string query = "LPOP urls:" + seed;
		RedisReplyPtr url = RedisReplyPtr((redisReply *) redisCommand(c, query.c_str()));
		return url.reply->str;
	}

	void reQueueSeed(std::string seed, int delay = 2){
		std::string query = "ZADD delayed_queue " + std::to_string(time(0) + delay) + " " + seed;
		RedisReplyPtr((redisReply*) redisCommand(c, query.c_str()));
	}

	void addUrl(std::string seed, std::string url){
		std::string query = "LPUSH urls:" + seed + " " + url;
		RedisReplyPtr((redisReply*) redisCommand(c, query.c_str()));
	}
};


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

	RedisReplyPtr readySeeds = frontier.getReadySeeds();


	for(int i = 0; i < readySeeds.reply->elements; i++){

		std::string seed = readySeeds.reply->element[i]->str;
		std::string url = frontier.popUrl(seed);

//		std::cout << url << std::endl;


		frontier.reQueueSeed(seed);
	}



	redisFree(c);
}
