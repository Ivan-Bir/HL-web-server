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

Значения RPS в зависимости от числа воркеров в сервере:
| **Num cores** | **server** | **nginx** |
|---------------|------------|-----------|
|1              |1905.45     |  1750.46  |
|2              |1938.91     |  2192.94  |
|3              |1954.87     |  2146.59  |
|4              |1932.83     |  2093.66  |
|12(max cores for stand)              |**1964.36**     |  **2151.19**  |
|16              |1921.12     |  2137.04  |
|64              |1856.84     |  2143.54  |

Нетрудно видеть, что оптимум располагается при числе воркеров, равным числу ядер. Однако с дальнейшим увеличением числа воркеров производительность уменьшается на сервере, ввиду, вероятно, дополнительных накладных расходов на переклюения контекста процессором для большего числа процессов.

В nginx такой деградации не было замечено, ввиду, предположительно, имеющихся оптимизаций для большого числа воркеров.

### Сервер
```
Server Software:        server
Server Hostname:        127.0.0.1
Server Port:            80

Document Path:          /httptest/wikipedia_russia.html
Document Length:        954824 bytes

Concurrency Level:      10
Time taken for tests:   5.091 seconds
Complete requests:      10000
Failed requests:        1
   (Connect: 0, Receive: 0, Length: 1, Exceptions: 0)
Total transferred:      9548325072 bytes
HTML transferred:       9547285176 bytes
Requests per second:    1964.36 [#/sec] (mean)
Time per request:       5.091 [ms] (mean)
Time per request:       0.509 [ms] (mean, across all concurrent requests)
Transfer rate:          1831677.68 [Kbytes/sec] received

Connection Times (ms)
              min  mean[+/-sd] median   max
Connect:        0    0   0.0      0       1
Processing:     1    5   1.8      5      65
Waiting:        0    0   1.8      0      61
Total:          2    5   1.8      5      65

Percentage of the requests served within a certain time (ms)
  50%      5
  66%      5
  75%      5
  80%      5
  90%      5
  95%      5
  98%      6
  99%      6
 100%     65 (longest request)
```

### Nginx
```
Server Software:        nginx/1.23.3
Server Hostname:        127.0.0.1
Server Port:            8000

Document Path:          /httptest/wikipedia_russia.html
Document Length:        954824 bytes

Concurrency Level:      10
Time taken for tests:   4.649 seconds
Complete requests:      10000
Failed requests:        0
Total transferred:      9550620000 bytes
HTML transferred:       9548240000 bytes
Requests per second:    2151.19 [#/sec] (mean)
Time per request:       4.649 [ms] (mean)
Time per request:       0.465 [ms] (mean, across all concurrent requests)
Transfer rate:          2006363.50 [Kbytes/sec] received

Connection Times (ms)
              min  mean[+/-sd] median   max
Connect:        0    0   0.1      0       2
Processing:     2    5   0.8      4      28
Waiting:        0    0   0.7      0      26
Total:          2    5   0.8      5      29
WARNING: The median and mean for the processing time are not within a normal deviation
        These results are probably not that reliable.

Percentage of the requests served within a certain time (ms)
  50%      5
  66%      5
  75%      5
  80%      5
  90%      5
  95%      5
  98%      5
  99%      6
 100%     29 (longest request)
```