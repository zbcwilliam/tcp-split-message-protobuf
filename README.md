# tcp-split-message-protobuf
# 111
# 222
# 333


# b3-1

# m4-1
tcp split message in receive side; the rpc is protobuf 2; message struct: header+message (header is a int type, indicating the message length)

dependency: protobuf2

reference: http://www.ideawu.net/blog/archives/1027.html

pseudocode：

char tmp[];
Buffer buffer;
// 网络循环：必须在一个循环中读取网络，因为网络数据是源源不断的。
while(1){

    // 从TCP流中读取不定长度的一段流数据，不能保证读到的数据是你期望的长度
    
    tcp.read(tmp);
    
    // 将这段流数据和之前收到的流数据拼接到一起
    
    buffer.append(tmp);
    
    // 解析循环：必须在一个循环中解析报文，应对所谓的粘包
    
    while(1){
    
        // 尝试解析报文
        
        msg = parse(buffer);
        
        if(!msg){
        
            // 报文还没有准备好，糟糕，我们遇到拆包了！跳出解析循环，继续读网络。
            
            break;
            
        }
        
        // 将解析过的报文对应的流数据清除
        
        buffer.remove(msg.length);
        
        // 业务处理
        
        process(msg);
        
    }
    
}


step1: proto file to .cc and .h

protoc $PROTO_PATH --cpp_out=. Test.proto

step2: server

g++ serverMultiple.cpp Test.pb.cc -o serverMultiple -lprotobuf -lpthread

step3: client

g++ client.cpp Test.pb.cc -o client -lprotobuf -lpthread

step4:

./serverMultiple

step5:

./client 
