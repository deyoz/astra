#!/usr/bin/env python
import pika

connection = pika.BlockingConnection( pika.URLParameters('amqp://sms.tst.serv:VgGu8coyB@vv-bunny-02.sirena-travel.ru:5672/%2f'))
channel = connection.channel()
channel.basic_qos(prefetch_count = 1)

def callback(ch, method, properties, body):
    print(" [x] Received %s %r" % (method.delivery_tag, body))
    print(method)
    '''
    if int(method.delivery_tag) == 5:
        print("nack")
        ch.basic_nack(method.delivery_tag, True)
    elif int(method.delivery_tag) == 10:
        ch.basic_ack(method.delivery_tag, True)
    else:
        ch.basic_ack(method.delivery_tag, True)
        '''







channel.basic_consume(callback,
                      queue='sms.tst.tstqueue',
                      no_ack=True)


print(' [*] Waiting for messages. To exit press CTRL+C')
channel.start_consuming()
