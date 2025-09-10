FROM python:3.13.7-trixie

WORKDIR /py

COPY ../py/app/requirements.txt /py/requirements.txt

RUN pip install --no-cache-dir --upgrade -r /py/requirements.txt

COPY ../py/app /py/app

CMD ["fastapi", "run", "app/app.py", "--proxy-headers", "--port", "80"]