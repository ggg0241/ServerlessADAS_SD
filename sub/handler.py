import json
import paho.mqtt.client as mqtt

TOPIC = 'controller'
PORT = 1883
BROKER = "172.17.0.1" #mosquitto broker IP

def handle(event):
    message = event.decode()
    print("Received message: {}".format(message))
    msg = 'stop'
    client = mqtt.Client()

    client.connect(BROKER, PORT, 60)
    client.publish(TOPIC, msg)

    return None
