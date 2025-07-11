#include <my_log.h>

my_Log::my_Log()
{
    max_log_size = 1024;
    count = 0;
    log_task_queue = nullptr;
    log_close = false;
}

my_Log::~my_Log()
{
    flush();
    delete log_task_queue;
    if(file_ptr != nullptr){
        fclose(file_ptr);
    }
}

my_Log *my_Log::get_instance()
{
    static my_Log instance;
    return &instance;
}


void my_Log::thread_log_write(){
    string log_str;
    std::unique_lock<std::mutex> lock(log_mutex);
    while(!my_Log::log_close){
        log_task_queue->log_task_pop(log_str);
        if(file_ptr != nullptr){
            fputs(log_str.c_str(), file_ptr);
            fflush(file_ptr);
        }
    }
}

void my_Log::write_log(int type,const char* format, ...){
    char type_str[7] = {0};
    switch (type)
    {
    case 0:
        strcpy(type_str,"DEBUG:");
        break;
    case 1:
        strcpy(type_str,"INFO:");
        break;
    case 2:
        strcpy(type_str,"WARN:");
        break;
    case 3:
        strcpy(type_str,"ERROR:");
        break;
    default:
        strcpy(type_str,"DEBUG:");
        break;
    }

    // 使用C++11的chrono库获取当前时间
    std::chrono::system_clock::time_point now = std::chrono::system_clock::now();
    std::time_t now_time = std::chrono::system_clock::to_time_t(now);
    struct tm* time_info = std::localtime(&now_time);
    
    va_list valst;
    va_start(valst, format);

    // 使用C++11的格式化方式构建日志字符串
    char log_str[256] = {0};
    char time_str[24] = {0};
    std::strftime(time_str, sizeof(time_str), "<%Y_%m_%d>%H:%M:%S---", time_info);
    snprintf(log_str, sizeof(time_str) + sizeof(type_str), "%s%s",time_str,type_str);
    vsnprintf(log_str + strlen(log_str), sizeof(log_str) - strlen(log_str), format, valst);
    log_str[strlen(log_str)] = '\n';
    log_str[255] = '\0';

    std::string log_string = log_str;
    log_task_queue->log_task_push(log_string);
}

bool my_Log::init(const char* path, const char* name, int size,bool close){
    log_close = close;
    if(log_close){
        return true;
    }
    
    max_log_size = size;
    count = 0;
    log_task_queue = new my_log_task<string>(size);
    if(log_task_queue == nullptr){
        std::cerr << "log_task_queue create error" << std::endl;
        return false;
    }
    strcpy(log_name,name);
    strcpy(log_path,path);
    memset(log_all_name, '\0', 100);

    time_t t = time(NULL);
    struct tm *sys_tm = localtime(&t);
    struct tm my_tm = *sys_tm;  
    snprintf(log_all_name,99,"%s%04d_%02d_%02d_%s",log_path,my_tm.tm_year + 1900, my_tm.tm_mon + 1, my_tm.tm_mday,log_name);
    
    file_ptr = fopen(log_all_name,"a");
    if(file_ptr == nullptr){
        std::cerr << "log file open error" << std::endl;
        return false;
    }
    thread log_thread(&my_Log::thread_log_write,this);
    log_thread.detach();
    return true;
}


void my_Log::flush(void)
{    
    std::lock_guard<std::mutex> lock(log_mutex);
    //强制刷新写入流缓冲区
    fflush(file_ptr);
}
