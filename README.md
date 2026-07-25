# memk

Distributed monitoring and analysis protocol prototype. Educational implementation of a centralized event processing pipeline for endpoint data.

## Architecture

The system consists of three independent components connected by a simple TCP/JSON protocol:

- client-agent — C++ service for endpoint event capture
- analysis-backend — Python processor for event ingestion, OCR, and NLP analysis
- ai-detection — optional anomaly scoring module

### Client Agent (C++)

Lightweight Windows service that captures endpoint events and forwards them to the backend. Built with WinAPI, Winsock, and GDI+.

Responsibilities:
- Keyboard hook monitoring
- Screenshot capture on configurable intervals
- Clipboard and USB activity detection
- Event buffering and TCP transmission to backend

Design:
- Runs as a background service with minimal footprint
- Buffers events locally to handle network interruptions
- Sends JSON envelopes over TCP to the backend ingest port

### Analysis Backend (Python)

Central processing hub that receives events from agents and runs analysis pipelines.

Stack:
- Kafka for decoupled message ingestion
- Tesseract OCR for image text extraction
- spaCy for NLP and PII detection
- TextBlob for sentiment analysis

The backend exposes two interfaces:
- HTTP/WebSocket on port 3001 for health checks and real-time status
- TCP on port 8080 for direct agent event ingestion (newline-delimited JSON)

Processing flow:
1. Receive raw event envelope from agent
2. Extract content: run OCR on screenshots, parse text from clipboard
3. Run NLP pipeline: keyword detection, entity recognition, sentiment
4. Forward flagged content to AI detection module
5. Emit alert with score and context

### AI Detection

Optional module that wraps a local LLM (Llama 2 GGUF via llama-cpp-python) for anomaly scoring.

Given a text sample, it returns a float between 0 and 1 indicating how anomalous the content is for a workplace context. Used as a secondary signal after OCR/NLP flagging.

## Protocol

Event envelope format (JSON, newline-delimited over TCP):

{
  "source": "agent-id",
  "timestamp": "2026-01-01T00:00:00Z",
  "type": "keystroke|screenshot|clipboard|usb",
  "payload": { ... }
}

Backend responses:
- 200 OK with JSON receipt
- 400 for malformed envelopes

## Quick Start

1. Create virtual environment and install dependencies:
   cd analysis-backend
   pip install -r requirements.txt

2. Start backend:
   python simple_server.py

3. Build and run client agent (Windows):
   cd client-agent
   cl /std:c++17 main.cpp ws2_32.lib gdiplus.lib ole32.lib
   ./main.exe

4. Run demo client (Python fallback):
   cd client-agent
   python simple_client.py

Run the launcher script on Windows:
  run.cmd        # starts backend + demo client
  run.cmd server # backend only
  run.cmd client # demo client only
  run.cmd check  # verify environment

## Project Structure

memk/
  client-agent/
    main.cpp           — C++ agent entry point
    simple_client.cpp  — C++ TCP client
    simple_client.py   — Python TCP client (demo)
  analysis-backend/
    main.py            — Full backend with Kafka/OCR/NLP
    simple_server.py   — Minimal HTTP/TCP demo server
    requirements.txt   — Python dependencies
  ai-detection/
    ai_detection.py    — Local LLM anomaly scorer
    test_anomaly.py    — Unit tests
  plans/
    architecture.md    — Detailed system design
  run.cmd              — Windows launcher

## Design Notes

- The backend is intentionally split into main.py (full pipeline) and simple_server.py (demo) so the protocol can be tested without Kafka or Tesseract installed.
- The C++ agent is a structural reference: keyboard hook and screenshot capture are implemented, while clipboard/USB hooks are placeholders.
- AI detection requires a GGUF model file in models/. The module is optional; the backend operates without it.

## Limitations

- No authentication or encryption on the TCP ingest channel (intentional for prototype clarity)
- Single-backend setup, no clustering
- Windows-only agent (WinAPI dependencies)
- No persistent storage in the demo server (in-memory only)
