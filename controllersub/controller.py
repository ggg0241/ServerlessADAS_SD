import paho.mqtt.client as mqtt
import time
import picar_4wd as fc

# Define the MQTT broker address and port
BROKER = "localhost"  # Replace with your MQTT broker's IP or hostname
PORT = 1883

# The callback function when a message is received from the MQTT broker
def on_message(client, userdata, message):
    start_time = time.time()
    topic = message.topic
    payload = message.payload.decode("utf-8")

    print(f"Received message: {payload} on topic: {topic}")

    # Check the received message and call corresponding functions
    if payload == "stop":
        fc.turn_right(25)
        time.sleep(1.29)
        fc.stop()
        fc.forward(50)
        end_time = time.time()
        print(end_time-start_time)
    #elif payload == 35:
    #    fc.forward(35)
    #elif payload == 25:
    #    fc.forward(25)
    #elif payload == 15:
    #    fc.forward(15)
    #else:
    #    fc.forward(50)

# Functions to be invoked based on the received messagon 3 invoked!")

def main():
    # Create a MQTT client
    client = mqtt.Client()

    # Assign the on_message callback function
    client.on_message = on_message

    # Connect to the MQTT broker
    client.connect(BROKER, PORT, 60)

    # Subscribe to the "controller" topic
    client.subscribe("controller")

    # Start the MQTT client loop to receive messages
    client.loop_forever()

if __name__ == "__main__":
    main()