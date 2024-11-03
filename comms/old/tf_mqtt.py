# mosquitto -c /etc/mosquitto/mosquitto.conf to start MQTT

import wave
import pathlib
import paho.mqtt.client as mqtt
import numpy as np
import tensorflow as tf
from tensorflow.keras.models import load_model
import json
import os
from datetime import datetime, timedelta
import boto3

DATASET_PATH = 'Sounds'

voice_command_classes = ('backward', 'go', 'left', 'right', 'stop', 'none') # 6 classes
emo_classes = ('cry', 'none') # 2 classes

voice_command_model = load_model('tf_model.keras')
emo_model = load_model('tf_emo_model.keras')

dynamodb = boto3.resource('dynamodb') 
table = dynamodb.Table('kachow_test') 

s3 = boto3.client('s3')

def upload_to_dynamodb(recognized_command):
    utc_now = datetime.utcnow()
    gmt_plus_8 = utc_now + timedelta(hours=8)
    timestamp = gmt_plus_8.strftime('%Y-%m-%d/%H:%M:%S')
    s3.upload_file('Sounds/recording.wav', 'kachow-bucket', 'Data/' + timestamp +'.wav') 
    url = 'https://kachow-bucket.s3.ap-southeast-1.amazonaws.com/Data/' + timestamp + '.wav'
    table.put_item(Item={ 
        'timestamp': timestamp,
        'command': recognized_command,
        'url':url
    })

def save_wav_file(file_path, audio_data):
    with wave.open(file_path, 'wb') as wav_file:
        wav_file.setnchannels(1)
        wav_file.setsampwidth(2)
        wav_file.setframerate(16000)
        wav_file.writeframesraw(audio_data)
    print(f"Audio saved to {file_path}")

def get_spectrogram(waveform):
  # Convert the waveform to a spectrogram via a STFT.
  spectrogram = tf.signal.stft(
      waveform, frame_length=255, frame_step=128)
  # Obtain the magnitude of the STFT.
  spectrogram = tf.abs(spectrogram)
  # Add a `channels` dimension, so that the spectrogram can be used
  # as image-like input data with convolution layers (which expect
  # shape (`batch_size`, `height`, `width`, `channels`).
  spectrogram = spectrogram[..., tf.newaxis]
  return spectrogram

# Load sample image
def load_sample():
    data_dir = pathlib.Path(DATASET_PATH)
    x = data_dir/'recording.wav'
    x = tf.io.read_file(str(x))
    x, sample_rate = tf.audio.decode_wav(x, desired_channels=1, desired_samples=16000)
    x = tf.squeeze(x, axis=-1)
    waveform = x
    x = get_spectrogram(x)
    x = x[tf.newaxis,...]

    return x

def voice_command_process():
    client.loop_stop()

    # Save the received audio data to a file
    save_wav_file('Sounds/recording.wav', audio_data)
    print("saved .wav file")

    utc_now = datetime.utcnow()
    gmt_plus_8 = utc_now + timedelta(hours=8)
    timestamp = gmt_plus_8.strftime('%Y-%m-%d/%H:%M:%S')
    print(timestamp)
    
    #Predict
    predictions = voice_command_model.predict(load_sample())
    print(f"Evaluating and classifying recording")
 
    probability = tf.nn.softmax(predictions[0])

    for i, label in enumerate(voice_command_classes):
        print(f'{label}: {probability.numpy()[i] * 100}%')

    if (max(probability.numpy()) * 100 < 50):
        print("try again")
    else:
        predicted_label_index = np.argmax(predictions[0])
        predicted_label = voice_command_classes[predicted_label_index]
        print(f'Predicted label: {predicted_label}')
        upload_to_dynamodb(predicted_label)

    audio_data.clear()

    client.loop()

def emo_process():
    client.loop_stop()

    # Save the received audio data to a file
    save_wav_file('Sounds/recording.wav', audio_data)
    print("saved .wav file")

    utc_now = datetime.utcnow()
    gmt_plus_8 = utc_now + timedelta(hours=8)
    timestamp = gmt_plus_8.strftime('%Y-%m-%d/%H:%M:%S')
    print(timestamp)
    
    #Predict
    predictions = emo_model.predict(load_sample())
    print(f"Evaluating and classifying recording")
 
    probability = tf.nn.softmax(predictions[0])

    for i, label in enumerate(emo_classes):
        print(f'{label}: {probability.numpy()[i] * 100}%')

    if (max(probability.numpy()) * 100 < 60):
        print("try again")
    else:
        predicted_label_index = np.argmax(predictions[0])
        predicted_label = emo_classes[predicted_label_index]
        print(f'Predicted label: {predicted_label}')
        upload_to_dynamodb(predicted_label)

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
    if topic == 'voice/wav':  
        audio_data.extend(message.payload)
        print(f'message: ', message)
    elif topic == "voice/stop":
        stop_message = message.payload.decode('utf-8')
        print(stop_message)
        if stop_message == "check voice command":
            voice_command_process()
        elif stop_message == "check crying":
            emo_process()
    
print(audio_data)

client = mqtt.Client()
client.on_connect = on_connect
client.on_message = on_message
client.connect("localhost", 1883, 60)
client.loop_forever()