#!/usr/bin/env python
import pika

connection = pika.BlockingConnection(
        pika.URLParameters('amqp://sms.tst.serv:VgGu8coyB@vv-bunny-02.sirena-travel.ru:5672/%2f'))
channel = connection.channel()


def callback(ch, method, properties, body):
    print(" [x] Received %s %r" % (method.delivery_tag, body))
    print(method)
#    ch.basic_ack(method.delivery_tag, True)
    ch.tx_rollback()
    ch.basic_recover(True)
    print ("rollback finish")

"""
    if int(method.delivery_tag) == 5:
        print("nack")
        ch.tx_commit()
    elif int(method.delivery_tag) == 10:
        ch.tx_rollback()
"""



channel.tx_select()
#channel.basic_recover()
channel.basic_qos(prefetch_count = 1)
channel.basic_consume(callback,
                      queue='sms.tst.tstqueue',
                      no_ack=False)


print(' [*] Waiting for messages. To exit press CTRL+C')
channel.start_consuming()
