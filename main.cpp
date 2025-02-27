#include <iostream>
#include <cstddef>
#include <cstdint>
#include <unistd.h>
#include <future>
#include "argparse.hpp"
#include <string>
#include <fcntl.h>
#include <filesystem>
#include <chrono>
#include <thread>




using uint64 = unsigned int64_t;
using byte = unsigned char;

using time_point_t = decltype( std::chrono::high_resolution_clock::now());
thread_local time_point_t start, end;






uint64 randu64() {  
    //lol
    return (uint64)rand() << 32 | (uint64)rand();
}


struct File {
    int fd;
    uint64 size;
    byte* buf;
    
    void set_fsize() {
        byte c = 0;
        pwrite64(fd, &c, 1, size - 1);
    }
    File(std::string fname, uint64 target_size, uint64 rwsize, int flags = 0) {
        fd = open(fname.c_str(), O_CREAT | O_RDWR | flags, 00700);
        size = target_size;
        set_fsize();
        buf = new byte[rwsize];
    }

    uint64 rand_offset(uint64 rwsize) const {
        return randu64() % (size - rwsize);
    }

    int rand_read(uint64 rwsize) const {
        //byte* buf = new byte[rwsize];
        pread64(fd, buf, rwsize, rand_offset(rwsize));
        //delete[] buf;
        return 0;
    }

    int rand_write(uint64 rwsize) const {
        //byte* buf = new byte[rwsize];
        pwrite64(fd, buf, rwsize, rand_offset(rwsize));
        //delete[] buf;
        return 0;
    };


};



int main(int argc, char** argv) {

    uint64 size = 0, target = 0, count = 0;
    std::string filename;
    bool f_read = 0, f_write = 0;
    bool f_direct_io = 0;

    argparse::ArgumentParser args("random file readwrite benchmark");

    auto &mode = args.add_group("readwrite");

    mode.add_argument("-r", "--read")
        .help("enable read")
        .flag()
        .store_into(f_read);

    mode.add_argument("-w", "--write")
        .help("enable write")
        .flag()
        .store_into(f_write);
    args.add_argument("-d", "--directio")
        .help("direct io")
        .flag()
        .store_into(f_direct_io);

    args.add_argument("-s", "--size")
        .help("rw size")
        .scan<'u', uint64>()
        .required()
        .store_into(size);


    args.add_argument("-t", "--targetsize")
        .help("target size")
        .scan<'u', uint64>()
        .required()
        .store_into(target);
    
    args.add_argument("-c", "--count")
        .help("op count")
        .scan<'u', uint64>()
        .required()
        .store_into(count);
    
    
    args.add_argument("-f", "--file")
        .help("file name")
        .default_value("testfile")
        .required()
        .store_into(filename);



    try
    {
        args.parse_args(argc, argv);
    }
    catch (const std::exception &err)
    {
        std::cerr << err.what() << std::endl;
        std::cerr << args;
        std::exit(1);
    }
    int flags = f_direct_io ? O_DIRECT : 0;
    File f(filename, target, size, flags);

    std::vector<std::shared_future<int>> futures_v;

    for(int i = 0; i < count; i++) {
        if(f_read) {
            std::shared_future<int> fut = std::async(&File::rand_read, &f, size);
            futures_v.push_back(fut);
        }
        if(f_write) {
        
            std::shared_future<int> fut2 = std::async(&File::rand_write, &f, size);
            
            futures_v.push_back(fut2);
        }
    }

    for(auto& fut : futures_v) {
        fut.wait();
    }
    std::cout << "ready to exit\n";
    return 0;

    
}