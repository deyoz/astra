

### Инициализация директории run
docker-compose -f docker/docker-compose.yaml run astra docker/init_run.sh

### Создание БД (требуется выполнить единожды)

``` bash
docker-compose -f docker/docker-compose.yaml run astra ./buildFromScratch.sh no/ora --createtcl --createdb
```

### Запуск Астры
``` bash
docker-compose -f docker/docker-compose.yaml pull
docker-compose -f docker/docker-compose.yaml up -d
 ```

### Обновление Астры
``` bash
docker-compose -f docker/docker-compose.yaml pull
docker-compose -f docker/docker-compose.yaml up -d
 ```
