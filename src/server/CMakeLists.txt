# 定义了一个SRC_LIST 变量，包含了该目录下所有的源文件 还需要包含db下的cpp文件
aux_source_directory(. SRC_LIST)
aux_source_directory(./db DB_LIST)
aux_source_directory(./model MODEL_LIST)
aux_source_directory(./redis REDIS_LIST)

# 指定生成可执行文件
add_executable(ChatServer ${SRC_LIST} ${DB_LIST} ${MODEL_LIST} ${REDIS_LIST})

# 链接库
target_link_libraries(ChatServer muduo_net muduo_base pthread mysqlclient hiredis boost_context)

