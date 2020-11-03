#!/usr/bin/env python
import pika

connection = pika.BlockingConnection( pika.URLParameters('amqp://sms.tst.serv:VgGu8coyB@vv-bunny-01.sirena-travel.ru:5672/%2f'))
channel = connection.channel()
channel.basic_qos(prefetch_count = 1)

def callback(ch, method, properties, body):
    print(" [x] Received %s %r" % (method.delivery_tag, body))

channel.basic_consume(callback,
                      queue='sms.tst.smsqueue',
                      no_ack=True)
print(' [*] Waiting for messages. To exit press CTRL+C')
channel.start_consuming()
