#!/bin/bash

echo "Start docker-compose .." && sleep 1
cd docker-elkk/ && sudo docker-compose up -d 
sleep 2
echo "The app is ready. Elk is up" 
