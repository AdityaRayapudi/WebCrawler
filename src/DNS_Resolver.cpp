#include "DNS_Resolver.hpp"

#include <iostream>


DNS_Resolver::DNS_Resolver(redisContext *c){
	this->c = c;

	// Get only Ipv4 Info
	hints.ai_family = AF_INET;

	// Initialize library
	ares_library_init(ARES_LIB_INIT_ALL);

	 // Enable event thread so we don't have to monitor file descriptors
	memset(&options, 0, sizeof(options));
	optmask |= ARES_OPT_EVENT_THREAD;
	options.evsys = ARES_EVSYS_DEFAULT;

	// Initialize channel to run queries, a single channel can accept unlimited
	// queries
	if (ares_init_options(&channel, &options, optmask) != ARES_SUCCESS) {
		std::cout << "c-ares initialization issue" << std::endl;
	}
}

void DNS_Resolver::dns_callback(void *arg, int status, int timeouts, struct ares_addrinfo *result){
	DNS_Resolver* self = static_cast<DNS_Resolver*>(arg);

	if (status != ARES_SUCCESS){
		std::cout << status << std::endl;
		return;
	}

	// Traverse through linked list of nodes from result
	for (struct ares_addrinfo_node *ai = result->nodes; ai != NULL; ai = ai->ai_next){
		char ip[INET_ADDRSTRLEN];

		// Data structure used to represent Ipv4 socket addresses
		struct sockaddr_in *addr = (struct sockaddr_in *) ai ->ai_addr;

		// If invalid 0 ip skip
		if(addr->sin_addr.s_addr == INADDR_ANY){
			std::cout << "Skipping 0.0.0.0" << std::endl;
			continue;
		}

		// Update ip to store the address
		inet_ntop(AF_INET, &(addr->sin_addr), ip, sizeof(ip));

		// Cache ip in redis db
		std::string query = "SET ip:" +  self->domain + " " + ip;
		RedisReplyPtr reply = RedisReplyPtr((redisReply*) redisCommand(self->c, query.c_str()));

		// Remove domain from pending set
		self->pendingLookup.erase(self->domain);

		// Only use first non 0 ip
		break;


	}


	ares_freeaddrinfo(result);
}



void DNS_Resolver::lookup(std::string domain){
	// End function if domain is already being looked up
	if(pendingLookup.find(domain) != pendingLookup.end()){
		return;
	}

	// Set current domain and add to pending
	this->domain = domain;
	pendingLookup.insert(domain);


	// Perform an IPv4 and IPv6 request for the provided domain name
	memset(&hints, 0, sizeof(hints));
	ares_getaddrinfo(channel, domain.c_str(), NULL, &hints, dns_callback, this);
	ares_queue_wait_empty(channel, -1);
}


DNS_Resolver::~DNS_Resolver(){
	ares_destroy(channel);

	ares_library_cleanup();
}
