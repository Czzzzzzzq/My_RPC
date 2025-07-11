    #include <my_task_node.h>
    
    my_Task_node::my_Task_node()
    {
        my_task_status = my_Task_node::task_status::READ;
        my_task_error_status = my_Task_node::error_status::NO_ERROR;
        my_task_socket = -1;
        my_task_read_buffer = "";
        my_task_write_buffer = "";
    }

    my_Task_node::~my_Task_node()
    {
        if(my_task_socket != -1)
        {
            close(my_task_socket);
        }
    }

    void my_Task_node::reset()
    {
        my_task_status = my_Task_node::task_status::COMPLETE;
        my_task_error_status = my_Task_node::error_status::NO_ERROR;
        my_task_socket = -1;
        my_task_read_buffer = "";
        my_task_write_buffer = "";
    }

    my_Task_node::task_status my_Task_node::get_status(){
        return my_task_status;
    }
    void my_Task_node::set_status(task_status status){
        my_task_status = status;
    }
    my_Task_node::error_status my_Task_node::get_error_status(){
        return my_task_error_status;
    }
    void my_Task_node::set_error_status(error_status status){
        my_task_error_status = status;
    }

    int my_Task_node::get_socket(){
        return my_task_socket;
    }
    void my_Task_node::set_socket(int socket){
        my_task_socket = socket;
    }

    std::string my_Task_node::get_read_buffer(){
        return my_task_read_buffer;
    }
    void my_Task_node::set_read_buffer(std::string read_buffer){
        my_task_read_buffer += read_buffer;
    }
    std::string my_Task_node::get_write_buffer(){
        return my_task_write_buffer;
    }
    void my_Task_node::set_write_buffer(std::string write_buffer){
        my_task_write_buffer += write_buffer;
    }

    int my_Task_node::get_listen_end_time(){
        return my_task_listen_end_time;
    }
    int my_Task_node::get_run_end_time(){
        return my_task_run_end_time;
    }
    void my_Task_node::set_time(int listen_end_time,int run_end_time){
        my_task_listen_end_time = listen_end_time;
        my_task_run_end_time = run_end_time;
    }
