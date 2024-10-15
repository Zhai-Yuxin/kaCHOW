from tensorflow.keras.models import load_model
import numpy as np
import os

# Labels

classes = classes = ('backward', 'go', 'left', 'right', 'stop') # 5 classes

# Load model

loaded_model = load_model('model.keras')

# Load sample image

from keras.preprocessing import image
from tensorflow.keras.applications import MobileNetV2
from tensorflow.keras.applications.mobilenet import preprocess_input

def load_sample(sample):
    x = image.load_img(f'Spectrograms/samples/{sample}.png', target_size=(224, 224))
    x = image.img_to_array(x)
    x = np.expand_dims(x, axis=0)
    x = preprocess_input(x)

    return x

sample = input('Sample number: ')
x = load_sample(f'sample{sample}')

# Predict class

base_model = MobileNetV2(weights='imagenet', include_top=False, input_shape=(224, 224, 3))
y = base_model.predict(x)

predictions = loaded_model.predict(y)

print(f"Evaluating and classifying Sample {sample}")

for i, label in enumerate(classes):
    print(f'{label}: {predictions[0][i]}')

for i, label in enumerate(classes):
    if predictions[0][i] == max(predictions[0]):
        print(label)

