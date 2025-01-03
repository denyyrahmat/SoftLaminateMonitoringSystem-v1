import paho.mqtt.client as mqtt
  
def on_connect(client, userdata, flags, rc):
  print("Connected with result code "+str(rc))
  client.subscribe("test")
  
def on_message(client, userdata, msg):
  result = msg.payload.decode("utf-8")
  print("message: " + result)
  
client = mqtt.Client()

client.on_connect = on_connect
client.on_message = on_message
client.connect("localhost", 1883, 60)

client.loop_forever()