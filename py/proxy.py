from mitmproxy import http
import redis

r = redis.Redis(host='127.0.0.1', port=6379, decode_responses=True)

def request(flow : http.HTTPFlow) -> None:
    host = flow.request.pretty_host
    ip = r.get("ip:" + host)

    # Update host and host header
    flow.request.host = ip
    flow.request.headers["Host"] = host
