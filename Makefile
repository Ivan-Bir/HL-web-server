PHONY: all
all:
	make stop-docker; make build-docker && make run-docker

build-bin:
	./build.sh

go-bin:
	sudo ./build/server

on-docker:
	sudo systemctl start docker

use-docker:
	docker run --rm -p 80:80 --name server -t ivanbir/barsev

build-docker:
	docker build -t server .

run-docker:
	docker run --rm -p 80:80 --name server -t server

stop-docker:
	docker stop server

build-nginx:
	docker build -t nginx -f nginx.Dockerfile .

run-nginx:
	docker run --rm -p 8000:8000 --name nginx -t nginx

perf-nginx:
	ab -n 10000 -c 10 127.0.0.1:8000/httptest/wikipedia_russia.html

perf-server:
	ab -n 10000 -c 10 127.0.0.1:80/httptest/wikipedia_russia.html

func:
	./httptest.py

