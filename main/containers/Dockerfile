FROM gcc:13 AS dev
WORKDIR /app
# copy entrypoint, but NOT the source — we'll mount it
COPY script.sh /script.sh
RUN chmod +x /script.sh

ENTRYPOINT ["/script.sh"]