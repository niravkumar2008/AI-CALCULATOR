FROM python:3.12-slim
WORKDIR /app
COPY requirements.txt .
RUN pip install --no-cache-dir -r requirements.txt
COPY *.py .
# The database lives on a mounted volume so it survives redeploys.
ENV DB_PATH=/data/proxy.db
# Firmware images for over-the-air updates live on the same volume (admin.py firmware).
ENV FIRMWARE_DIR=/data/firmware
CMD ["uvicorn", "app:app", "--host", "0.0.0.0", "--port", "8000", "--proxy-headers"]
