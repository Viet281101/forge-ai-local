#!/bin/bash
# runtime/test_diagnostic.sh - Diagnostic test script

SOCKET="/tmp/forge-ai.sock"

echo "==> Diagnostic Test Suite"
echo ""

# Test 1: Ping
echo "[1/4] Testing ping..."
RESPONSE=$(echo '{"version":1,"action":"ping"}' | socat - UNIX-CONNECT:$SOCKET 2>&1)
echo "Response: $RESPONSE"
echo ""

# Test 2: Model Info
echo "[2/4] Testing model_info..."
RESPONSE=$(echo '{"version":1,"action":"model_info"}' | socat - UNIX-CONNECT:$SOCKET 2>&1)
echo "Response: $RESPONSE"
echo ""

# Test 3: Simple generation (very short)
echo "[3/4] Testing simple generation (10 tokens max)..."
printf '{"version":1,"action":"generate","prompt":"Hello","max_tokens":10,"temperature":0.7}' > /tmp/test_req.json
cat /tmp/test_req.json
echo ""
echo "Waiting for response (may take 10-20 seconds for first generation)..."
# Use socat with explicit timeout
RESPONSE=$(timeout 60s socat -T60 - UNIX-CONNECT:$SOCKET < /tmp/test_req.json 2>&1)
EXIT_CODE=$?
echo "Exit code: $EXIT_CODE"
if [ $EXIT_CODE -eq 124 ]; then
    echo "✗ Timeout after 60s"
elif [ $EXIT_CODE -eq 0 ]; then
    echo "✓ Success"
fi
echo "Response: $RESPONSE"
echo ""

# Test 4: Check if server is still alive
echo "[4/4] Checking if server is alive..."
sleep 1
RESPONSE=$(echo '{"version":1,"action":"ping"}' | socat - UNIX-CONNECT:$SOCKET 2>&1)
if [ $? -eq 0 ]; then
    echo "✓ Server is still alive"
    echo "Response: $RESPONSE"
else
    echo "✗ Server crashed"
fi
echo ""

echo "==> Diagnostic complete"
