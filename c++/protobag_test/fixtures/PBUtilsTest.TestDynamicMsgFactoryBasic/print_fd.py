
# To generate: `protoc moof.proto --python_out=.`
from moof_pb2 import Moof

m = Moof(x="i am a dogcow")

print("BEGIN MOOF TEXT FORMAT")
print(m)
print("END MOOF TEXT FORMAT")

from google.protobuf import descriptor_pb2
fd = descriptor_pb2.FileDescriptorProto()
Moof.DESCRIPTOR.file.CopyToProto(fd)
print("BEGIN MOOF FILEDESCRIPTORPROTO TEXT FORMAT")
print(fd)
print("END MOOF FILEDESCRIPTORPROTO TEXT FORMAT")
