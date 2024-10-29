from tensorflow.keras.models import load_model
import numpy as np
from keras.preprocessing import image

# Labels
classes = ('backward', 'go', 'left', 'right', 'stop')  # 5 classes

# Load model
loaded_model = load_model('test_model.keras')

# Load sample image
def load_sample(sample):
    x = image.load_img(f'Spectrograms/samples/{sample}.png', target_size=(224, 224))
    x = image.img_to_array(x)
    x = np.expand_dims(x, axis=0)
    x /= 255.0  # Normalize to [0, 1] for your model
    return x

sample = input('Sample number: ')
x = load_sample(f'sample{sample}')

# Predict class using the loaded model
predictions = loaded_model.predict(x)

print(f"Evaluating and classifying Sample {sample}")

for i, label in enumerate(classes):
    print(f'{label}: {predictions[0][i]}')

# Get the label with the highest predicted probability
predicted_label_index = np.argmax(predictions[0])
predicted_label = classes[predicted_label_index]
print(f'Predicted label: {predicted_label}')
