# 编译器和编译选项
CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -g `pkg-config --cflags protobuf` 

# 头文件目录（仅用于编译阶段）
INCLUDES = -I./coroutine -I./my_protobuf_analysis -I./log -I./timer -I./mysql -I./webserver_reactor -I./my_zookeeper -I./my_scheduler

# 查找Protobuf生成的源文件
PROTOBUF_SRCS = $(wildcard my_protobuf_analysis/*.pb.cc)
PROTOBUF_OBJS = $(PROTOBUF_SRCS:.cc=.o)

# 通用源文件（客户端和服务器共享，新增timer相关文件）
COMMON_SRCS = $(wildcard my_protobuf_analysis/*.cpp log/*.cpp my_zookeeper/*.cpp \
               webserver_reactor/*.cpp mysql/*.cpp my_scheduler/*.cpp coroutine/*.cpp timer/*.cpp) $(PROTOBUF_SRCS)
COMMON_OBJS = $(COMMON_SRCS:.cpp=.o)
COMMON_OBJS := $(COMMON_OBJS:.cc=.o) 

# 服务器特定源文件和目标文件（仅保留my_rpc_server相关文件）
SERVER_SRCS = $(wildcard my_rpc_server/*.cpp)
SERVER_OBJS = $(SERVER_SRCS:.cpp=.o)

# 客户端特定源文件和目标文件
CLIENT_SRCS = $(wildcard my_rpc_client/*.cpp)
CLIENT_OBJS = $(CLIENT_SRCS:.cpp=.o)

# 可执行文件名称
SERVER_TARGET = my_Rpc_server
CLIENT_TARGET = my_Rpc_client

# 链接库（仅用于链接阶段）
LDFLAGS = `pkg-config --libs protobuf` -lpthread -lmysqlclient -lzookeeper_mt

# 通用编译规则
%.o: %.cpp
	@echo "Compiling $<"
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@
	@echo "Compiled $@ successfully"

%.o: %.cc
	@echo "Compiling $<"
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@
	@echo "Compiled $@ successfully"

# 链接生成服务器可执行文件
$(SERVER_TARGET): $(COMMON_OBJS) $(SERVER_OBJS)
	@echo "Linking $@"
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)
	@echo "Linked $@ successfully"

# 链接生成客户端可执行文件
$(CLIENT_TARGET): $(COMMON_OBJS) $(CLIENT_OBJS)
	@echo "Linking $@"
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)
	@echo "Linked $@ successfully"

all: $(SERVER_TARGET) $(CLIENT_TARGET)

# 清理目标文件和可执行文件
clean:
	@echo "Cleaning up..."
	rm -f $(COMMON_OBJS) $(SERVER_OBJS) $(CLIENT_OBJS) $(SERVER_TARGET) $(CLIENT_TARGET)
	@echo "Cleaned up successfully"