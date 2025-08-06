#ifndef REDIS_REPLY_HPP
#define REDIS_REPLY_HPP

#include <hiredis/hiredis.h>

struct RedisReplyPtr {
	redisReply* reply;
	RedisReplyPtr(redisReply* r);
	~RedisReplyPtr();

};

#endif
