from mitmproxy import http
import redis

# Connect to Redis
r = redis.Redis(host='127.0.0.1', port=6379, decode_responses=True)

def request(flow : http.HTTPFlow) -> None:
    host = flow.request.pretty_host
    ip = r.get("ip:" + host)

    # Update host and host header if cached
    if ip is not None:
        flow.request.scheme = "https"
        flow.request.host = host

        # Port for https requests
        flow.request.port = 443
        flow.request.headers["Host"] = host

        # Request ip 
        flow.request.address = (ip, 443)

