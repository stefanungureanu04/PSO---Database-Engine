#!/bin/bash

# Start server in background
./server 8081 &
SERVER_PID=$!
sleep 1

# Create a table
echo "CREATE TABLE test (id INT, name VARCHAR(20))" | ./client 127.0.0.1 8081
sleep 0.5

# Insert data from multiple clients concurrently
for i in {1..5}; do
    echo "INSERT INTO test VALUES ($i, 'User$i')" | ./client 127.0.0.1 8081 &
done

wait

# Select data
echo "SELECT * FROM test" | ./client 127.0.0.1 8081

# Test Backup
echo "BACKUP" | ./client 127.0.0.1 8081

# Cleanup
kill $SERVER_PID
