#include "DNS_Resolver.hpp"

#include <iostream>


DNS_Resolver::DNS_Resolver(redisContext *c){
//		this->c = c;

	/* Get only Ipv4 Info */
	hints.ai_family = AF_INET;

	/* Initialize library */
	ares_library_init(ARES_LIB_INIT_ALL);

	 /* Enable event thread so we don't have to monitor file descriptors */
	memset(&options, 0, sizeof(options));
	optmask |= ARES_OPT_EVENT_THREAD;
	options.evsys = ARES_EVSYS_DEFAULT;

	/* Initialize channel to run queries, a single channel can accept unlimited
	* queries */
	if (ares_init_options(&channel, &options, optmask) != ARES_SUCCESS) {
		std::cout << "c-ares initialization issue" << std::endl;
	}
}

void DNS_Resolver::dns_callback(void *arg, int status, int timeouts, struct ares_addrinfo *result){

	if (status != ARES_SUCCESS){
		std::cout << status << std::endl;
		return;
	}

	/* Traverse through linked list of nodes from result*/
	for (struct ares_addrinfo_node *ai = result->nodes; ai != NULL; ai = ai->ai_next){
		char ip[INET_ADDRSTRLEN];

		/* Data structure used to represent Ipv4 socket addresses */
		struct sockaddr_in *addr = (struct sockaddr_in *) ai ->ai_addr;
		inet_ntop(AF_INET, &(addr->sin_addr), ip, sizeof(ip));
		std::string query = "ip:" +  *(std::string *)arg + " " + ip;
		std::cout << query << std::endl;
//			RedisReplyPtr reply = RedisReplyPtr((redisReply*) redisCommand(c, query.c_str()));
	}


	ares_freeaddrinfo(result);
}



void DNS_Resolver::lookup(std::string domain){
	/* Perform an IPv4 and IPv6 request for the provided domain name */
	memset(&hints, 0, sizeof(hints));
	ares_getaddrinfo(channel, domain.c_str(), NULL, &hints, dns_callback, &domain);
	ares_queue_wait_empty(channel, -1);
}

DNS_Resolver::~DNS_Resolver(){
	ares_destroy(channel);

	ares_library_cleanup();
}
