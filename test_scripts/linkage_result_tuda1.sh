#!/bin/sh
curl -v -X POST -H "Expect:" -H "Content-Type: application/json" --data @configurations/linkage_result_TUDA1.json 'http://127.0.0.1:5000/linkageResult/TUDA1/TUDA2'
