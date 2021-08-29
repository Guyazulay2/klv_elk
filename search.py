import json
from elasticsearch import Elasticsearch
elastic_client = Elasticsearch(hosts=["http://elastic:changeme@localhost:9200"])
import pandas as pd




res = elastic_client.search(index="new_index", body={"query": {"match_all": {}}})
# print("Got %d Hits:" % res['hits']['total']['value'])
for hit in res['hits']['hits']:
    print(hit["_source"]["platform_heading_angle"])

