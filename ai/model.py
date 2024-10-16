import os
import numpy as np
import tensorflow as tf
from tensorflow.keras.models import Sequential
from tensorflow.keras.layers import Conv2D, MaxPooling2D, Flatten, Dense, Dropout
from tensorflow.keras.layers import BatchNormalization

# Constants
IMAGE_SIZE = 224
BATCH_SIZE = 10
EPOCHS = 30
DATA_DIR = 'Spectrograms'
CLASSES = ('backward', 'go', 'left', 'right', 'stop')

# Configure GPU memory growth
physical_devices = tf.config.list_physical_devices('GPU')
if physical_devices:
    for gpu in physical_devices:
        tf.config.experimental.set_memory_growth(gpu, True)

# Load and preprocess images
def load_and_preprocess_image(file_path, label):
    image = tf.io.read_file(file_path)
    image = tf.image.decode_jpeg(image, channels=3)
    image = tf.image.resize(image, [IMAGE_SIZE, IMAGE_SIZE])
    image /= 255.0  # Normalize to [0, 1]
    return image, label

# Create dataset
def create_dataset(data_dir, classes):
    file_paths = []
    labels = []

    for index, label in enumerate(classes):
        class_dir = os.path.join(data_dir, label)
        for file in os.listdir(class_dir):
            file_paths.append(os.path.join(class_dir, file))
            labels.append(index)

    dataset = tf.data.Dataset.from_tensor_slices((file_paths, labels))
    dataset = dataset.map(load_and_preprocess_image, num_parallel_calls=tf.data.experimental.AUTOTUNE)
    dataset = dataset.shuffle(buffer_size=1000).batch(BATCH_SIZE).prefetch(tf.data.experimental.AUTOTUNE)

    return dataset

# Define the CNN model
def build_model(input_shape):
    model = Sequential([
        Conv2D(32, (3, 3), activation='relu', input_shape=input_shape),
	BatchNormalization(),
        MaxPooling2D(2, 2),
        Conv2D(128, (3, 3), activation='relu'),
	BatchNormalization(),
        MaxPooling2D(2, 2),
        Conv2D(128, (3, 3), activation='relu'),
	BatchNormalization(),
        MaxPooling2D(2, 2),
        Conv2D(128, (3, 3), activation='relu'),
	BatchNormalization(),
        MaxPooling2D(2, 2),
	Dropout(0.5),
        Flatten(),
        Dense(1024, activation='relu'),
        Dense(len(CLASSES), activation='softmax')
    ])
    return model

# Train the model
def train_model(model, train_dataset, test_dataset):
    model.compile(optimizer=tf.keras.optimizers.Adam(learning_rate=0.0001),
                  loss='sparse_categorical_crossentropy',
                  metrics=['accuracy'])

    model.fit(train_dataset, validation_data=test_dataset, epochs=EPOCHS)

# Create datasets
dataset = create_dataset(DATA_DIR, CLASSES)
train_size = int(0.8 * len(dataset))
train_dataset = dataset.take(train_size)
test_dataset = dataset.skip(train_size)

# Build and train model
model = build_model((IMAGE_SIZE, IMAGE_SIZE, 3))
train_model(model, train_dataset, test_dataset)

# Save the final model
model.save('model.keras')
