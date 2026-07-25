"""
Analysis Backend for Sentinel DLP System

This module implements a data pipeline that processes screenshots from client agents via Kafka,
performs OCR with Tesseract, and conducts NLP analysis including keyword detection and sentiment analysis.

Scalability Notes:
- For high throughput, consider using multiprocessing or asyncio for parallel event processing.
- Kafka consumer can be configured with multiple partitions and consumer groups for load balancing.
- OCR processing can be offloaded to GPU-accelerated libraries like EasyOCR for better performance.
- Use caching for NLP models if memory allows.
"""

from kafka import KafkaConsumer, KafkaProducer
import pytesseract
import cv2
import spacy
import json
from PIL import Image
import io
import sys
import os
import re
import numpy as np
from textblob import TextBlob
sys.path.append(os.path.join(os.path.dirname(__file__), '..', 'ai-detection'))
from ai_detection import detect_anomaly

# Load NLP model
nlp = spacy.load("en_core_web_sm")

# Keywords for detection
SENSITIVE_KEYWORDS = ['секретно', 'пароль']

def preprocess_image(image_data):
    """Preprocess image for better OCR accuracy."""
    # Convert to numpy array
    image = Image.open(io.BytesIO(image_data))
    image_np = cv2.cvtColor(np.array(image), cv2.COLOR_RGB2BGR)

    # Convert to grayscale
    gray = cv2.cvtColor(image_np, cv2.COLOR_BGR2GRAY)

    # Apply Gaussian blur to reduce noise
    blurred = cv2.GaussianBlur(gray, (5, 5), 0)

    # Apply thresholding
    _, thresh = cv2.threshold(blurred, 0, 255, cv2.THRESH_BINARY + cv2.THRESH_OTSU)

    return Image.fromarray(thresh)

def extract_text_with_ocr(image_data):
    """Concrete example of OCR pipeline: preprocess and extract text."""
    preprocessed_image = preprocess_image(image_data)
    text = pytesseract.image_to_string(preprocessed_image, lang='rus+eng')  # Support Russian and English
    return text

def detect_keywords(text):
    """Detect sensitive keywords in the text."""
    found_keywords = []
    text_lower = text.lower()
    for keyword in SENSITIVE_KEYWORDS:
        if keyword.lower() in text_lower:
            found_keywords.append(keyword)
    return found_keywords

def analyze_sentiment(text):
    """Perform sentiment analysis on the text."""
    blob = TextBlob(text)
    return blob.sentiment.polarity  # Returns a float between -1 (negative) and 1 (positive)

# Kafka setup
consumer = KafkaConsumer('dlp-events', bootstrap_servers=['localhost:9092'], auto_offset_reset='earliest',
                         enable_auto_commit=True, group_id='dlp-group', value_deserializer=lambda x: json.loads(x.decode('utf-8')))

producer = KafkaProducer(bootstrap_servers=['localhost:9092'], value_serializer=lambda x: json.dumps(x).encode('utf-8'))

def process_event(event):
    # OCR processing with improved pipeline
    if 'image' in event:
        image_data = event['image']
        text = extract_text_with_ocr(image_data)

        # Chat parsing: Assume specific window classes, but for demo, process all
        # Context reconstructor: Stitch if multiple images
        chat_log = reconstruct_chat(text)

        # NLP analysis
        doc = nlp(text)
        sensitive_data = [ent.text for ent in doc.ents if ent.label_ in ['PERSON', 'ORG', 'GPE', 'MONEY']]

        # Keyword detection
        found_keywords = detect_keywords(text)

        # Sentiment analysis
        sentiment_score = analyze_sentiment(text)

        # Send to AI for anomaly
        anomaly_score = check_anomaly(text)

        alert = {
            'event_id': event['id'],
            'text': text,
            'chat_log': chat_log,
            'sensitive_data': sensitive_data,
            'found_keywords': found_keywords,
            'sentiment_score': sentiment_score,
            'anomaly_score': anomaly_score
        }

        producer.send('alerts', alert)

def reconstruct_chat(text):
    # Placeholder: Simple reconstruction
    return text.replace('\n', ' | ')

def check_anomaly(text):
    return detect_anomaly(text)

if __name__ == '__main__':
    for message in consumer:
        process_event(message.value)