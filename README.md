## How to get the varible from elk query
```

 
## check all indices

$  curl -XGET elastic:changeme@localhost:9200/_cat/indices?h=index

## check spesific index

$  curl -XGET elastic:changeme@localhost:9200/new_index
$  curl -XGET elastic:changeme@localhost:9200/new_index?pretty=true


```

```
## Create index in elk 
$  res = elastic_client.index(index='new_index',doc_type="_doc", id="1", body=data)
        print(res['result'],res)

```
