
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
mv .env .env.1
docker-compose -f docker/docker-compose.yaml run astra ./bin/demo.sh
mv .env.1 .env
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
