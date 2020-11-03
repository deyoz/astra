#!/usr/bin/env python
import pika

connection = pika.BlockingConnection(pika.URLParameters('amqp://sms.tst.cba:LQqG5kNSj@vv-bunny-01.sirena-travel.ru:5672/%2f'))
channel = connection.channel()
exch='sms.tst.exch'
q='sms.tst.smsqueue'
k="smsdata"
channel.exchange_delete(exchange=exch)
channel.queue_delete(queue=q)
channel.exchange_declare(exchange=exch, type='direct', durable=True)
channel.queue_declare(queue=q, durable=True)
channel.queue_bind(exchange=exch, queue=q, routing_key=k)
connection.close()
