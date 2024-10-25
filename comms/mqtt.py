# mosquitto -c /etc/mosquitto/mosquitto.conf to start MQTT

import paho.mqtt.client as mqtt
from time import sleep
import pandas as pd
import numpy as np
from sklearn.pipeline import Pipeline
from sklearn.preprocessing import StandardScaler
from sklearn.model_selection import train_test_split
from sklearn.naive_bayes import GaussianNB
import wave
import os
import scipy.io.wavfile
from preprocessing import create_pngs_from_wavs
from tensorflow.keras.models import load_model
import numpy as np
from keras.preprocessing import image

classes = ('backward', 'go', 'left', 'right', 'stop') # 5 classes

loaded_model = load_model('new_model.keras')

def save_wav_file(file_path, audio_data):
    with open(file_path, 'wb') as wav_file:
        wav_file.write(audio_data)
    print(f"Audio saved to {file_path}")

# Load sample image
def load_sample():
    x = image.load_img(f'Spectrograms/recording.png', target_size=(224, 224))
    x = image.img_to_array(x)
    x = np.expand_dims(x, axis=0)
    x /= 255.0  # Normalize to [0, 1] for your model
    return x

def process():
    client.loop_stop()

    # Save the received audio data to a file
    save_wav_file('Sounds/recording.wav', audio_data)
    print("saved .wav file")

    create_pngs_from_wavs('Sounds/', 'Spectrograms/')

    #Predict
    predictions = loaded_model.predict(load_sample())
    print(f"Evaluating and classifying recording")

    for i, label in enumerate(classes):
        print(f'{label}: {predictions[0][i]}')
    predicted_label_index = np.argmax(predictions[0])
    predicted_label = classes[predicted_label_index]
    print(f'Predicted label: {predicted_label}')

    audio_data.clear()

    client.loop()

audio_data = bytearray()

def on_connect(client, userdata, flags, rc):
	print("Connected with result code: " + str(rc))
	client.subscribe("voice/#")

def on_message(client, userdata, message):
    topic = message.topic
    global audio_data
    # Append the received payload (audio chunk) to the buffer
    audio_data.extend(message.payload)
    print(f'message: ', message)
    if topic == "voice/stop":
        print(message.payload.decode('utf-8'))
        process()
    

print(audio_data)

client = mqtt.Client()
client.on_connect = on_connect
client.on_message = on_message
client.connect("localhost", 1883, 60)
client.loop_forever()




    
