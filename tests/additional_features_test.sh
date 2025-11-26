#!/bin/bash

# Start server
./server 8083 &
SERVER_PID=$!
sleep 1

# Test SHOW and LOG
# 1. Create DB
# 2. Use DB
# 3. Create Table
# 4. Show Databases
# 5. Show Tables
# 6. Log command
# 7. Check log content

(
echo "CREATE DATABASE db_show"
echo "USE db_show"
echo "CREATE TABLE t_show (id INT)"
echo "SHOW -DATABASES"
echo "SHOW -TABLES"
echo "LOG"
echo "EXIT"
) | ./client 127.0.0.1 8083

# Cleanup
kill $SERVER_PID
rm -rf data/db_show
rm server.log
