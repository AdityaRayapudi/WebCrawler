FROM mitmproxy/mitmproxy:latest

WORKDIR /server

# Install pip requiremnts
COPY ../py/proxy/requirements.txt /server/requirements.txt

COPY ../py/proxy /server/proxy

RUN pip install --no-cache-dir --upgrade -r /server/requirements.txt

CMD ["mitmdump", "-s", "proxy/proxy.py", "-p", "8080"]
