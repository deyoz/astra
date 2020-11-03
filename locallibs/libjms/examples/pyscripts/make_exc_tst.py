#!/usr/bin/env python
import pika

connection = pika.BlockingConnection(pika.URLParameters('amqp://sms.tst.cba:LQqG5kNSj@vv-bunny-01.sirena-travel.ru:5672/%2f'))
print ('connected')
channel = connection.channel()
print ('channel created')

exch='sms.tst.tstexch'
q='sms.tst.tstqueue'
k="tstdata"
channel.exchange_delete(exchange=exch)
print ('exchange deleted')
channel.queue_delete(queue=q)
print ('queue deleted')
channel.exchange_declare(exchange=exch, type='direct',durable=True)
print ('exchange declared')
channel.queue_declare(queue=q, durable=True)
print ('queue declared')
channel.queue_bind(exchange=exch, queue=q, routing_key=k)
print ('queue binded')

connection.close()
