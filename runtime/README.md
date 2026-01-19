# Runtime build & debug

```bash
cd runtime

make            # build
make run        # build + run
make clean      # clean
make rebuild    # clean + build
make test       # send request test
```

### Test

```bash
echo '{
  "version": 1,
  "action": "infer",
  "messages": [
    {
      "role": "assistant",
      "tool_call": {
        "name": "list_dir",
        "arguments": { "path": "." }
      }
    }
  ]
}' | socat - UNIX-CONNECT:/tmp/forge-ai.sock
```
