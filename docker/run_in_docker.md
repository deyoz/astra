

### ���樠������ ��४�ਨ run
docker-compose -f docker/docker-compose.yaml run astra docker/init_run.sh

### �������� �� (�ॡ���� �믮����� ��������)

``` bash
docker-compose -f docker/docker-compose.yaml run astra ./buildFromScratch.sh no/ora --createtcl --createdb
```

### ����� �����
``` bash
docker-compose -f docker/docker-compose.yaml pull
docker-compose -f docker/docker-compose.yaml up -d
 ```

### ���������� �����
``` bash
docker-compose -f docker/docker-compose.yaml pull
docker-compose -f docker/docker-compose.yaml up -d
 ```
