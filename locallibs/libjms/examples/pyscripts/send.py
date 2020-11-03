#!/usr/bin/env python
import pika

connection = pika.BlockingConnection(pika.URLParameters('amqp://sms.tst.cba:LQqG5kNSj@vv-bunny-02.sirena-travel.ru:5672/%2f'))
channel = connection.channel()
exch='sms.tst.tstexch'
q='sms.tst.tstqueue'
k="tstdata"
for i in range(1,10):
    channel.basic_publish(exchange=exch,
            routing_key=k,
            body='Hello World! %d'%i )
    print(" [x] Sent 'Hello World! %d'" %i )
connection.close()
