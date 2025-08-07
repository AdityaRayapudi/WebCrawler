#ifndef DNS_RESOLVER_HPP
#define DNS_RESOLVER_HPP

#include "RedisReplyPtr.hpp"

#include <hiredis/hiredis.h>
#include <ares.h>
#include <arpa/inet.h>
#include <string>
#include <unordered_set>

class DNS_Resolver{
private:
	redisContext* c;

	ares_channel_t *channel = NULL;
	struct ares_options options;
	int optmask = 0;
	struct ares_addrinfo_hints hints;

	std::string domain;
	std::unordered_set<std::string> pendingLookup;

	static void dns_callback(void *arg, int status, int timeouts, struct ares_addrinfo *result);

public:
	DNS_Resolver(redisContext *c = NULL);

	void lookup(std::string domain);

	~DNS_Resolver();
};


#endif
