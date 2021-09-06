## How to get the varible from elk query
```

## check all indices

$  curl -XGET elastic:changeme@localhost:9200/_cat/indices?h=index
$  curl -X GET 'http://elastic:changeme@localhost:9200/_cat/indices?v'

## check spesific index

$  curl -XGET elastic:changeme@localhost:9200/new_index
$  curl -XGET elastic:changeme@localhost:9200/new_index?pretty=true

$  curl -XGET elastic:changeme@localhost:9200/new_index/_search


------------------------------



## See all json inputs

curl -XGET "elasticsearch:9200/new_index/_search" -H 'Content-Type: application/json' -d'
{
    "query": {
        "match_all": {}
    }
}'



------------------------------

## Get parameter by env :

curl -XGET elastic:changeme@localhost:9200/new_index/_search -H 'Content-Type: application/json' -d'
{
  "_source": ["sensor_longitude"]
}
'



-------------------

## Get parametrs by id :

curl -XGET elastic:changeme@localhost:9200/new_index/_search -H 'Content-Type: application/json' -d'
{
  "query": {
    "terms": {
      "_id": [ "2021-09-02", "2021-08-30"] 
    }
  }
}'


----------------------

## Including only selected fields using source filtering :

curl -XGET elastic:changeme@localhost:9200/new_index/_search -H 'Content-Type: application/json' -d'
{
  "query": {
    "match_all": {}
  },
  "_source": {
       "includes": ["event_start_time_utc", "mission_id", "target_width"]
  }
}'


----------------------------

## Get Index json by name and id :

curl -XGET "elastic:changeme@localhost:9200/_mget" -H 'Content-Type: application/json' -d'
{
  "docs": [
    {
      "_index": "new_index",
      "_id": "2021-09-02"
    }
  ]
}'

---------------------------

## ## Get Index json by name id and _source :


curl -XGET "elastic:changeme@localhost:9200/_mget" -H 'Content-Type: application/json' -d'
{
  "docs": [
    {
      "_index": "new_index",
      "_id": "1",
      "_source": ["sensor_longitude"]
    }
  ]
}'

----------------------------

## Filter source fields

curl -XGET "elastic:changeme@localhost:9200/_mget" -H 'Content-Type: application/json' -d'
{
  "docs": [
    {
      "_index": "new_index",
      "_type": "_doc",
      "_id": "2021-09-02",
      "_source": [ "event_start_time_utc" ]
    }
  ]
}'



```




b'{"checksum":18726,"event_start_time_utc":"23-11-2213-T13:58:29.+0200IST","frame_center_elevation":-900.0,"frame_center_latitude":32.79167355633884,"frame_center_longitude":34.96762375112978,"image_coordinate_system":"Geodetic WGS84","image_source_sensor":"EOW","mission_id":"Video source EOW for UAV 12 from GCS 1 video start time 7/1/2021 8:19:35 AM","platform_designation":"Hermes 900 HFE D-12","platform_heading_angle":288.28564453125,"platform_pitch_angle":0.0,"platform_roll_angle":-9.376811981201172,"precision_time_stamp":"07-01-2021-T11:16:46.+0200IST","security_local_set":{"1":"\\u0002","12":"\\u0004","13":"//SZ","2":"\\u0003","22":"10","3":"//SZ","4":"Security- SCI/SHI information","5":"FOR OFFICIAL USE ONLY","6":"Releasing Instructions"},"sensor_horizontal_field_of_view":40.52918243408203,"sensor_latitude":32.785903915197544,"sensor_longitude":34.96158215913995,"sensor_relative_azimuth_angle":110.74662389949584,"sensor_relative_elevation_angle":-4.419317880840654,"sensor_relative_roll_angle":0.1249713264696699,"sensor_true_altitude":-900.0,"sensor_vertical_field_of_view":22.794231414794922,"slant_range":858.5560137542328,"target_width":0.0,"uas_datalink_ls_version_number":10}'
