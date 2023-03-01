## Архитектура
* Язык программирования: **C**
* Реализация многопоточности: **prefork**
* Асинхронность: **libev**

## Конфигурация
Задание конфигураций осуществляется через файл **serv.conf**

### Параметры:
* _port_ - порт, на котором сервер будет слушать соединения (_8080_ по умолчанию)
* _cpu_limit_ - количество работающих процессов. Рекомендуется устанавливать число, равное числу ядер в системе (_1_ по умолчанию)
* _root_dir_ - директория, относительно которой сервер ищет запрашиваемые файлы (_/var/www/html_ по умолчанию).
Не рекомендуется устанавливать корневую директорию.

## Использование
* Запуск/перезапуск - **make**
* Использование готового [докер-контейнера](https://hub.docker.com/repository/docker/ivanbir/barsev/general) - **make use-docker**
* Сборка бинарного файла - **make build-bin**
* Запуск бинарного файла - **make go-bin**
* Сборка докер-образа - **make build-docker**
* Запуск в докер-контейнере - **make run-docker**

## Тестирование
* Запуск функционального тестирования на сервер - **make func**
* Запуск нагрузочного тестирования на сервер - **make perf-server**
* Сборка Nginx - **make build-nginx**
* Запуск Nginx - **make run-nginx**
* Запуск нагрузочного тестирования на Nginx - **make perf-nginx**

## Сравнительная оценка результатов тестирования
Нагрузочное тестирование производилось с помощью Apache HTTP server benchmarking tool.

* **RPS_serv** = 1862.87
* **RPS_nginx** = 1843.89

RPS_serv/RPS_nginx = **1.01**

### Сервер
```
Server Software:        server
Server Hostname:        127.0.0.1
Server Port:            80

Document Path:          /httptest/wikipedia_russia.html
Document Length:        954824 bytes

Concurrency Level:      10
Time taken for tests:   5.368 seconds
Complete requests:      10000
Failed requests:        0
Total transferred:      9549280000 bytes
HTML transferred:       9548240000 bytes
Requests per second:    1862.87 [#/sec] (mean)
Time per request:       5.368 [ms] (mean)
Time per request:       0.537 [ms] (mean, across all concurrent requests)
Transfer rate:          1737211.69 [Kbytes/sec] received

Connection Times (ms)
              min  mean[+/-sd] median   max
Connect:        0    0   0.1      0       2
Processing:     2    5   1.3      5      42
Waiting:        0    1   1.2      0      37
Total:          2    5   1.3      5      42

Percentage of the requests served within a certain time (ms)
  50%      5
  66%      5
  75%      6
  80%      6
  90%      6
  95%      6
  98%      7
  99%      8
 100%     42 (longest request)
```

### Nginx
```
Server Software:        nginx/1.23.3
Server Hostname:        127.0.0.1
Server Port:            8000

Document Path:          /httptest/wikipedia_russia.html
Document Length:        954824 bytes

Concurrency Level:      10
Time taken for tests:   5.423 seconds
Complete requests:      10000
Failed requests:        0
Total transferred:      9550620000 bytes
HTML transferred:       9548240000 bytes
Requests per second:    1843.89 [#/sec] (mean)
Time per request:       5.423 [ms] (mean)
Time per request:       0.542 [ms] (mean, across all concurrent requests)
Transfer rate:          1719757.37 [Kbytes/sec] received

Connection Times (ms)
              min  mean[+/-sd] median   max
Connect:        0    0   0.1      0       1
Processing:     2    5   1.4      5      29
Waiting:        0    1   1.0      0      17
Total:          2    5   1.4      5      29

Percentage of the requests served within a certain time (ms)
  50%      5
  66%      5
  75%      6
  80%      6
  90%      6
  95%      7
  98%      9
  99%     11
 100%     29 (longest request)
```