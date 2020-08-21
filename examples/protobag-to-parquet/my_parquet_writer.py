# Copyright 2020 Standard Cyborg
# 
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
# 
#     http://www.apache.org/licenses/LICENSE-2.0
# 
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

import protobag

from MyMessages_pb2 import DinoHunter
from MyMessages_pb2 import Position

if __name__ == '__main__':
  bag = protobag.Protobag(path='example_bag.zip')
  writer = bag.create_writer()

  max_hunter = DinoHunter(
          first_name='py_max',
          id=1,
          dinos=[
            {'name': 'py_nibbles', 'type': DinoHunter.PEOPLEEATINGSAURUS},
          ])
  writer.write_msg("hunters/py_max", max_hunter)

  lara_hunter = DinoHunter(
          first_name='py_lara',
          id=2,
          dinos=[
            {'name': 'py_bites', 'type': DinoHunter.PEOPLEEATINGSAURUS},
            {'name': 'py_stinky', 'type': DinoHunter.VEGGIESAURUS},
          ])
  writer.write_msg("hunters/py_lara", lara_hunter)

  # A Chase!
  for t in range(10):
    lara_pos = Position(x=t, y=t+1)
    writer.write_stamped_msg("positions/lara", lara_pos, t_sec=t)

    toofz_pos = Position(x=t+2, y=t+3)
    writer.write_stamped_msg("positions/toofz", toofz_pos, t_sec=t)


  # Use Raw API
  s = b"i am a raw string"
  writer.write_raw("raw_data", s)

  writer.close()
  print("Wrote to %s" % bag.path)
  




  path = 'example_bag.zip'
  print("Using protobag library %s" % protobag.__file__)
  print("Reading bag %s" % path)

  bag = protobag.Protobag(
          path=path,
          msg_classes=(
            DinoHunter,
            Position))
  rows = []
  for entry in bag.iter_entries():
    # ignore the index
    if '_protobag_index' in entry.entryname:
      continue
    
    print("Entry:")
    print(entry)
    print()
    print()

    row = protobag.DictRowEntry.from_entry(entry)
    rows.append(row)



  import pandas as pd
  import attr
  df = pd.DataFrame([
    # Convert to pyarrow-friendly types
    dict(
      entryname=row.entryname,
      type_url=row.type_url,
      msg_dict=row.msg_dict,
      topic=row.topic,
      timestamp=
        row.timestamp.ToDatetime() if row.timestamp else None,
      descriptor_data=
        row.descriptor_data.SerializeToString() if row.descriptor_data else None,
    )
    for row in rows
  ])
  print(df)
  print(df.info())
  print()

  import pyarrow as pa
  import pyarrow.parquet as pq
  table = pa.Table.from_pandas(df)
  pq.write_table(table, 'example.parquet')




  # Nope they don't have read support yet
  # table2 = pq.read_table('example.parquet')
  # df2 = table2.to_pandas()
  # print(df2)
  # print(df2.info())

  parquet_file = pq.ParquetFile('example.parquet')
  print(parquet_file.metadata)
  print(parquet_file.schema)