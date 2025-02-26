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
    File(std::string fname, uint64 target_size, uint64 rwsize) {
        fd = open(fname.c_str(), O_CREAT | O_RDWR, 00700);
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

    argparse::ArgumentParser args("random file readwrite benchmark");
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
    File f(filename, target, size);

    std::vector<std::shared_future<int>> futures_v;

    for(int i = 0; i < count; i++) {
        
        std::shared_future<int> fut = std::async(&File::rand_read, &f, size);
        std::shared_future<int> fut2 = std::async(&File::rand_write, &f, size);
        futures_v.push_back(fut);
        futures_v.push_back(fut2);
    }

    for(auto& fut : futures_v) {
        fut.wait();
    }


    
}