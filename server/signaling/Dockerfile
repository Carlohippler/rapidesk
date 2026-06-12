FROM node:20-alpine

WORKDIR /app

# Install dependencies
COPY package.json package-lock.json* ./
RUN npm ci --only=production

# Copy source
COPY server.js ./

# Create non-root user
RUN addgroup -g 1001 -S rapiddesk && \
    adduser -S rapiddesk -u 1001
USER rapiddesk

# Expose WebSocket port
EXPOSE 8443

# Health check
HEALTHCHECK --interval=30s --timeout=3s --start-period=5s --retries=3 \
    CMD node -e "require('http').get('http://localhost:8443/health', (r) => r.statusCode === 200 ? process.exit(0) : process.exit(1))"

ENTRYPOINT ["node", "server.js"]