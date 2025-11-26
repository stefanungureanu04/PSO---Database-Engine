#!/bin/bash

# Start server
./server 8082 &
SERVER_PID=$!
sleep 1

# Test Multi-DB and Case Insensitivity
# 1. Create DB
# 2. Use DB
# 3. Create Table
# 4. Insert (Mixed Case)
# 5. Select (Mixed Case)
# 6. Create another DB and switch
# 7. Drop DB

(
echo "CREATE DATABASE db1"
echo "USE db1"
echo "CREATE TABLE t1 (id INT, val VARCHAR(10))"
echo "InSeRt InTo t1 VaLuEs (1, 'test')"
echo "sElEcT * fRoM t1"
echo "CREATE DATABASE db2"
echo "USE db2"
echo "SELECT * FROM t1" # Should fail or be empty if table doesn't exist in db2
echo "DROP DATABASE db1"
echo "EXIT"
) | ./client 127.0.0.1 8082

# Cleanup
kill $SERVER_PID
rm -rf data/db1 data/db2
