import json
import paho.mqtt.client as mqtt

TOPIC = 'controller'
PORT = 1883
# third ip address found using 'hostname -I'
BROKER = "172.17.0.1"

def handle(event):
    message = event.decode()
    print("Received message: {}".format(message))
    # parses the input to get the distance (only useful for adaptive cruise control application)
    #test = message.split()
    #test1 = test[3]
    #distance = int(test1[:-1])

    # right now only publishes 'stop' but can be worked on
    msg = 'stop'
    client = mqtt.Client()

    client.connect(BROKER, PORT, 60)
    client.publish(TOPIC, msg)

    return None
