
### ��⠭���� ID ��㯯� � ���짮��⥫� �� ����� ���ண� �㤥� ����� ����� ���⥩���
``` bash
# cat .env
UID=54323
GID=54324
```

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


mv .env .env.1
docker-compose run astra ./buildFromScratch.sh no/ora --createtcl --createdb
mv .env.1 .env
docker-compose  up -d
Recreating astra_astra_1 ... done
