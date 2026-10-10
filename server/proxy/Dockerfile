FROM python:3.12-slim
WORKDIR /app
COPY requirements.txt .
RUN pip install --no-cache-dir -r requirements.txt
COPY *.py .
# The server's system prompt and JSON schema (versioned files).
COPY prompts/ prompts/
# The database lives on a mounted volume so it survives redeploys.
ENV DB_PATH=/data/proxy.db
# Firmware images for over-the-air updates live on the same volume (admin.py firmware).
ENV FIRMWARE_DIR=/data/firmware
# Never run as root.
RUN useradd --create-home --uid 10001 proxy && mkdir -p /data && chown proxy /data
USER proxy
# ONE worker: the burst limiter is in memory and SQLite has one writer.
CMD ["uvicorn", "app:app", "--host", "0.0.0.0", "--port", "8000", "--workers", "1", "--proxy-headers", "--forwarded-allow-ips", "*"]
