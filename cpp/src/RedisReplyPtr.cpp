#include "RedisReplyPtr.hpp"

// Redis Reply Pointer that will free itself at tend of scope
RedisReplyPtr::RedisReplyPtr(redisReply* r) : reply(r){}

RedisReplyPtr::~RedisReplyPtr() { freeReplyObject(reply); }
