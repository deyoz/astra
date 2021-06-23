
### Установка ID группы и пользователя от имени которого будет запуск внутри контейнера
``` bash
# cat .env
UID=54323
GID=54324
```

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


mv .env .env.1
docker-compose run astra ./buildFromScratch.sh no/ora --createtcl --createdb
mv .env.1 .env
docker-compose  up -d
Recreating astra_astra_1 ... done
