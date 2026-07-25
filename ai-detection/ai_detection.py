# AI anomaly detection using local LLM (Llama)
# Uses llama-cpp-python for efficient local inference

from llama_cpp import Llama
import re

# Load the local Llama model (GGUF format)
# Assume model file is in models/ directory, e.g., llama-2-7b-chat.Q4_K_M.gguf
# Download from Hugging Face if needed
model_path = "models/llama-2-7b-chat.Q4_K_M.gguf"
llm = Llama(model_path=model_path, n_ctx=512, n_threads=4)  # Adjust n_ctx and n_threads for efficiency

def detect_anomaly(text):
    """
    Analyze text for anomalous behavior using local LLM.
    Returns a float between 0 and 1, where 1 indicates high anomaly.
    """
    prompt = f"""Analyze the following text for signs of anomalous behavior in a workplace setting, such as attempts at collusion, negative sentiment towards the company, or sharing sensitive information like passwords.

Text: "{text}"

Rate the anomaly level from 0 to 1, where 0 is completely normal and 1 is highly anomalous. Only return the number as a float (e.g., 0.75). Do not include any other text."""

    try:
        response = llm(prompt, max_tokens=10, temperature=0.1)  # Low temperature for consistent scoring
        response_text = response['choices'][0]['text'].strip()
        # Extract the number using regex
        match = re.search(r'(\d*\.?\d+)', response_text)
        if match:
            score = float(match.group(1))
            return min(max(score, 0), 1)  # Clamp between 0 and 1
        else:
            return 0.0  # Default to normal if parsing fails
    except Exception as e:
        print(f"Error in anomaly detection: {e}")
        return 0.0