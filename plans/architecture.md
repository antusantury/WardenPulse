# Sentinel DLP System Architecture Design

## Introduction
The Sentinel DLP (Data Loss Prevention) system is designed as an educational project to demonstrate enterprise security principles. It focuses on preventing unauthorized data exfiltration through monitoring, analysis, and alerting. Key design goals include high performance for real-time processing, low resource footprint to minimize impact on endpoints, and stealthiness to avoid detection by users or malicious actors.

## High-Level Architecture Overview
The system follows a distributed architecture with four main components:
- **Client Agent (C++)**: Lightweight endpoint monitoring agents deployed on user devices, with resilience features for persistence.
- **Analysis Backend (Python)**: Central processing hub using Kafka for messaging, OCR for image analysis (including chat parsing), and NLP for text processing.
- **AI Anomaly Detection (Local LLM)**: Machine learning component for detecting unusual data patterns.
- **Web Dashboard (React)**: User interface for administrators to view alerts, manage policies, and initiate incident response.

The architecture includes incident response & forensics modules for stealth remote remediation and deep content inspection. It emphasizes modularity, scalability, and security. Data flows from endpoints through secure channels to the backend for analysis, with results displayed on the dashboard.

## Component Details

### Client Agent (C++)
- **Responsibilities**: Monitors file system operations, clipboard activities, network traffic, and peripheral devices (USB, printers). Provides resilience against removal and supports remote remediation.
- **Design**: Implemented in C++ for high performance and low memory usage. Runs as a system service with minimal user visibility.
- **Key Features**:
  - Hooks into OS APIs for real-time monitoring.
  - Buffers events locally to reduce network overhead.
  - Encrypts data before transmission.
  - Supports policy-based filtering to minimize false positives.
  - **Resilience**: Includes a watchdog service that monitors the agent process and restarts it if terminated. Uses registry persistence (e.g., run keys or scheduled tasks) to ensure automatic startup after reboot, resistant to accidental user removal.
- **Footprint**: <50MB RAM, <5% CPU usage under normal load.

### Analysis Backend (Python)
- **Responsibilities**: Receives data from agents, performs content analysis including deep inspection of chat applications, and coordinates with AI detection.
- **Design**: Microservices architecture using Python for flexibility.
  - **Kafka**: Message queue for decoupling producers (agents) and consumers (analyzers).
  - **OCR Module**: Extracts text from images using libraries like Tesseract. Expanded for chat parsing: Targets window classes of apps like Telegram, WhatsApp, and Signal. Includes a "context reconstructor" that stitches screenshots together to form readable chat history logs for the compliance database.
  - **NLP Module**: Analyzes text for sensitive data patterns (PII, financial info) using spaCy or similar.
- **Key Features**:
  - Scalable processing with worker pools.
  - Caching for frequently accessed policies.
  - Asynchronous processing to handle bursts.

### AI Anomaly Detection (Local LLM)
- **Responsibilities**: Identifies anomalous data transfer patterns that may indicate breaches.
- **Design**: Integrates a local Large Language Model (e.g., Llama or GPT-J) for contextual analysis.
- **Key Features**:
  - Trained on enterprise data patterns.
  - Runs locally to maintain privacy and reduce latency.
  - Provides confidence scores for alerts.

### Web Dashboard (React)
- **Responsibilities**: Displays real-time alerts, historical data, policy management, and initiates incident response actions.
- **Design**: Single-page application using React for responsive UI.
- **Key Features**:
  - Real-time updates via WebSockets.
  - Role-based access control.
  - Visualization of data flows and incidents.
  - Initiates stealth remote remediation sessions.

## Incident Response & Forensics
- **Stealth Remote Remediation (Hidden VNC)**: Allows security officers to remotely access endpoints to "fix configuration issues" without disturbing the user. Implemented as a "hidden desktop" using virtual desktop APIs or HVNC (Hidden Virtual Network Computing) techniques, enabling admins to work in a background session while the user continues on the main desktop. Triggered from the dashboard via secure commands to the agent.
- **Forensics Logging**: Stores detailed chat histories and event logs in a compliance database for auditing and investigation.

## Data Flow
1. Client agent detects potential data exfiltration event (e.g., file copy to USB) or monitors chat applications.
2. Agent captures metadata (file path, size, hash), content samples, or screenshots of chat windows.
3. Data is encrypted and sent to Kafka topic.
4. Backend consumers process the message: OCR/NLP analysis, including chat parsing and context reconstruction.
5. If suspicious, data is passed to AI for anomaly scoring.
6. Alerts are generated and stored in database, with chat logs archived for forensics.
7. Dashboard polls or receives push notifications for updates.
8. For incident response, dashboard sends commands to agent for stealth remote remediation (hidden VNC session).

## Security Considerations
- **Encryption**: All inter-component communication uses TLS 1.3.
- **Authentication**: Mutual TLS for agents, JWT for dashboard.
- **Authorization**: Least privilege principles; agents have read-only access.
- **Stealthiness**: Agents use rootkit-like techniques (e.g., hiding processes), but legally compliant for educational purposes.
- **Data Privacy**: Minimal data collection; content analysis only on flagged items.
- **Threat Model**: Protects against insider threats, malware, and accidental leaks.

## Integration Points
- **APIs**:
  - REST API for policy updates from dashboard to backend.
  - gRPC for high-performance agent-backend communication.
  - WebSocket for real-time dashboard updates.
- **External Systems**: Integrates with SIEM tools via syslog, Active Directory for user context.