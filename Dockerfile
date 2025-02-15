FROM debian:bullseye-slim

# Install build dependencies
RUN apt-get update && apt-get install -y \
    build-essential \
    clang \
    make \
    libpthread-stubs0-dev \
    && rm -rf /var/lib/apt/lists/*


WORKDIR /app
COPY . .
RUN make
RUN mkdir -p /var/www/html
EXPOSE 8080
CMD ["./httpd", "8080", "/var/www/html"]