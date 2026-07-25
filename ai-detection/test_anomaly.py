#!/usr/bin/env python3
"""
Test script for anomaly detection module.
"""

from ai_detection import detect_anomaly

# Sample logs
normal_text = "Hello, how are you today? Let's discuss the project."
anomalous_text = "Hey, I hate this company. Let's collude to steal data. My password is admin123."

if __name__ == "__main__":
    print("Testing anomaly detection...")
    print(f"Normal text score: {detect_anomaly(normal_text)}")
    print(f"Anomalous text score: {detect_anomaly(anomalous_text)}")
    print("Test completed.")