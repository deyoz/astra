#!/usr/bin/env python
import pika
exch='sms.tst.tstexch'
q='sms.tst.tstqueue'
k="tstdata"

connection = pika.BlockingConnection(pika.URLParameters('amqp://sms.tst.all:sms6483@vv-bunny-01.sirena-travel.ru:5672/%2f'))
channel = connection.channel()

channel.basic_qos(prefetch_count = 1)

channel.tx_select()
for i in range(1,10):
    channel.basic_publish(exchange=exch,
            routing_key=k,
            body='Hello World! %d'%i )
    print(" [x] Sent 'Hello World!'")
channel.tx_commit()

def callback(ch, method, properties, body):
    print(" [x] Received %s %r" % (method.delivery_tag, body))
    print(method)
    ch.basic_ack(method.delivery_tag, True)




channel2 = connection.channel()
channel2.basic_qos(prefetch_count = 1)

channel2.basic_consume(callback,
                      queue='sms.tst.tstqueue',
                      no_ack=False)


print(' [*] Waiting for messages. To exit press CTRL+C')
channel2.start_consuming()
