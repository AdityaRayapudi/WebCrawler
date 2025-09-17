FROM mcr.microsoft.com/playwright/python:v1.50.0-noble

WORKDIR /py

COPY ../py/app/requirements.txt /py/requirements.txt

RUN pip install --no-cache-dir --upgrade -r /py/requirements.txt

RUN playwright install --with-deps chromium

COPY ../py/app /py/app

CMD ["fastapi", "run", "app/app.py", "--proxy-headers", "--port", "80"]